/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "steengaardPAVisitor.h"

void SteengaardPAVisitor::initPA(SymTab<SymBase> *symbTab)
{
  _symbTab=symbTab;
  assert(_symbTab);
  std::set<unsigned> vars;
  std::set<FuncSignature *> funcs;
  pa=new CSteensgaardPA();
  analPointers=false;
  _symbTab->getVarsAndFuncs(vars,funcs);
  pa->initPASolver(vars,funcs);
}  

void SteengaardPAVisitor::buildPASet()
{
  if(!analPointers)
    {
      pa->BuildVarToVarsAndVarToLabelsPointToSets();
      analPointers=true;
    }     
}
void SteengaardPAVisitor::rebuildPASet()
{
  pa->BuildVarToVarsAndVarToLabelsPointToSets();
  pa->BuildVarToFuncsPointToSets();
  analPointers=true;
}

void SteengaardPAVisitor::showPAInfo(bool prIntrls) 
{
  rebuildPASet();
  if(debugLabel>1)
    pa->PrintAsPointsToSets(_symbTab);
  if(debugLabel>2)
    pa->PrintInternals();  
}

void SteengaardPAVisitor::recordVarAccess(clang::SourceLocation l, std::pair<unsigned,AccessType> acc)
{
  typedef std::tuple<unsigned,AccessType,std::string> AccInfoType;
  AccInfoType accInfo=std::make_tuple(acc.first,acc.second,currFuncStartLoc);
  varMod.insert(std::pair<clang::SourceLocation, std::pair<unsigned,AccessType> >(l,acc));
  varAccInfo.insert(std::pair<clang::SourceLocation, AccInfoType>(l,accInfo));
}

void SteengaardPAVisitor::showVarReadWriteLoc()
{
  //std::multimap<clang::SourceLocation,std::pair<unsigned,AccessType> >::iterator it=varMod.begin();
  std::multimap<clang::SourceLocation,std::tuple<unsigned,AccessType,std::string> >::iterator it=varAccInfo.begin();
  for(;it!=varAccInfo.end();it++)
    {
      unsigned varid; 
      AccessType acc;
      std::string funcLoc;
      std::tie(varid,acc,funcLoc)=it->second;
      //std::string var=((_symbTab->lookupSymb(it->second.first))->getVarDecl())->getNameAsString();
      std::string var=((_symbTab->lookupSymb(varid))->getVarDecl())->getNameAsString();
      std::string varAcc= (acc==RD ? "RD":(acc==WR ? "WR": "RW"));
      //std::string varAcc=it->second.second==RD ? "RD":(it->second.second==WR ? "WR": "RW");
      errs()<<" Accessed Var "<<var<<" at "<<it->first.printToString(astContext->getSourceManager())<<" : "<<varAcc<<"\n";   
    }	  
}

void SteengaardPAVisitor::storeGlobalPointers()
{
  buildPASet();
  std::set<unsigned> ptrToGlobals;
  for(SetIter it=gv.gVarsBegin();it!=gv.gVarsEnd();it++)
    { 
      std::set<unsigned> vars_pointed_to; 
      pa->GetPointedAtVar(*it,&vars_pointed_to);
      if(!vars_pointed_to.empty())
	ptrToGlobals.insert(vars_pointed_to.begin(),vars_pointed_to.end());
      //gv.insertPtsToGv(vars_pointed_to,*it);    // check this method
    }
    
  //split pointers according to Read or Write access 
  // ForALL "var" accessed in the code (next msg)
  std::multimap<clang::SourceLocation,std::tuple<unsigned,AccessType,std::string > >::iterator it=varAccInfo.begin();
  for(;it!=varAccInfo.end();it++)
    {
      clang::ValueDecl *val=NULL;
      auto *sym=_symbTab->lookupSymb(std::get<0>(it->second));
      if(sym) val=sym->getVarDecl();
      
      //std::string varAsLoc=val->getLocation()->printToString(astContext->getSourceManager());
      
      // if "var" points to a global variable or itself a global
      if(ptrToGlobals.find(std::get<0>(it->second))!=ptrToGlobals.end() || gv.isGv(std::get<0>(it->second)))
	{ 
	  std::set<unsigned> vars_pointed_to;
	  std::set<unsigned> intsect;
	  
	  // loc is the location where "var" is accessed, and "vCurr" is the variable name
	  std::string loc=it->first.printToString(astContext->getSourceManager());
	  std::string vCurr;
	  if(val) vCurr=val->getNameAsString();
	  else vCurr="You should not get this name";
	  // get all "vars" that "var" points 
	  pa->GetPointsToVars(std::get<0>(it->second),&vars_pointed_to);

	  // if "var" is a global, insert it to "vars"
	  if(gv.isGv(std::get<0>(it->second)))
	    vars_pointed_to.insert(std::get<0>(it->second));

	  // Now take the intersection of "vars" and globals, which are the set of global vars that var points 
	  std::set_intersection(vars_pointed_to.begin(), vars_pointed_to.end(),gv.gVarsBegin(),gv.gVarsEnd(), std::inserter(intsect,intsect.begin()));  

	  if(std::get<1>(it->second)==RD) { 
	    
	    // all globals at insect are read at loc
	    gv.storeGlobalRead(intsect,loc);
	    for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
	      std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
	      // all globals at insect are read at loc through "var"  
	      gv.storeMapInfo(loc,vCurr,vGlobal,std::get<2>(it->second));
	    }   	 
	  }
	  else {
	    gv.storeGlobalWrite(intsect,loc);
	    for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
	      std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
	      gv.storeMapInfo(loc,vCurr,vGlobal,std::get<2>(it->second));
	    }  
	  }
	}     
    }    
}

