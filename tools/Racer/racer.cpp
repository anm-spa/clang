/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "steengaardPAVisitor.h"
#include "raceAnalysis.h"
#include "headerDepAnalysis.h"
#include "callGraphAnalysis.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory RacerOptCat("Static Analysis Options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

static cl::opt<bool> Symb("sym",cl::desc("Build and dump the Symbol Table"), cl::cat(RacerOptCat));
static cl::opt<bool> PA("pa",cl::desc("show pointer analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> HA("ha",cl::desc("show header analysis info"), cl::cat(RacerOptCat));
static cl::opt<bool> CG("cg",cl::desc("Call Graph Info"), cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC1("m1",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));
static cl::opt<std::string> FUNC2("m2",cl::desc("Initial method from which execution starts"), cl::value_desc("function name"),cl::cat(RacerOptCat));

static cl::opt<bool> RA("ra",cl::desc("Data race analysis"), cl::cat(RacerOptCat));

enum DLevel {
  O, O1, O2, O3
};
cl::opt<DLevel> DebugLevel("dl", cl::desc("Choose Debug level:"),
  cl::values(
	     clEnumValN(O, "none", "No debugging"),
	     clEnumVal(O1, "Minimal debug info"),
	     clEnumVal(O2, "Expected debug info"),
	     clEnumVal(O3, "Extended debug info")), cl::cat(RacerOptCat));

RaceFinder *racer=new RaceFinder();
int debugLabel=0;
void VisualizeCFG(clang::AnalysisDeclContext *ac,clang::CFG *cfg)
{
   //errs()<<" Printing CFGs"<<"\n";
   ac->dumpCFG(false);
}

class PointerAnalysis : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
  explicit PointerAnalysis(CompilerInstance *CI,std::string file)
    : visitorPA(new SteengaardPAVisitor(CI,debugLabel,file)),visitorSymTab(new SymTabBuilderVisitor(CI,debugLabel))
  {
  }

  // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
    
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    errs()<<"building initPA\n";
    visitorPA->initPA(visitorSymTab->getSymTab());
    errs()<<"Traverse Decl\n";
    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    errs()<<"Store Global Pointer\n";
    visitorPA->storeGlobalPointers();
    errs()<<"show PA\n";
    visitorPA->showPAInfo();
    errs()<<"Show globals\n";
    visitorPA->getGvHandler()->showGlobals();
    }   
};


class RaceDetector : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
  explicit RaceDetector(CompilerInstance *CI, std::string file)
    : visitorPA(new SteengaardPAVisitor(CI,debugLabel,file)),visitorSymTab(new SymTabBuilderVisitor(CI, debugLabel))
      {
      }

    // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    
    visitorPA->initPA(visitorSymTab->getSymTab());
    
    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    visitorPA->storeGlobalPointers();
    visitorPA->showPAInfo();   
    //visitorPA->showVarReadWriteLoc(); 

    racer->createNewTUAnalysis(visitorPA->getGvHandler());
    }   
};

class PAFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    std::cout<<"Pointer Analysis of "<<file.str()<<"\n";
    return llvm::make_unique<PointerAnalysis>(&CI,file.str()); // pass CI pointer to ASTConsumer
   }
};

class CGFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    std::cout<<"Building Call Graph of "<<file.str()<<"\n";
    std::ofstream Method1(FUNC1.c_str()), Method2(FUNC2.c_str());
    if (Method1.good() && Method2.good()) //errs()<<"Method Name :"<<FUNC.c_str()<<"\n";
      return llvm::make_unique<CGReachabilityInf>(&CI,FUNC1.c_str(),FUNC2.c_str()); 
    else return llvm::make_unique<CGReachabilityInf>(&CI);
   }
};


class RacerFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    return llvm::make_unique<RaceDetector>(&CI,file.str()); // pass CI pointer to ASTConsumer
    //return llvm::make_unique<CallGraphASTConsumer>(&CI); // pass CI pointer to ASTConsumer	
   }
};

class SymbTabAction : public ASTFrontendAction {
 public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)    {
      return llvm::make_unique<SymTabBuilder>(&CI,debugLabel);
     }
};

int main(int argc, const char **argv) {
    // parse the command-line args passed to your code 

    CommonOptionsParser op(argc, argv, RacerOptCat);        
    // create a new Clang Tool instance (a LibTooling environment)
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
   	
    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result;
    if(DebugLevel==O1) debugLabel=1;
    else if(DebugLevel==O2) debugLabel=2;
    else if(DebugLevel==O3) debugLabel=3;
    if(!Symb && !PA && !HA && !CG && !RA)
    { 
      errs()<<"Analysis options are not provided. See, e.g., racer --help\n";
      return 0;
    }  
    if(Symb) result = Tool.run(newFrontendActionFactory<SymbTabAction>().get());
    if(PA) result = Tool.run(newFrontendActionFactory<PAFrontendAction>().get());
    if(HA) {
       RepoGraph repo(debugLabel);
       IncAnalFrontendActionFactory act (repo);
       Tool.run(&act);
       std::vector<std::string> hFiles=repo.getHeaderFiles();
       repo.printHeaderList();

       ClangTool Tool1(op.getCompilations(), hFiles);
       result = Tool1.run(newFrontendActionFactory<PAFrontendAction>().get());
    }
    if(CG){
      
      result=Tool.run(newFrontendActionFactory<CGFrontendAction>().get());
    }
    if(RA){
      if(op.getSourcePathList().size()!=2) 
      {
	errs()<<"Command Format Error, exactly two sources are required to run the command. See, e.g. racer --help\n";
	return 0;	  
      }
      result = Tool.run(newFrontendActionFactory<RacerFrontendAction>().get());
      racer->extractPossibleRaces();
    }

   return result;
}
