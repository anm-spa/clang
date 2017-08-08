#ifndef SYMTAB_H_
#define SYMTAB_H_

#include<map>
#include<set>
#include<vector>
#include "clang/AST/Decl.h"

using namespace clang;
using namespace llvm;
using namespace std;
template <class T> class symTab;

// Types of variables related to ptr variable declared in the code.
// PTRONE refers to *v and PTRZERO refers v (e.g.)
typedef enum {PTRONE,PTRZERO,ADDR_OF,FUNPTR,FUNTYPE,NOPTR,UNDEF} PtrType;

// Symbol type
typedef enum {VAR,EXPR, CALLEXPR,FUNCDECL,ARGVAR, RET, NAI} IdentType;    // NAI: Not An Identifier


// Function signature includes the function id, and any formal parameters and return id.
//class fsignature
class FuncSignature
{
public:
FuncSignature(unsigned id,std::vector<unsigned> &args, std::vector<unsigned> &rturn)
{
  fid=id;
  params=args;
  rets=rturn;
}
~FuncSignature(){}
 unsigned fid;
 std::vector<unsigned> params;
 std::vector<unsigned> rets;
};


class SymBase
{
 public: 
 SymBase(): _id(static_cast<unsigned int>(-1)){}
 SymBase(unsigned id): _id(id){}   
 virtual ~SymBase(){}   
  unsigned getId() const {return _id;}
  void setId(unsigned id){_id=id;}
  virtual void dump(){}
  virtual clang::ValueDecl * getVarDecl() const {return NULL;}
  virtual clang::FunctionDecl *getFuncDecl() const {return NULL;}
  virtual FuncSignature *getFuncSig() const {return NULL;}
  virtual FuncSignature * buildSign(){return NULL;}
  virtual IdentType getType(){return NAI;}
  std::string ptrTypeToStr(PtrType ptr)
    {
      switch(ptr)
	{
	case PTRONE:  return "PTR L1";
	case PTRZERO:  return "PTR L0";
	case ADDR_OF: return "ADDR OF";
	case NOPTR:   return "NOPTR";
	case UNDEF:   return "UNDEF";  
	default: return "UNDEF";  
	}
    }
  
 private:
  unsigned _id;
};

class SymVarCxtClang: public SymBase
{
public:
SymVarCxtClang(): SymBase(){_var=NULL; _subtype=UNDEF;}

SymVarCxtClang(clang::ValueDecl *var, PtrType ptr,std::string func):SymBase()
{ _var=var; _subtype=ptr; scope=func;}

void dump(){ 
  errs()<<"\t\t"<<_var->getNameAsString()<< " ("<< scope <<")"; 
  errs()<<"\t\t VAR \t\t"<<ptrTypeToStr(_subtype)<<"\n";
}
IdentType getType(){return VAR;}
clang::ValueDecl * getVarDecl() const {return _var;}
clang::FunctionDecl *getFuncDecl() const {return NULL;}
FuncSignature *getFuncSig() const {return NULL;}

private:
  clang::ValueDecl * _var;
  PtrType _subtype;
  std::string scope;
};

class SymExprCxtClang: public SymBase
{
public:
 SymExprCxtClang(): SymBase(){ _exp=NULL;}
 SymExprCxtClang(clang::Expr *exp): SymBase(){_exp=exp;}
 SymExprCxtClang(unsigned id, clang::Expr *exp): SymBase(id){_exp=exp;}
 void dump(){ errs()<<"Expr Not Dump"; errs()<<"\t EXPR \t No\n";} 
 IdentType getType(){return EXPR;}
 virtual clang::ValueDecl * getVarDecl() const {return NULL;}
 clang::FunctionDecl *getFuncDecl() const {return NULL;}
 FuncSignature *getFuncSig() const {return NULL;}
private:
 clang::Expr * _exp;
};


class SymFuncRetCxtClang: public SymBase
{
public:
 SymFuncRetCxtClang(std::string func): SymBase(){scope=func;}
  void dump(){ 
      errs()<<" \t\tRet "<< "("<< scope <<")"; 
      errs()<<"\t\t TEMPVAR \t No"<<"\n";
  } 
  IdentType getType(){return RET;}
  virtual clang::ValueDecl * getVarDecl() const {return NULL;}
  clang::FunctionDecl *getFuncDecl() const {return NULL;}
  FuncSignature *getFuncSig() const {return NULL;}
private:
  std::string scope;
};

