/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "symTabBuilder.h"

void SymTabBuilderVisitor::dumpSymTab(){
if(debugLabel>1)
  symbTab->dump(); 
}  
 
bool SymTabBuilderVisitor::VisitFunctionDecl(FunctionDecl *func) 
{
  // if(!astContext->getSourceManager().isInSystemHeader(func->getLocStart()))
  current_f=func;
  std::string fname;
  if(current_f) fname=current_f->getNameInfo().getAsString();
  else fname="Global";
  SymFuncDeclCxtClang *ident=new SymFuncDeclCxtClang(func);
  symbTab->addSymb(ident);
  unsigned id=ident->getId();
  for(unsigned int i=0; i<func->getNumParams(); i++)
  {
    clang::ParmVarDecl *p=func->parameters()[i];
    if(clang::VarDecl *vd=dyn_cast<clang::VarDecl>(p))
    if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vd))
    {	    
      if(!vd->getType()->isPointerType())
       {
        SymArgVarCxtClang *arg=new SymArgVarCxtClang(val,NOPTR,id,fname);
        symbTab->addSymb(arg);
        ident->insertArg(arg);
       } 
      else{
        SymArgVarCxtClang *arg=new SymArgVarCxtClang(val,PTRONE,id,fname);
        ident->insertArg(arg);
        symbTab->addSymb(arg);
       }   
    }
  }
  if(!func->getReturnType().getTypePtr()->isVoidType())
  {
     SymFuncRetCxtClang *ret=new SymFuncRetCxtClang(fname);
     symbTab->addSymb(ret);
     ident->insertRet(ret);
   } 
 return true;
}
    
bool SymTabBuilderVisitor::VisitVarDecl(VarDecl *vdecl)
{
  if(!vdecl->isLocalVarDecl() && !vdecl->hasLinkage()) {return true;}
  if(!astContext->getSourceManager().isInSystemHeader(vdecl->getLocStart()))
    { 
      if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vdecl)) {
	std::string fname;
	if(current_f) fname=current_f->getNameInfo().getAsString();
	else fname="Global";
	if(!vdecl->getType()->isPointerType()){
	  SymVarCxtClang *ident=new SymVarCxtClang(val,NOPTR,fname);
	  symbTab->addSymb(ident);
	  //symbTab->dump(0);	
	}	
	else
	  { 
	    SymVarCxtClang *ident=new SymVarCxtClang(val,PTRONE,fname);
	    symbTab->addSymb(ident);
	    //symbTab->dump(0);
	  }   
      }
    }
  return true;
}
