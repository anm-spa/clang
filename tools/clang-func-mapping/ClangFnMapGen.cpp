//===- ClangFnMapGen.cpp -----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//
//
// Clang tool which creates a list of defined functions and the files in which
// they are defined.
//
//===--------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Signals.h"
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <unistd.h>
#include <vector>

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

typedef StringSet<> StrSet;
typedef StringMap<StrSet> CallGraph;

static cl::OptionCategory ClangFnMapGenCategory("clang-fnmapgen options");
static cl::opt<std::string> CTUDir(
    "xtu-dir",
    cl::desc(
        "Directory that contains the CTU related files (e.g.: AST dumps)."),
    cl::init(""), cl::cat(ClangFnMapGenCategory));

static cl::opt<bool> UseUSR("use-usr",
                            cl::desc("Use USR instead of name mangling."),
                            cl::init(false), cl::cat(ClangFnMapGenCategory));
static cl::opt<bool> UseRP("use-realpath",
                            cl::desc("Use real path of source files names in output files."),
                            cl::init(false), cl::cat(ClangFnMapGenCategory));

static void lockedWrite(StringRef FileName, StringRef Content) {
  int fd = open(FileName.str().c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
  flock(fd, LOCK_EX);
  ssize_t written = write(fd, Content.data(), Content.size());
  assert(written == (ssize_t)Content.size());
  (void)written;
  flock(fd, LOCK_UN);
  close(fd);
}

static std::string getTripleSuffix(ASTContext &Ctx) {
  // We are not going to support vendor and don't support OS and environment.
  // FIXME: support OS and environment correctly.
  Triple::ArchType T = Ctx.getTargetInfo().getTriple().getArch();
  if (T == Triple::thumb)
    T = Triple::arm;
  return Ctx.getTargetInfo().getTriple().getArchTypeName(T);
}

class MapFunctionNamesConsumer : public ASTConsumer {
private:
  ASTContext &Ctx;
  ItaniumMangleContext *ItaniumCtx;
  std::stringstream DefinedFuncsStr;
  std::stringstream ExternFuncStr;
  CallGraph CG;
  const std::string Triple;

public:
  MapFunctionNamesConsumer(ASTContext &Context, ItaniumMangleContext *MangleCtx)
      : Ctx(Context), ItaniumCtx(MangleCtx),
        Triple(std::string("@") + getTripleSuffix(Context)) {}
  std::string CurrentFileName;

  ~MapFunctionNamesConsumer();
  virtual void HandleTranslationUnit(ASTContext &Ctx) {
    handleDecl(Ctx.getTranslationUnitDecl());
  }

private:
  std::string getMangledName(const FunctionDecl *FD, MangleContext *Ctx);
  std::string getMangledName(const FunctionDecl *FD) {
    return getMangledName(FD, ItaniumCtx);
  }

  bool isCLibraryFunction(const FunctionDecl *FD);
  void handleDecl(const Decl *D);

  class WalkAST : public ConstStmtVisitor<WalkAST> {
    MapFunctionNamesConsumer &Parent;
    std::string CurrentFuncName;
    MangleContext *MangleCtx;
    const std::string Triple;

  public:
    WalkAST(MapFunctionNamesConsumer &parent, const std::string &FuncName,
            MangleContext *Ctx, const std::string &triple)
        : Parent(parent), CurrentFuncName(FuncName), MangleCtx(Ctx),
          Triple(triple) {}
    void VisitCallExpr(const CallExpr *CE);
    void VisitStmt(const Stmt *S) { VisitChildren(S); }
    void VisitChildren(const Stmt *S);
  };
};

std::string MapFunctionNamesConsumer::getMangledName(const FunctionDecl *FD,
                                                     MangleContext *Ctx) {
  std::string MangledName;
  llvm::raw_string_ostream os(MangledName);
  if (const auto *CCD = dyn_cast<CXXConstructorDecl>(FD))
    // FIXME: Use correct Ctor/DtorType.
    Ctx->mangleCXXCtor(CCD, Ctor_Complete, os);
  else if (const auto *CDD = dyn_cast<CXXDestructorDecl>(FD))
    Ctx->mangleCXXDtor(CDD, Dtor_Complete, os);
  else
    Ctx->mangleName(FD, os);
  os.flush();
  return MangledName;
}

void MapFunctionNamesConsumer::handleDecl(const Decl *D) {
  if (!D)
    return;

  if (const auto *FD = dyn_cast<FunctionDecl>(D)) {
    if (const Stmt *Body = FD->getBody()) {
      std::string MangledName = getMangledName(FD);
      const SourceManager &SM = Ctx.getSourceManager();
      if (CurrentFileName.empty()) {
        StringRef SMgrName =
            SM.getFileEntryForID(SM.getMainFileID())->getName();
        char *Path = realpath(SMgrName.str().c_str(), nullptr);
        CurrentFileName = Path;
        free(Path);
      }

      SmallString<128> FileName("");
      if (!UseRP){
        FileName="ast";
        llvm::sys::path::append(FileName, getTripleSuffix(Ctx), CurrentFileName);
      }else
        FileName=CurrentFileName;
      std::string FullName;
      if (UseUSR) {
        SmallString<128> DeclUSR;
        bool Res = index::generateUSRForDecl(D, DeclUSR);
        if (Res)
          D->dump();
        assert(!Res);
        FullName = DeclUSR.str().str();
      } else {
        FullName = MangledName + Triple;
      }

      switch (FD->getLinkageInternal()) {
      case ExternalLinkage:
      case VisibleNoLinkage:
      case UniqueExternalLinkage:
        if (SM.isInMainFile(Body->getLocStart()))
          DefinedFuncsStr << "!";
        DefinedFuncsStr << FullName << " " << FileName.c_str() << " 0\n";
      default:
        break;
      }

      WalkAST Walker(*this, FullName, ItaniumCtx, Triple);
      Walker.Visit(Body);
    } else if (!FD->getBody() && !FD->getBuiltinID()) {
      std::string MangledName;
      if (UseUSR) {
        SmallString<128> DeclUSR;
        bool Res = index::generateUSRForDecl(D, DeclUSR);
        if (Res)
          D->dump();
        assert(!Res);
        MangledName = DeclUSR.str().str();
      } else {
        MangledName = getMangledName(FD) + Triple;
      }

      ExternFuncStr << MangledName << "\n";
    }
  }

  if (const auto *DC = dyn_cast<DeclContext>(D))
    for (const Decl *D : DC->decls())
      handleDecl(D);
}

bool MapFunctionNamesConsumer::isCLibraryFunction(const FunctionDecl *FD) {
  SourceManager &SM = Ctx.getSourceManager();
  if (!FD)
    return false;
  SourceLocation Loc = FD->getLocation();
  if (Loc.isValid())
    return SM.isInSystemHeader(Loc);
  return true;
}

MapFunctionNamesConsumer::~MapFunctionNamesConsumer() {
  // Flush results to files.
  SmallString<128> ExternalFns(CTUDir);
  SmallString<128> DefinedFns(CTUDir);
  SmallString<128> CfgFile(CTUDir);
  llvm::sys::path::append(ExternalFns, "externalFns.txt");
  llvm::sys::path::append(DefinedFns, "definedFns.txt");
  llvm::sys::path::append(CfgFile, "cfg.txt");
  lockedWrite(ExternalFns, ExternFuncStr.str());
  lockedWrite(DefinedFns, DefinedFuncsStr.str());
  std::ostringstream CFGStr;
  for (auto &Entry : CG) {
    CFGStr << CurrentFileName << Triple << "::" << Entry.getKey().data();
    for (auto &E : Entry.getValue())
      CFGStr << ' ' << E.getKey().data();
    CFGStr << '\n';
  }

  lockedWrite(CfgFile, CFGStr.str());
}

void MapFunctionNamesConsumer::WalkAST::VisitChildren(const Stmt *S) {
  for (const Stmt *CS : S->children())
    if (CS)
      Visit(CS);
}

void MapFunctionNamesConsumer::WalkAST::VisitCallExpr(const CallExpr *CE) {
  const auto *FD = dyn_cast_or_null<FunctionDecl>(CE->getCalleeDecl());
  if (FD && !FD->getBuiltinID()) {
    std::string FuncName = (FD->hasBody() ? "::" : "") +
                           Parent.getMangledName(FD, MangleCtx) + Triple;
    Parent.CG[CurrentFuncName].insert(FuncName);
  }
  VisitChildren(CE);
}

class MapFunctionNamesAction : public ASTFrontendAction {
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) {
    ItaniumMangleContext *ItaniumCtx =
        ItaniumMangleContext::create(CI.getASTContext(), CI.getDiagnostics());
    ItaniumCtx->setShouldForceMangleProto(true);
    std::unique_ptr<ASTConsumer> PFC(
        new MapFunctionNamesConsumer(CI.getASTContext(), ItaniumCtx));
    return PFC;
  }
};

int main(int argc, const char **argv) {
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(argv[0], false);
  PrettyStackTraceProgram X(argc, argv);

  SmallVector<std::string, 4> Sources;
  CommonOptionsParser OptionsParser(argc, argv, ClangFnMapGenCategory,
                                    cl::ZeroOrMore);

  if (CTUDir.getNumOccurrences() != 1) {
    errs() << "Exactly one CTU dir should be provided\n";
    return 1;
  }
  const StringRef cppFile = ".cpp", ccFile = ".cc", cFile = ".c",
                  cxxFile = ".cxx";
  for (int i = 1; i < argc; i++) {
    StringRef arg = argv[i];
    if (arg.endswith(cppFile) || arg.endswith(ccFile) || arg.endswith(cFile) ||
        arg.endswith(cxxFile)) {
      Sources.push_back(arg);
    }
  }
  ClangTool Tool(OptionsParser.getCompilations(), Sources);
  Tool.run(newFrontendActionFactory<MapFunctionNamesAction>().get());
  return 0;
}