class SymArgVarCxtClang: public SymBase
{
public:
 SymArgVarCxtClang(clang::ValueDecl *val, PtrType ptr, unsigned fid, std::string func): SymBase(){
   _arg=val; _subtype=ptr;_funid=fid;scope=func;
}

void dump(){ 
  errs()<<"\t\t"<<_arg->getNameAsString()<< " ("<< scope <<")"; 
  errs()<<"\t\t FuncArgVar \t "<<ptrTypeToStr(_subtype)<<"\n";
} 
IdentType getType(){return ARGVAR;}
virtual clang::ValueDecl * getVarDecl() const {return _arg;}
clang::FunctionDecl *getFuncDecl() const {return NULL;}
FuncSignature *getFuncSig() const {return NULL;}
private:
  clang::ValueDecl * _arg;
  unsigned _funid;
  PtrType _subtype;
  std::string scope;
};

class SymFuncDeclCxtClang: public SymBase
{
public:
 SymFuncDeclCxtClang(): SymBase(){ _func=NULL;}
 SymFuncDeclCxtClang(clang::FunctionDecl *fun): SymBase(){_func=fun;}
 ~SymFuncDeclCxtClang(){delete _fsig;}
 void insertArg(SymArgVarCxtClang *arg){_params.push_back(arg);} 
 void insertRet(SymFuncRetCxtClang *ret){_rets.push_back(ret);} 
 void dump()
 { 
   errs()<<"\t\t"<<_func->getNameInfo().getAsString(); errs()<<"\t\t FUNC DECL \t X\n";
 }
 IdentType getType(){return FUNCDECL;}
 virtual clang::ValueDecl * getVarDecl() const {return NULL;}
 clang::FunctionDecl *getFuncDecl() const {return _func;}
 FuncSignature *getFuncSig() const {return _fsig;}
 FuncSignature * buildSign(){
 std::vector<unsigned> args,rets;
 for(auto it=_params.begin();it!=_params.end();it++)
   args.push_back((*it)->getId());
       
 for(auto it=_rets.begin();it!=_rets.end();it++)
   rets.push_back((*it)->getId());

 _fsig=new FuncSignature(getId(),args,rets); 
   return _fsig;
 }
 
private:
  clang::FunctionDecl * _func;
  FuncSignature * _fsig;
  std::vector<SymArgVarCxtClang *> _params;
  std::vector<SymFuncRetCxtClang *> _rets;
};

template <class T>
class SymTab
{
public:
  SymTab(){}
  virtual ~SymTab(){};

  void dump(unsigned key)
  {
    if(valid(key))
      _tab[key]->dump();
  }
  unsigned addSymb(T *symb)
  {
    unsigned key=_tab.size();
    _tab.resize(key+1);
    _tab[key] = symb;
    _map.insert(std::pair<T*,unsigned>(symb,key));  
    if(symb->getType()==VAR|| symb->getType()==ARGVAR)
      _varmap.insert(std::pair<clang::ValueDecl *,unsigned>(symb->getVarDecl(),key));
    
    if(symb->getType()==FUNCDECL)
      _funcmap.insert(std::pair<clang::FunctionDecl *,unsigned>(symb->getFuncDecl(),key));
    symb->setId(key);
    return key;
  }

  T *lookupSymb(unsigned key) const
  {
   if(valid(key))
     return _tab[key];
   else return NULL;
  }


  long lookupId(clang::ValueDecl *varDecl) const
  {
    auto it=_varmap.find(varDecl);
    if(it!=_varmap.end())
      return (it->second);
    else return -1;
  }

  FuncSignature *lookupfunc(clang::FunctionDecl *funDecl) const
  {
    auto it=_funcmap.find(funDecl);
    if(it!=_funcmap.end())
      return _tab[it->second]->getFuncSig();
    else return NULL;
  }  

  long lookupId(T *symb) const
  {
    auto it=_map.find(symb);
    if(it!=_map.end())
      return (it->second);
    else return -1;
  }

  unsigned size(){return _tab.size();}

  void getVarsAndFuncs(std::set<unsigned> &vars, std::set<FuncSignature *> &funcs) const
  {
    for(auto it=_tab.begin();it!=_tab.end();it++)
    {
      if((*it)->getType()==VAR || (*it)->getType()==ARGVAR || (*it)->getType()==RET)
	vars.insert((*it)->getId());
      if((*it)->getType()==FUNCDECL)
	{
         funcs.insert((*it)->buildSign());
        }
    }  
  }
  
  void dump()
  {
    errs()<< "-------------------Symbol Table Information----------------------\n";
    errs()<< "| Id |           Symbol (Scope)      |     Type     | SubType      |\n"; 
    errs()<< "-----------------------------------------------------------------\n";
    for(unsigned long i=0; i< _tab.size();i++)
    {
      errs()<<"| "<< i<<"  ";
      _tab[i]->dump(); 

    }
    errs()<< "-----------------------------------------------------------------\n";
  }
  

protected:
  bool valid(unsigned key) const{ return key>=0 && key <_tab.size();}

private:
  std::map<T *,unsigned> _map;
  std::map<clang::ValueDecl *,unsigned> _varmap;
  std::map<clang::FunctionDecl *,unsigned> _funcmap;
  std::vector<T *> _tab;
};


#endif
