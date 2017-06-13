#ifndef STEENSGAARDPAVISITOR_H_
#define STEENSGAARDPAVISITOR_H_

#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "steengaardPA.h"
#include "GlobalVarHandler.h"
#include "wpCallGraphBuilder.h"
#include "symTabBuilder.h"

//#include "clang/AST/Stmt.h"
//#include "unionfind.h"

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
  symTab<symIdentBase> *_symbTab;
  bool analPointers;                           
  fsignature * current_fs;
  std::map<clang::SourceLocation,std::pair<unsigned,AccessType> > varAcc; 

public:
  explicit SteengaardPAVisitor(CompilerInstance *CI) 
    : astContext(&(CI->getASTContext())) // initialize private members
    {}

  void initPA(symTab<symIdentBase> *symbTab)
  {
    _symbTab=symbTab;
    assert(_symbTab);
    std::set<unsigned> vars;
    std::set<fsignature *> funcs;
    pa=new CSteensgaardPA();
    analPointers=false;
    _symbTab->getVarsAndFuncs(vars,funcs);
    pa->initPASolver(vars,funcs);
  }  
  void importSymTab(symTab<symIdentBase> *symbTab)
  {
    _symbTab=symbTab;
  }  

  GlobalVarHandler *getGvHandler() 
  {  
    return &gv;
  }

  void buildPASet()
  {
    if(!analPointers)
      {
	pa->BuildVarToVarsAndVarToLabelsPointToSets();
	analPointers=true;
      }     
  }
  void rebuildPASet()
  {
    pa->BuildVarToVarsAndVarToLabelsPointToSets();
    analPointers=true;
  }

  void printPAInfo(bool prIntrls=false) 
  {
    rebuildPASet();
    pa->PrintAsPointsToSets(_symbTab);
    if(prIntrls)
      pa->PrintInternals();
      
  }

  void writeVarAccessInfo(clang::SourceLocation l, std::pair<unsigned,AccessType> acc)
  {
    varAcc.insert(std::pair<clang::SourceLocation, std::pair<unsigned,AccessType> >(l,acc));
  }

  void printVarAccessInfo()
  {
    std::map<clang::SourceLocation,std::pair<unsigned,AccessType> >::iterator it=varAcc.begin();
    for(;it!=varAcc.end();it++)
      {
	std::string var=((_symbTab->lookupSymb(it->second.first))->getVarDecl())->getNameAsString();
	errs()<<" Accessed Var "<<var<<" at "<<it->first.printToString(astContext->getSourceManager())<<" : "<<printAccessType(it->second.second)<<"\n";   
      }	  
  }

void storeGlobalPointers()
{
  buildPASet();
  for(SetIter it=gv.gVarsBegin();it!=gv.gVarsEnd();it++)
  { 
    std::set<unsigned> vars_pointed_to; 
    pa->GetPointedAtVar(*it,&vars_pointed_to);
    if(!vars_pointed_to.empty())
      gv.insertPtsToGv(vars_pointed_to,*it);
  }
    
  //split pointers according to Read or Write access 
  std::map<clang::SourceLocation,std::pair<unsigned,AccessType> >::iterator it=varAcc.begin();
  for(;it!=varAcc.end();it++)
  {
    if(gv.isPtrToGv(it->second.first)||gv.isGv(it->second.first))
    { 
       std::set<unsigned> vars_pointed_to;
       std::set<unsigned> intsect;
       std::string loc=it->first.printToString(astContext->getSourceManager());
       std::string vCurr=((_symbTab->lookupSymb(it->second.first))->getVarDecl())->getNameAsString();
       pa->GetPointsToVars(it->second.first,&vars_pointed_to);
       if(gv.isGv(it->second.first))
	 vars_pointed_to.insert(it->second.first);
       std::set_intersection(vars_pointed_to.begin(), vars_pointed_to.end(),gv.gVarsBegin(),gv.gVarsEnd(), std::inserter(intsect,intsect.begin()));
       if(it->second.second==RD) { 
	 gv.storeGlobalRead(intsect,loc);
	 for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
	   std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
	   gv.storeMapInfo(loc,vCurr,vGlobal);
	 }   	 
       }
       else {
	 gv.storeGlobalWrite(intsect,loc);
	 for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
   	   std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
	   gv.storeMapInfo(loc,vCurr,vGlobal);
	 }  
       }
    }     
  }    
}