// void SteengaardPAVisitor::storeGlobalPointers()
// {
//   buildPASet();
//   std::set<unsigned> ptrToGlobals;
//   for(SetIter it=gv.gVarsBegin();it!=gv.gVarsEnd();it++)
//     { 
//       std::set<unsigned> vars_pointed_to; 
//       pa->GetPointedAtVar(*it,&vars_pointed_to);
//       if(!vars_pointed_to.empty())
// 	ptrToGlobals.insert(vars_pointed_to.begin(),vars_pointed_to.end());
//       //gv.insertPtsToGv(vars_pointed_to,*it);    // check this method
//     }
    
//   //split pointers according to Read or Write access 
//   // ForALL "var" accessed in the code (next msg)
//   std::multimap<clang::SourceLocation,std::pair<unsigned,AccessType> >::iterator it=varMod.begin();
//   for(;it!=varMod.end();it++)
//     {
//       clang::ValueDecl *val=NULL;
//       auto *sym=_symbTab->lookupSymb(it->second.first);
//       if(sym) val=sym->getVarDecl();
      
//       //std::string varAsLoc=val->getLocation()->printToString(astContext->getSourceManager());
      
//       // if "var" points to a global variable or itself a global
//       if(ptrToGlobals.find(it->second.first)!=ptrToGlobals.end() || gv.isGv(it->second.first))
// 	{ 
// 	  std::set<unsigned> vars_pointed_to;
// 	  std::set<unsigned> intsect;
	  
// 	  // loc is the location where "var" is accessed, and "vCurr" is the variable name
// 	  std::string loc=it->first.printToString(astContext->getSourceManager());
// 	  std::string vCurr;
// 	  if(val) vCurr=val->getNameAsString();
// 	  else vCurr="You should not get this name";
// 	  // get all "vars" that "var" points 
// 	  pa->GetPointsToVars(it->second.first,&vars_pointed_to);

// 	  // if "var" is a global, insert it to "vars"
// 	  if(gv.isGv(it->second.first))
// 	    vars_pointed_to.insert(it->second.first);

// 	  // Now take the intersection of "vars" and globals, which are the set of global vars that var points 
// 	  std::set_intersection(vars_pointed_to.begin(), vars_pointed_to.end(),gv.gVarsBegin(),gv.gVarsEnd(), std::inserter(intsect,intsect.begin()));  

// 	  if(it->second.second==RD) { 
	    
// 	    // all globals at insect are read at loc
// 	    gv.storeGlobalRead(intsect,loc);
// 	    for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
// 	      std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
// 	      // all globals at insect are read at loc through "var"  
// 	      gv.storeMapInfo(loc,vCurr,vGlobal);
// 	    }   	 
// 	  }
// 	  else {
// 	    gv.storeGlobalWrite(intsect,loc);
// 	    for(std::set<unsigned>::iterator i=intsect.begin();i!=intsect.end();i++){
// 	      std::string vGlobal=((_symbTab->lookupSymb(*i))->getVarDecl())->getNameAsString();
// 	      gv.storeMapInfo(loc,vCurr,vGlobal);
// 	    }  
// 	  }
// 	}     
//     }    
// }


