/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "libExt/CompilerInstanceCtu.h"
#include "libExt/CallGraphCtu.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Sema/Sema.h"
#include "steengaardPAVisitor.h"
#include "symTabBuilder.h"
#include "raceAnalysis.h"
#include "headerDepAnalysis.h"
//#include "callGraphAnalysis.h"
#include "CGFrontendAction.h"
#include "commandOptions.h"
#include <memory>
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;


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


class CGFrontendFactory : public clang::tooling::FrontendActionFactory {

    CallGraph &_cg;
    std::vector<std::unique_ptr<clang::CompilerInstanceCtu>> *astL;
public:
  CGFrontendFactory (CallGraph &cg, std::vector<std::unique_ptr<clang::CompilerInstanceCtu>> &ast)
    : _cg (cg) { astL=&ast;}

    virtual clang::FrontendAction *create () {
      return new CGFrontendAction(_cg);
    }

    bool runInvocation(
    std::shared_ptr<CompilerInvocation> Invocation, FileManager *Files,
    std::shared_ptr<PCHContainerOperations> PCHContainerOps,
    DiagnosticConsumer *DiagConsumer) {
      // Create a compiler instance to handle the actual work.
      std::unique_ptr<clang::CompilerInstanceCtu> Compiler=llvm::make_unique<clang::CompilerInstanceCtu>   (std::move(PCHContainerOps));
      Compiler->setInvocation(std::move(Invocation));
      Compiler->setFileManager(Files);

      // The FrontendAction can have lifetime requirements for Compiler or its
      // members, and we need to ensure it's deleted earlier than Compiler. So we
      // pass it to an std::unique_ptr declared after the Compiler variable.
  
      std::unique_ptr<FrontendAction> ScopedToolAction(create());
   
      // Create the compiler's actual diagnostics engine.
      Compiler->createDiagnostics(DiagConsumer, /*ShouldOwnClient=*/false);
      if (!Compiler->hasDiagnostics())
	return false;
      Compiler->createSourceManager(*Files);
      Compiler->setFrontendAction(ScopedToolAction.get());
      const bool Success = Compiler->ExecuteActionCtu(*ScopedToolAction);
      astL->push_back(std::move(Compiler));
      Files->clearStatCaches();
      return Success;
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
      CallGraph cg;
      std::vector<std::unique_ptr<clang::CompilerInstanceCtu> > vectCI;
      CGFrontendFactory cgFact(cg,vectCI);
      result=Tool.run(&cgFact);
      cg.finishGraphConstruction();
      cg.viewGraph();
      for(auto it=vectCI.begin();it!=vectCI.end();it++)
	{
	  FrontendAction *fact=(*it)->getFrontendAction();
	  if(CGFrontendAction *cgFrontend=static_cast<CGFrontendAction *>(fact)) 
	    cgFrontend->EndFrontendAction();
	  it->get()->EndCompilerActionOnSourceFile();
	}
    }
    if(RA){
      if(op.getSourcePathList().size()!=2) 
      {
	errs()<<"Command Format Error, exactly two sources are required to run the command. See, e.g. racer --help\n";
	return 0;	  
      }

      // Call Graph Construction

      ClangTool ToolCG(op.getCompilations(), op.getSourcePathList());
      CallGraph cg;
      std::vector<std::unique_ptr<clang::CompilerInstanceCtu> > vectCI;
      CGFrontendFactory cgFact(cg,vectCI);
      ToolCG.run(&cgFact);
      cg.finishGraphConstruction();
      cg.viewGraph();      
      // Race Detection
      result = Tool.run(newFrontendActionFactory<RacerFrontendAction>().get());
      racer->setCallGraph(&cg);
      std::ofstream Method1(FUNC1.c_str()), Method2(FUNC2.c_str());
      if (Method1.good() && Method2.good()) 
	racer->setTaskStartPoint(FUNC1.c_str(),FUNC2.c_str());
      racer->extractPossibleRaces();
      
      // Finish compiler instances
      for(auto it=vectCI.begin();it!=vectCI.end();it++)
	{
	  FrontendAction *fact=(*it)->getFrontendAction();
	  if(CGFrontendAction *cgFrontend=static_cast<CGFrontendAction *>(fact)) 
	    cgFrontend->EndFrontendAction();
	  it->get()->EndCompilerActionOnSourceFile();
	}

    }

   return result;
}
