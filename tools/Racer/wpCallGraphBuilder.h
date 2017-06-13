#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "clang/Analysis/CallGraph.h"
#include "clang/Analysis/AnalysisContext.h"


using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;



class CallGraphASTConsumer : public ASTConsumer {
private:
  CallGraph CG;
public:
    // override the constructor in order to pass CI
    explicit CallGraphASTConsumer(CompilerInstance *CI)
 //     : visitor(new ExampleVisitor(CI)) // initialize the visitor
      {
      }

    // override this to call our ExampleVisitor on the entire source file
     virtual void HandleTranslationUnit(ASTContext &Context) {
         
        TranslationUnitDecl *tu=Context.getTranslationUnitDecl();
	CG.addToCallGraph(tu);
	CG.viewGraph();
	
    }   

    
};