bool VisitFunctionDecl(FunctionDecl *func) {
  if(!astContext->getSourceManager().isInSystemHeader(func->getLocStart())){
    string funcName = func->getNameInfo().getName().getAsString();
    
    //string retName = func->getReturnType().getAsString();
    for(unsigned int i=0; i<func->getNumParams(); i++)
      {
	clang::ParmVarDecl *p=func->parameters()[i];
	if(clang::VarDecl *vd=dyn_cast<clang::VarDecl>(p))
	  {
	    VisitVarDecl(vd);
	    if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vd)) {	    
	      unsigned vid=_symbTab->lookupId(val);
	      //writeVarAccessInfo(vd->getSourceRange().getBegin(),std::pair<unsigned,AccessType>(vid,RD)); 
	    }
	  }  
	// errs()<<QualType::getAsString(p->getType().split());
	// errs() << " " << p->getQualifiedNameAsString();
      }

      current_fs=_symbTab->lookupfunc(func);       

    /*  clang::CFG *cfg;
	clang::AnalysisDeclContext ac(nullptr, func);     
        if(!(cfg=ac.getCFG()))
	{errs()<< "CFG cannot be built"<<"\n";exit(1);}
	VisualizeCFG(&ac,cfg);
        */
	// Stmt *FuncBody=func->getBody();
	// FuncBody->dump(); 
  }
  return true;
}
  
