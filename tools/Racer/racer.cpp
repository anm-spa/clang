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


class RaceDetector : public ASTConsumer {
private:
  SteengaardPAVisitor *visitorPA; 
  symTabBuilderVisitor *visitorSymTab;
public:
    // override the constructor in order to pass CI
    explicit RaceDetector(CompilerInstance *CI)
      : visitorPA(new SteengaardPAVisitor(CI)),visitorSymTab(new symTabBuilderVisitor(CI))
      {
      }

    // override this to call our ExampleVisitor on the entire source file
  virtual void HandleTranslationUnit(ASTContext &Context) {
         //we can use ASTContext to get the TranslationUnitDecl, which is
         //    a single Decl that collectively represents the entire source file 

        // building Symbol Table
    
    visitorSymTab->TraverseDecl(Context.getTranslationUnitDecl());
    visitorSymTab->dumpSymTab();
    
    visitorPA->initPA(visitorSymTab->getSymTab());
    //visitorPA->printPAInfo();      
    visitorPA->TraverseDecl(Context.getTranslationUnitDecl());
    visitorPA->storeGlobalPointers();	
    visitorPA->printPAInfo();   
    //visitorPA->printVarAccessInfo(); 

    racer->createNewTUAnalysis(visitorPA->getGvHandler());
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
      return llvm::make_unique<symTabBuilder>(&CI);
     }
};


static cl::OptionCategory RacerOptCat("Racer tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

static cl::opt<bool> Symb("sym",cl::desc("Build and dump the Symbol Table"));

int main(int argc, const char **argv) {
    // parse the command-line args passed to your code 
    CommonOptionsParser op(argc, argv, RacerOptCat);        
    // create a new Clang Tool instance (a LibTooling environment)
    	
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());


    // init_DataFlowInfo(15);
   	
    // run the Clang Tool, creating a new FrontendAction (explained below)
    int result;
    if(Symb) errs()<<"-sym option set\n";
    if(Symb) result = Tool.run(newFrontendActionFactory<SymbTabAction>().get());
    else{
      result = Tool.run(newFrontendActionFactory<RacerFrontendAction>().get());
      //analysisInfo->extractPossibleRaces();
      racer->extractPossibleRaces();
    }

   return result;
}
