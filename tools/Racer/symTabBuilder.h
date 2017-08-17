/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "symtab.h"
using namespace llvm;

class SymTabBuilderVisitor : public RecursiveASTVisitor<SymTabBuilderVisitor> {
private:
    ASTContext *astContext; // used for getting additional AST info
    FunctionDecl *current_f=NULL;
    SymTab<SymBase> *symbTab=new SymTab<SymBase>();
    int debugLabel;
public:
 void dumpSymTab(){
   if(debugLabel>1)
   symbTab->dump(); 
 }  
 explicit SymTabBuilderVisitor(CompilerInstance *CI, int dl) 
   : astContext(&(CI->getASTContext())), debugLabel(dl) 
   {}
 
 SymTab<SymBase> *getSymTab()
 {
   return symbTab;
 } 
 
 bool VisitFunctionDecl(FunctionDecl *func) 
 {
   
   // if(!astContext->getSourceManager().isInSystemHeader(func->getLocStart()))
     {
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
	     if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vd)) {	    
	       if(!vd->getType()->isPointerType()){
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
     }
   return true;
 }
    
 bool VisitVarDecl(VarDecl *vdecl)
 {
   if(!vdecl->isLocalVarDecl() && !vdecl->hasLinkage()) {return true;}
   if(!astContext->getSourceManager().isInSystemHeader(vdecl->getLocStart()))
     { 
       if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vdecl)) {
	 std::string fname;
	 if(current_f) fname=current_f->getNameInfo().getAsString();
	 else fname="Global";
	 // Debug Info
	 //errs()<<"Context :"<<fname<<" Ref: "<<val->getNameAsString()<<" Type: ";
	 //errs()<< val->getType().getAsString()<<"\n";
	 /*if(val->getType().getTypePtr()->isFunctionPointerType())
	 {

	 } */ 
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
            
      /*  if(vdecl->hasLinkage()) 
	  globals.insert(key);  */
       }
     }
   return true;
 }
  
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