std::pair<unsigned,PtrType> getExprTypeAndId(clang::Expr *exp)
{
  if(clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
  {
    if(clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit())
      if(clang::DeclRefExpr *dexp = clang::dyn_cast<clang::DeclRefExpr>(subexp))
	if(clang::ValueDecl *vdec=dyn_cast<clang::ValueDecl>(dexp->getDecl()))
	{
	  clang::UnaryOperator::Opcode op=uop->getOpcode();
	  unsigned id=_symbTab->lookupId(vdec);
	  switch(op){
	  case UO_Deref:  
	    return std::pair<unsigned,PtrType>(id,PTR_L1);
	    break;	    
	  case UO_AddrOf:
	    return std::pair<unsigned,PtrType>(id,ADDR_OF);
	    break;
	  default: break;                   
	  }
	}
  } 
  else if(clang::DeclRefExpr *ref = clang::dyn_cast<clang::DeclRefExpr>(exp))  
    {
      if(clang::ValueDecl *vdec=dyn_cast<clang::ValueDecl>(ref->getDecl()))
	{
  	  unsigned id=_symbTab->lookupId(vdec);
	  if(vdec->getType()->isPointerType())
	    return std::pair<unsigned,PtrType>(id,PTR_L0);           
	  else 
	    return std::pair<unsigned,PtrType>(id,NOPTR); 
	}    
    }   
  return std::pair<unsigned,PtrType>(0,UNDEF);
 }
   
 void visitBinaryOperator(clang::BinaryOperator *bop)
 {
   clang::BinaryOperator::Opcode opcode=bop->getOpcode();
   if(opcode==clang::BO_Assign)    
     {
       clang::Expr *lhs = bop->getLHS()->IgnoreImplicit();
       clang::Expr *rhs = bop->getRHS()->IgnoreImplicit();
       std::pair<unsigned,PtrType> lhs_pair=getExprTypeAndId(lhs);
       std::pair<unsigned,PtrType> rhs_pair=getExprTypeAndId(rhs);
       
       if((lhs_pair.second==PTR_L0) && (rhs_pair.second==PTR_L0))
	 {
	   //errs()<< "Address e1=e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignStmt(lhs_pair.first,rhs_pair.first);  
	   
	 }
       else if((lhs_pair.second==PTR_L0) && (rhs_pair.second==ADDR_OF))
	 {
	   //errs()<< "Address e1=&e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignAddrStmt(lhs_pair.first,rhs_pair.first);
	 }
       else if((lhs_pair.second==PTR_L0) && (rhs_pair.second==PTR_L1))
	 {
	   //errs()<< "Assignment e1=*e2"<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignFromIndStmt(lhs_pair.first,rhs_pair.first);
	   writeVarAccessInfo(rhs->getExprLoc(),std::pair<unsigned,AccessType>(rhs_pair.first,RD));
	   writeVarAccessInfo(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	 }
       else if((lhs_pair.second==PTR_L1) && (rhs_pair.second==PTR_L0))
	 {
	   // errs()<< "Assignment *e1=e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignToIndStmt(lhs_pair.first,rhs_pair.first);
	   writeVarAccessInfo(rhs->getExprLoc(),std::pair<unsigned,AccessType>(rhs_pair.first,RD));
	   writeVarAccessInfo(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	 }
       else if(CallExpr *call = dyn_cast<CallExpr>(rhs)) 
	 {
	   if(lhs_pair.second==PTR_L1|| lhs_pair.second==PTR_L0)
	     updatePAOnCallExpr(call,true,lhs_pair.first);
	   else 
	     updatePAOnCallExpr(call,false,0);
	 } 
       else
	 { 
	   writeVarAccessInfo(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	   traverse_subExpr(rhs);
	 } 
     }    
 }

bool VisitStmt(Stmt *st) 
{
  // const char *stType=st->getStmtClassName();
  //  printf("\t%s \n",  
  if (DeclStmt *decl_stmt=dyn_cast<DeclStmt>(st)) {               
    // info:consider statement like int x; or int x=10,y,z; etc
    //decl_stmt->dump();

    for(clang::DeclStmt::const_decl_iterator it=decl_stmt->decl_begin();it!=decl_stmt->decl_end();it++)   { 
      // single statement may contain multiple declarations     
      if(VarDecl *vardecl=dyn_cast<VarDecl>(*it))    // if it is a variable declaration
	{
	  VarDecl *def=vardecl->getDefinition();
	  if(def) VisitVarDecl(def);
	  Expr * exp=const_cast<clang::Expr *>(def->getAnyInitializer());
	  if(exp){
	    traverse_subExpr(exp->IgnoreImplicit());
	    if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(def)) {	    
	      unsigned vid=_symbTab->lookupId(val);
	      writeVarAccessInfo(def->getSourceRange().getBegin(),std::pair<unsigned,AccessType>(vid,WR));
	    }
	  // Locationdef->getSourceRange().getBegin().printToString(astContext->getSourceManager())
	  }
	}
    }
  }   
  else if (clang::BinaryOperator *bop=dyn_cast<clang::BinaryOperator>(st))
    visitBinaryOperator(bop); 

  // Update PA for function calls
  else if (CallExpr *call = dyn_cast<CallExpr>(st)) {
    updatePAOnCallExpr(call,false,0);
  }

  else if (ReturnStmt *rtst = dyn_cast<ReturnStmt>(st)) {
    Expr *retexpr=rtst->getRetValue()->IgnoreImplicit();
    unsigned id=current_fs->rets[0];
    updatePABasedOnExpr(id,retexpr);   
  }  
  else if (IfStmt *ifstmt = dyn_cast<IfStmt>(st)) {
    traverse_subExpr(ifstmt->getCond()->IgnoreImplicit());
  }  
  return true;
}

 void updatePAOnCallExpr(CallExpr *call,bool hasRet, unsigned retId)
 {
  fsignature * fsig=NULL;
  if(clang::FunctionDecl *fdecl=dyn_cast<clang::FunctionDecl>(call->getCalleeDecl()))
    { 
      //errs()<<"Function "<<fdecl->getNameInfo().getName().getAsString()<<": return type ";
      //errs()<< fdecl->getReturnType().getAsString()<<"\n";
      fsig=_symbTab->lookupfunc(fdecl);       
    //unsigned func=pa->CreateNewVar();
    //updatePABasedOnExpr(func,call);           // implement this function
      if(fsig){
	int i=0;
	for(clang::CallExpr::arg_iterator it=call->arg_begin();it!=call->arg_end();it++,i++)
	{
          //errs()<<"Function "<<fdecl->getNameInfo().getName().getAsString()<<":\n Args Type";
	  updatePABasedOnExpr(fsig->params[i],(*it)->IgnoreImplicit());
	}
	if(hasRet)
	  pa->ProcessAssignStmt(retId,fsig->rets[0]);  // C funcion has only one return value
	  
      Decl* decl=call->getCalleeDecl();
      TraverseDecl(decl);
      }
      //errs()<<"Function: "<<fdecl->getNameInfo().getName().getAsString()<<" not processed\n ";	
    }
}
void updatePABasedOnExpr(unsigned id, Expr * exp)
{
  //if(exp->isIntegerConstantExpr(*astContext,NULL)) return true;
  std::pair<unsigned,PtrType> expTypePair=getExprTypeAndId(exp);
  if(expTypePair.second==PTR_L1 || expTypePair.second==PTR_L0)
    { 
      pa->ProcessAssignStmt(id,expTypePair.first);  
      writeVarAccessInfo(exp->getExprLoc(),std::pair<unsigned,AccessType>(expTypePair.first,RD));
    }
  else if(expTypePair.second==ADDR_OF)
    {
      pa->ProcessAssignAddrStmt(id,expTypePair.first);
      writeVarAccessInfo(exp->getExprLoc(),std::pair<unsigned,AccessType>(expTypePair.first,RD));
    }
  else traverse_subExpr(exp);
 
  
  /*  
 if(clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
 {
   clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
   traverse_subExpr(subexp);
   return true;
 }    
     
 if(BinaryOperator *bop=dyn_cast<BinaryOperator>(exp))    
 {
   Expr *lhs=bop->getLHS();
   Expr *rhs=bop->getRHS();
   traverse_subExpr(lhs->IgnoreImplicit());
   traverse_subExpr(rhs->IgnoreImplicit());    
   return true;     
 } 
  */
}

bool traverse_subExpr(Expr * exp)
{
 if(exp->isIntegerConstantExpr(*astContext,NULL)) return true;
 if(clang::DeclRefExpr *dexpr=dyn_cast<clang::DeclRefExpr>(exp))
 {
  if(dyn_cast<VarDecl>(dexpr->getDecl()))
   {
     /* clang::VarDecl *vd=dyn_cast<VarDecl>(dexpr->getDecl());
       if(vd->getType()->isPointerType())
	 errs()<< "Pointer Variables Accessed "<<vd->getQualifiedNameAsString()<<"\n";
       else
	 errs()<< "Variables Accessed "<<vd->getQualifiedNameAsString()<<"\n";
     */ 
    std::pair<unsigned,PtrType> rhs_pair=getExprTypeAndId(exp);
    writeVarAccessInfo(exp->getExprLoc(),std::pair<unsigned,AccessType>(rhs_pair.first,RD));
    return true;
   } 
 }    
   
 if(clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
 {
   clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit();
   traverse_subExpr(subexp);
   return true;
 }    
     
 if(BinaryOperator *bop=dyn_cast<BinaryOperator>(exp))    
 {
   Expr *lhs=bop->getLHS();
   Expr *rhs=bop->getRHS();
   traverse_subExpr(lhs->IgnoreImplicit());
   traverse_subExpr(rhs->IgnoreImplicit());    
   return true;     
 } 
 return false;
}

bool VisitVarDecl(VarDecl *vdecl)
{
  if(!astContext->getSourceManager().isInSystemHeader(vdecl->getLocStart()))
    { 
      if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vdecl)) {	    
	//if(def->hasGlobalStorage())
	//  errs()<<" Variable has Global storage"<<"\n"; 
	//key=pa->CreateNewVar(); 
	if(vdecl->hasLinkage()) 
	  { 
	    unsigned key=_symbTab->lookupId(val);
	    gv.insert(key);
	  }  
	//errs()<<" Variable is Linkage Global"<<"\n";
      }
    }
  return true;
}

};

#endif
