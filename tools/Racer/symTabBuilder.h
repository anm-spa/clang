#include "symtab.h"
using namespace llvm;

class symTabBuilderVisitor : public RecursiveASTVisitor<symTabBuilderVisitor> {
private:
    ASTContext *astContext; // used for getting additional AST info
    FunctionDecl *current_f=NULL;
    symTab<symIdentBase> *symbTab=new symTab<symIdentBase>();

public:
 void dumpSymTab(){
   symbTab->dump(); 
 }  
 explicit symTabBuilderVisitor(CompilerInstance *CI) 
   : astContext(&(CI->getASTContext())) // initialize private members
   {}
 
 symTab<symIdentBase> *getSymTab()
 {
   return symbTab;
 } 
  
 bool VisitFunctionDecl(FunctionDecl *func) 
 {
   if(!astContext->getSourceManager().isInSystemHeader(func->getLocStart()))
     {
       current_f=func;
       std::string fname;
       if(current_f) fname=current_f->getNameInfo().getAsString();
       else fname="Global";
       symIdentFuncDeclCxtClang *ident=new symIdentFuncDeclCxtClang(func);
       symbTab->addSymb(ident);
       unsigned id=ident->getId();
       for(unsigned int i=0; i<func->getNumParams(); i++)
	 {
	   clang::ParmVarDecl *p=func->parameters()[i];
	   if(clang::VarDecl *vd=dyn_cast<clang::VarDecl>(p))
	     if(clang::ValueDecl *val=dyn_cast<clang::ValueDecl>(vd)) {	    
	       if(!vd->getType()->isPointerType()){
		 symIdentArgVarCxtClang *arg=new symIdentArgVarCxtClang(val,NOPTR,id,fname);
		 symbTab->addSymb(arg);
		 ident->insertArg(arg);
	       } 
	       else{
		 symIdentArgVarCxtClang *arg=new symIdentArgVarCxtClang(val,PTR_L1,id,fname);
		 ident->insertArg(arg);
		 symbTab->addSymb(arg);
	       }   
	     }
	 }
       if(!func->getReturnType().getTypePtr()->isVoidType())
       {
	 symIdentFuncRetCxtClang *ret=new symIdentFuncRetCxtClang(fname);
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
	 errs()<<"Context :"<<fname<<" Ref: "<<val->getNameAsString()<<" Type: ";
	 errs()<< val->getType().getAsString()<<"\n";
	 if(!vdecl->getType()->isPointerType()){
	   symIdentVarCxtClang *ident=new symIdentVarCxtClang(val,NOPTR,fname);
	   symbTab->addSymb(ident);
	   //symbTab->dump(0);	
	 }	
	 else
	   { 
	     symIdentVarCxtClang *ident=new symIdentVarCxtClang(val,PTR_L1,fname);
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


class symTabBuilder : public ASTConsumer{
private:
  symTabBuilderVisitor *visitor; // doesn't have to be private  
public:
    // override the constructor in order to pass CI
  explicit symTabBuilder(CompilerInstance *CI)
  : visitor(new symTabBuilderVisitor(CI)) // initialize the visitor
    {}

  virtual void HandleTranslationUnit(ASTContext &Context) {
    visitor->TraverseDecl(Context.getTranslationUnitDecl());
    visitor->dumpSymTab();
  }   
    
};
