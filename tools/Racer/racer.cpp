#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "SteengaardPAVisitor.h"
#include "RaceAnalysis.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

RaceFinder *racer=new RaceFinder();

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
    explicit PointerAnalysis(CompilerInstance *CI)
      : visitorPA(new SteengaardPAVisitor(CI)),visitorSymTab(new SymTabBuilderVisitor(CI))
  {
  }

  // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
    
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    
    visitorPA->initPA(visitorSymTab->getSymTab());

    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    
    visitorPA->showPAInfo();   
    }   
};



class RaceDetector : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  SymTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
    explicit RaceDetector(CompilerInstance *CI)
      : visitorPA(new SteengaardPAVisitor(CI)),visitorSymTab(new SymTabBuilderVisitor(CI))
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
    return llvm::make_unique<PointerAnalysis>(&CI); // pass CI pointer to ASTConsumer
   }
};


class RacerFrontendAction : public ASTFrontendAction {
 public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    return llvm::make_unique<RaceDetector>(&CI); // pass CI pointer to ASTConsumer
    //return llvm::make_unique<CallGraphASTConsumer>(&CI); // pass CI pointer to ASTConsumer	
   }
};

class SymbTabAction : public ASTFrontendAction {
 public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)    {
      return llvm::make_unique<SymTabBuilder>(&CI);
     }
};


static cl::OptionCategory RacerOptCat("Static Analysis Options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

static cl::opt<bool> Symb("sym",cl::desc("Build and dump the Symbol Table"));
static cl::opt<bool> PA("pa",cl::desc("show pointer analysis info"));

int main(int argc, const char **argv) {
    // parse the command-line args passed to your code 
    CommonOptionsParser op(argc, argv, RacerOptCat);        
    // create a new Clang Tool instance (a LibTooling environment)
    	
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());
   	
    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result;
    if(Symb) result = Tool.run(newFrontendActionFactory<SymbTabAction>().get());
    if(PA) result = Tool.run(newFrontendActionFactory<PAFrontendAction>().get());
    if(!Symb && !PA){
      result = Tool.run(newFrontendActionFactory<RacerFrontendAction>().get());
      //analysisInfo->extractPossibleRaces();
      racer->extractPossibleRaces();
    }

   return result;
}
