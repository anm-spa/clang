/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef LLVM_CLANG_CGFRONTENDACTION_H
#define LLVM_CLANG_CGFRONTENDACTION_H


#include "clang/Driver/Options.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "libExt/CompilerInstanceCtu.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/Tooling.h"
#include "libExt/CallGraphCtu.h"

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;

class CGConsumer : public ASTConsumer {
private:
  CallGraph &CG;
public:
  explicit CGConsumer(CompilerInstance *CI,CallGraph &g) : CG(g)
  {}

  virtual void HandleTranslationUnit(ASTContext &Context) {     
    TranslationUnitDecl *tu=Context.getTranslationUnitDecl();
    CG.addToCallGraph(tu);
  }    
};


class CGFrontendAction : public ASTFrontendAction {
  CallGraph &_cg;
public:
  CGFrontendAction(CallGraph &cg)
  : _cg (cg) {}

  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file)   {
    llvm::errs()<<"Building Call Graph of "<<file.str()<<"\n"; 
    return llvm::make_unique<CGConsumer>(&CI,_cg);
  }

~CGFrontendAction(){
}

void EndSourceFileAction() {}  
void EndSourceFileCtu() { }

void EndActionOnSourceFile() {}  

void EndFrontendAction()
{
  EndActionOnSourceFile();
  setCompilerInstance(nullptr);
  //setCurrentInput(FrontendInputFile());
}

};

#endif