bool SteengaardPAVisitor::VisitFunctionDecl(FunctionDecl *func) {
  if(!astContext) {exit(0);}
  current_fs=_symbTab->lookupfunc(func);
  // currFuncStartLoc=func->getSourceRange().getBegin().printToString(astContext->getSourceManager());
  currFuncStartLoc=func->getNameInfo().getAsString();
  isVisitingFunc=true;
  return true;
}
  
std::pair<unsigned,PtrType> SteengaardPAVisitor::getExprIdandType(clang::Expr *exp)
 {
   unsigned id;
   if(clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(exp))
     {
       if(clang::Expr *subexp=uop->getSubExpr()->IgnoreImplicit())
	 if(clang::DeclRefExpr *dexp = clang::dyn_cast<clang::DeclRefExpr>(subexp))
	   if(clang::ValueDecl *vdec=dyn_cast<clang::ValueDecl>(dexp->getDecl()))
	     {
	       clang::UnaryOperator::Opcode op=uop->getOpcode();
	       
	       //Expr is a function type ref
	       if(vdec->getType().getTypePtr()->isFunctionType())
		 {
		   FunctionDecl *fdecl=vdec->getAsFunction();    
		   id=_symbTab->lookupfunc(fdecl)->fid;
		   return std::pair<unsigned,PtrType>(id,FUNTYPE);
		 }
	       id=_symbTab->lookupId(vdec);
	       
	      //else expr is a variable pointer
	       switch(op){
	       case UO_Deref:  
		 return std::pair<unsigned,PtrType>(id,PTRONE);
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
	   id=_symbTab->lookupId(vdec);
	   
	  //expr is a function pointer declaration
	   if(vdec->getType().getTypePtr()->isFunctionPointerType())
	     return std::pair<unsigned,PtrType>(id,FUNPTR);

	  // Function Type
	   else if(vdec->getType().getTypePtr()->isFunctionType())
	     {
	       FunctionDecl *fdecl=vdec->getAsFunction();    
	       id=_symbTab->lookupfunc(fdecl)->fid;
	       return std::pair<unsigned,PtrType>(id,FUNTYPE);
	     }
	  
	  // expression is pointer type, but without dereference
	   else if(vdec->getType()->isPointerType())
	     return std::pair<unsigned,PtrType>(id,PTRZERO);           
	  // else normal variable
	   else  
	     return std::pair<unsigned,PtrType>(id,NOPTR); 
	 }    
     }   
   return std::pair<unsigned,PtrType>(0,UNDEF);
 }


void SteengaardPAVisitor::updatePAonUnaryExpr(clang::UnaryOperator *uop)
 {
   clang::Expr *exp = uop->getSubExpr()->IgnoreImplicit();
   if(uop->isIncrementDecrementOp())
   if(clang::DeclRefExpr *dref=dyn_cast<clang::DeclRefExpr>(exp))
     {
       std::pair<unsigned,PtrType> E=getExprIdandType(exp);
       recordVarAccess(dref->getLocStart(),std::pair<unsigned,AccessType>(E.first,RD));
       recordVarAccess(dref->getLocEnd(),std::pair<unsigned,AccessType>(E.first,WR));
     }     
 }
    
 void SteengaardPAVisitor::updatePAonBinaryExpr(clang::BinaryOperator *bop)
 {
   clang::BinaryOperator::Opcode opcode=bop->getOpcode();
   if(opcode==clang::BO_Assign)    
     {
       clang::Expr *lhs = bop->getLHS()->IgnoreImplicit();
       clang::Expr *rhs = bop->getRHS()->IgnoreImplicit();
       std::pair<unsigned,PtrType> lhs_pair=getExprIdandType(lhs);
       std::pair<unsigned,PtrType> rhs_pair=getExprIdandType(rhs);
       
       if((lhs_pair.second==PTRZERO) && (rhs_pair.second==PTRZERO))
	 {
	   //errs()<< "Address e1=e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignStmt(lhs_pair.first,rhs_pair.first);  	   
	 }
       else if((lhs_pair.second==PTRZERO) && (rhs_pair.second==ADDR_OF))
	 {
	   //errs()<< "Address e1=&e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignAddrStmt(lhs_pair.first,rhs_pair.first);
	 }
       else if((lhs_pair.second==PTRZERO) && (rhs_pair.second==PTRONE))
	 {
	   //errs()<< "Assignment e1=*e2"<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignFromIndStmt(lhs_pair.first,rhs_pair.first);
	   recordVarAccess(rhs->getExprLoc(),std::pair<unsigned,AccessType>(rhs_pair.first,RD));
	   recordVarAccess(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	 }
       else if((lhs_pair.second==PTRONE) && (rhs_pair.second==PTRZERO))
	 {
	   // errs()<< "Assignment *e1=e2 "<<lhs_pair.first<<" "<<rhs_pair.first;
	   pa->ProcessAssignToIndStmt(lhs_pair.first,rhs_pair.first);
	   recordVarAccess(rhs->getExprLoc(),std::pair<unsigned,AccessType>(rhs_pair.first,RD));
	   recordVarAccess(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	 }
       else if(CallExpr *call = dyn_cast<CallExpr>(rhs)) 
	 {
	   if(lhs_pair.second==PTRONE || lhs_pair.second==PTRZERO)
	     updatePAOnCallExpr(call,lhs_pair.first);
	   else updatePAOnCallExpr(call,-1);
	 }
       else if((lhs_pair.second==FUNPTR) && (rhs_pair.second==FUNTYPE))
	{
	  pa->ProcessAssignFunPtrAddrStmt(lhs_pair.first,rhs_pair.first);
	}
       else
	 { 
	   recordVarAccess(lhs->getExprLoc(),std::pair<unsigned,AccessType>(lhs_pair.first,WR));
	   traverse_subExpr(rhs);
	 } 
     }    
 }

 bool SteengaardPAVisitor::VisitStmt(Stmt *st) 
 {
   // Consider statements like int x or int x=10,y,z
   /*  if (DeclStmt *decl_stmt=dyn_cast<DeclStmt>(st)) {  
     clang::DeclStmt::const_decl_iterator it=decl_stmt->decl_begin();
     // single statement may contain multiple declarations     
     for(;it!=decl_stmt->decl_end();it++)   { 
       if(VarDecl *vardecl=dyn_cast<VarDecl>(*it))    // if it is a variable declaration
	 {
	   VarDecl *def=vardecl->getDefinition();
	   if(def) VisitVarDecl(def);
	 }
     }//forEnd
   } */   
   if (clang::BinaryOperator *bop=dyn_cast<clang::BinaryOperator>(st))
     updatePAonBinaryExpr(bop); 
   else if (clang::UnaryOperator *uop=dyn_cast<clang::UnaryOperator>(st))
     updatePAonUnaryExpr(uop);

   // Update PA for function calls
   // if x=f(Args), this st is processed by both updatePAOnCallExpr and updatePAonBinaryExpr
  else if (CallExpr *call = dyn_cast<CallExpr>(st)) {
    updatePAOnCallExpr(call,-1);
  }

  else if (ReturnStmt *rtst = dyn_cast<ReturnStmt>(st)) {
    if(rtst->getRetValue() && isVisitingFunc){
      Expr *retexpr=rtst->getRetValue()->IgnoreImplicit();
      unsigned id=current_fs->rets[0];
      updatePABasedOnExpr(id,retexpr);
    }
    if(isVisitingFunc) isVisitingFunc=false;
  }  
  else if (IfStmt *ifstmt = dyn_cast<IfStmt>(st)) {
    Expr *ifexp=ifstmt->getCond();
    if(ifexp) traverse_subExpr(ifexp->IgnoreImplicit());
  }  
  else if (ForStmt *forstmt = dyn_cast<ForStmt>(st)) 
  {
    Expr *forexp=forstmt->getCond();
    if(forexp) traverse_subExpr(forexp->IgnoreImplicit());
  }
  else if(WhileStmt *whilestmt = dyn_cast<WhileStmt>(st)) 
  {
    Expr *whileexp=whilestmt->getCond();
    if(whileexp) traverse_subExpr(whileexp->IgnoreImplicit());
  }
   return true;
 }


 std::set<FunctionDecl *> SteengaardPAVisitor::getCallsFromFuncPtr(CallExpr *call)
 {
   ArrayRef<Stmt*> children=call->getRawSubExprs();
   ArrayRef<Stmt*>::iterator I=children.begin();
   std::set<FunctionDecl *> funcCalls;
   Stmt *S=(*I)->IgnoreImplicit();
   if(clang::DeclRefExpr *ref = clang::dyn_cast<clang::DeclRefExpr>(S))
     if(clang::ValueDecl *vdec=dyn_cast<clang::ValueDecl>(ref->getDecl()))
       {
	 unsigned id=_symbTab->lookupId(vdec);
	 if(vdec->getType().getTypePtr()->isFunctionPointerType()){
	   //errs()<<"Function Pointer: "<<id<<" Name :"<<vdec->getNameAsString();
	   std::set<unsigned> pts2func=pa->getPtsToFuncsWithPartialPA(id);
	   for(std::set<unsigned>::iterator it=pts2func.begin();it!=pts2func.end();it++)
	     {
	       FunctionDecl * f=(_symbTab->lookupSymb(*it))->getFuncDecl();
	       std::string fname=f->getNameInfo().getName().getAsString();
	       //errs()<<"points to: "<<fname<<"\n";
	       funcCalls.insert(f);
	     }
	 }
       }
   return funcCalls;
 }


void SteengaardPAVisitor::updatePAOnFuncCall(FunctionDecl *calleeDecl,std::set<Expr*> ActualInArgs,long ActualOutArg)
{
  FuncSignature * fsig=NULL;
  fsig=_symbTab->lookupfunc(calleeDecl);       
  if(fsig){
    int i=0;
    for(std::set<Expr*>::iterator it=ActualInArgs.begin();it!=ActualInArgs.end();it++,i++)       
      updatePABasedOnExpr(fsig->params[i],(*it)->IgnoreImplicit());
    if(ActualOutArg>=0)
       pa->ProcessAssignStmt(ActualOutArg,fsig->rets[0]);  // C funcion has only one return value
  }
}

void SteengaardPAVisitor::updatePAOnCallExpr(CallExpr *call,long ActOutArg)
{
  std::set<Expr*> ActualInArgs;
  for(clang::CallExpr::arg_iterator it=call->arg_begin();it!=call->arg_end();it++)
    {
      ActualInArgs.insert(*it);
    }
  FunctionDecl *callee=call->getDirectCallee();
  if(!callee)
    {
      std::set<FunctionDecl *> fCalls;
      fCalls=getCallsFromFuncPtr(call);
      std::set<FunctionDecl *>::iterator I=fCalls.begin(),E=fCalls.end();
      for(;I!=E;I++)
	updatePAOnFuncCall(*I,ActualInArgs,ActOutArg); 	  
    }
  else updatePAOnFuncCall(callee,ActualInArgs,ActOutArg);    
}

void SteengaardPAVisitor::updatePABasedOnExpr(unsigned id, Expr * exp)
{
  std::pair<unsigned,PtrType> expTypePair=getExprIdandType(exp);
  if(expTypePair.second==PTRONE || expTypePair.second==PTRZERO)
    { 
      pa->ProcessAssignStmt(id,expTypePair.first);  
      recordVarAccess(exp->getExprLoc(),std::pair<unsigned,AccessType>(expTypePair.first,RD));
    }
  else if(expTypePair.second==ADDR_OF)
    {
      pa->ProcessAssignAddrStmt(id,expTypePair.first);
      recordVarAccess(exp->getExprLoc(),std::pair<unsigned,AccessType>(expTypePair.first,RD));
    }
  else traverse_subExpr(exp);
}

bool SteengaardPAVisitor::traverse_subExpr(Expr * exp)
{ 
  assert(exp!=NULL);
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
	  std::pair<unsigned,PtrType> E=getExprIdandType(exp);
	  recordVarAccess(exp->getExprLoc(),std::pair<unsigned,AccessType>(E.first,RD));
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

// Store Global Variable Information
bool SteengaardPAVisitor::VisitVarDecl(VarDecl *vDecl)
{
  if(!astContext->getSourceManager().isInSystemHeader(vDecl->getLocStart())) 
    {
      if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vDecl)) {
	Expr * exp=const_cast<clang::Expr *>(vDecl->getAnyInitializer());
	unsigned key=_symbTab->lookupId(val);
	if(vDecl->hasLinkage())   
	  gv.insert(key, val->getNameAsString());  
	if(exp){
	  traverse_subExpr(exp->IgnoreImplicit());
	  recordVarAccess(vDecl->getSourceRange().getBegin(),std::pair<unsigned,AccessType>(key,WR));
	}
      }
    }
  return true;
}
 
