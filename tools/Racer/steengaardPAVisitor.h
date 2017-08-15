/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef STEENSGAARDPAVISITOR_H_
#define STEENSGAARDPAVISITOR_H_

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "steengaardPA.h"
#include "GlobalVarHandler.h"
#include "symTabBuilder.h"

// CFG related issue
#include "clang/Analysis/AnalysisContext.h"
#include <vector>
#include <algorithm>
#include<map>

using namespace std;
using namespace clang;

class SteengaardPAVisitor : public RecursiveASTVisitor<SteengaardPAVisitor> {
private:
  ASTContext *astContext;                    // used for getting additional AST info
  CSteensgaardPA *pa;                        // PA solver object
  GlobalVarHandler gv;
  SymTab<SymBase> *_symbTab;
  bool analPointers;                           
  FuncSignature * current_fs;
  bool isVisitingFunc;
  std::multimap<clang::SourceLocation,std::pair<unsigned,AccessType> >  varMod; 
  int debugLabel;
public:
  explicit SteengaardPAVisitor(CompilerInstance *CI, int dl) 
    : astContext(&(CI->getASTContext())), isVisitingFunc(false), debugLabel(dl) // initialize private members
    {}

  void initPA(SymTab<SymBase> *symbTab);
  inline void importSymTab(SymTab<SymBase> *symbTab) { _symbTab=symbTab;}  
  inline GlobalVarHandler *getGvHandler() {return &gv;}
  void buildPASet();
  void rebuildPASet();
  void showPAInfo(bool prIntrls=false); 
  void recordVarAccess(clang::SourceLocation l, std::pair<unsigned,AccessType> acc);
  void showVarReadWriteLoc();
  void storeGlobalPointers();
  bool VisitFunctionDecl(FunctionDecl *func);
  std::pair<unsigned,PtrType> getExprIdandType(clang::Expr *exp);
  void updatePAonUnaryExpr(clang::UnaryOperator *uop);
  void updatePAonBinaryExpr(clang::BinaryOperator *bop);
  bool VisitStmt(Stmt *st);
  std::set<FunctionDecl *> getCallsFromFuncPtr(CallExpr *call);
  void updatePAOnFuncCall(FunctionDecl *calleeDecl,std::set<Expr*> ActualInArgs,long ActualOutArg);
  void updatePAOnCallExpr(CallExpr *call,long ActOutArg);
  void updatePABasedOnExpr(unsigned id, Expr * exp);
  bool traverse_subExpr(Expr * exp);
  bool VisitVarDecl(VarDecl *vDecl);
 
};

#endif
