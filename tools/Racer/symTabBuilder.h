/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/


#ifndef LLVM_CLANG_SYMTABBUILDER_H
#define LLVM_CLANG_SYMTABBUILDER_H

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "symtab.h"
using namespace llvm;

class SymTabBuilderVisitor : public RecursiveASTVisitor<SymTabBuilderVisitor> {
private:
    ASTContext *astContext; // used for getting additional AST info
    FunctionDecl *current_f=NULL;
    SymTab<SymBase> *symbTab=new SymTab<SymBase>();
    int debugLabel;
public:
    explicit SymTabBuilderVisitor(CompilerInstance *CI, int dl) 
      : astContext(&(CI->getASTContext())), debugLabel(dl) {}
    void dumpSymTab();
    inline SymTab<SymBase> *getSymTab(){return symbTab;} 
    bool VisitFunctionDecl(FunctionDecl *func);    
    bool VisitVarDecl(VarDecl *vdecl);  
};


class SymTabBuilder : public ASTConsumer{
private:
  SymTabBuilderVisitor *visitor; // doesn't have to be private  
public:
    // override the constructor in order to pass CI
  explicit SymTabBuilder(CompilerInstance *CI, int dl)
    : visitor(new SymTabBuilderVisitor(CI,dl)) // initialize the visitor
    {}
 virtual void HandleTranslationUnit(ASTContext &Context) {
    visitor->TraverseDecl(Context.getTranslationUnitDecl());
    visitor->dumpSymTab();
  }   
    
};

#endif
