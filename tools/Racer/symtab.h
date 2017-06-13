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

typedef enum {PTR_L1,PTR_L0,ADDR_OF,NOPTR,UNDEF} PtrType;

typedef enum {VAR,EXPR, CALLEXPR,FUNCDECL,ARGVAR, RET, NAI} IdentType;    // NAI: Not An Identifier

class fsignature
{
public:
 fsignature(unsigned id,std::vector<unsigned> &args, std::vector<unsigned> &rturn)
 {
  fid=id;
  params=args;
  rets=rturn;
 }
~fsignature(){}
 unsigned fid;
 std::vector<unsigned> params;
 std::vector<unsigned> rets;
};

class symIdentBase
{
public:
//template<class T> friend class symTab; 
 symIdentBase(): _id(static_cast<unsigned int>(-1)){}
 symIdentBase(unsigned id): _id(id){}
 ~symIdentBase(){}   
 unsigned getId() const {return _id;}
//virtual std::string getName(){return "Base Class has no name";}
 void setId(unsigned id){_id=id;}
 virtual void dump(){}
 virtual clang::ValueDecl * getVarDecl() const {return NULL;}
 virtual clang::FunctionDecl *getFuncDecl() const {return NULL;}
 virtual fsignature *getFuncSig() const {return NULL;}
 virtual fsignature * buildSign(){return NULL;}
 virtual IdentType getType(){return NAI;}
 std::string ptrTypeToStr(PtrType ptr)
 {
  switch(ptr)
  {
    case PTR_L1:  return "PTR L1";
    case PTR_L0:  return "PTR L0";
    case ADDR_OF: return "ADDR OF";
    case NOPTR:   return "NOPTR";
    case UNDEF:   return "UNDEF";  
    default: return "UNDEF";  
   }
 }

private:
 unsigned _id;
};

class symIdentVarCxtClang: public symIdentBase
{
public:
// template <class T> friend class symTab; 
 symIdentVarCxtClang(): symIdentBase(){_var=NULL; _subtype=UNDEF;}
/* symIdentVarCxtClang(clang::ValueDecl *var, PtrType ptr):symIdentBase()
  { _var=var; _subtype=ptr; }
*/
 symIdentVarCxtClang(clang::ValueDecl *var, PtrType ptr,std::string func):symIdentBase()
{ _var=var; _subtype=ptr; scope=func;}

/* symIdentVarCxtClang(unsigned id, clang::ValueDecl *var, PtrType ptr):symIdentBase(id)
  { _var=var; _subtype=ptr; }
*/
  void dump(){ 
     errs()<<"\t\t"<<_var->getNameAsString()<< " ("<< scope <<")"; 
     errs()<<"\t\t VAR \t\t"<<ptrTypeToStr(_subtype)<<"\n";
  }
  IdentType getType(){return VAR;}
//std::string getName(){return "Base Class has no name";}
  clang::ValueDecl * getVarDecl() const {return _var;}
  clang::FunctionDecl *getFuncDecl() const {return NULL;}
  fsignature *getFuncSig() const {return NULL;}
private:
  clang::ValueDecl * _var;
  PtrType _subtype;
  std::string scope;
};

class symIdentExprCxtClang: public symIdentBase
{
public:
 symIdentExprCxtClang(): symIdentBase(){ _exp=NULL;}
 symIdentExprCxtClang(clang::Expr *exp): symIdentBase(){_exp=exp;}
symIdentExprCxtClang(unsigned id, clang::Expr *exp): symIdentBase(id){_exp=exp;}
void dump(){ errs()<<"Expr Not Dump"; errs()<<"\t EXPR \t No\n";} 
IdentType getType(){return EXPR;}
virtual clang::ValueDecl * getVarDecl() const {return NULL;}
clang::FunctionDecl *getFuncDecl() const {return NULL;}
fsignature *getFuncSig() const {return NULL;}
private:
  clang::Expr * _exp;
};


class symIdentFuncRetCxtClang: public symIdentBase
{
public:
//symIdentFuncRetCxtClang(): symIdentBase(){}
symIdentFuncRetCxtClang(std::string func): symIdentBase(){scope=func;}
  void dump(){ 
      errs()<<" \t\tRet "<< "("<< scope <<")"; 
      errs()<<"\t\t TEMPVAR \t No"<<"\n";} 
  IdentType getType(){return RET;}
  virtual clang::ValueDecl * getVarDecl() const {return NULL;}
  clang::FunctionDecl *getFuncDecl() const {return NULL;}
  fsignature *getFuncSig() const {return NULL;}
private:
  std::string scope;
};

class symIdentArgVarCxtClang: public symIdentBase
{
public:
/*symIdentArgVarCxtClang(): symIdentBase(){ _arg=NULL;_subtype=UNDEF;}
 symIdentArgVarCxtClang(clang::ValueDecl *val, PtrType ptr): symIdentBase(){_arg=val,_subtype=ptr;}
 symIdentArgVarCxtClang(clang::ValueDecl *val, PtrType ptr, unsigned fid): symIdentBase(){_arg=val; _subtype=ptr;_funid=fid;}*/
 symIdentArgVarCxtClang(clang::ValueDecl *val, PtrType ptr, unsigned fid, std::string func): symIdentBase(){_arg=val; _subtype=ptr;_funid=fid;scope=func;}

  void dump(){ 
    errs()<<"\t\t"<<_arg->getNameAsString()<< " ("<< scope <<")"; 
    errs()<<"\t\t FuncArgVar \t "<<ptrTypeToStr(_subtype)<<"\n";
  } 
IdentType getType(){return ARGVAR;}
virtual clang::ValueDecl * getVarDecl() const {return _arg;}
clang::FunctionDecl *getFuncDecl() const {return NULL;}
fsignature *getFuncSig() const {return NULL;}
private:
  clang::ValueDecl * _arg;
  unsigned _funid;
  PtrType _subtype;
  std::string scope;
};

class symIdentFuncDeclCxtClang: public symIdentBase
{
public:
 symIdentFuncDeclCxtClang(): symIdentBase(){ _func=NULL;}
 symIdentFuncDeclCxtClang(clang::FunctionDecl *fun): symIdentBase(){_func=fun;}
  //symIdentFuncDeclCxtClang(unsigned id, clang::FunctionDecl *fun): symIdentBase(id){_func=fun;}
  ~symIdentFuncDeclCxtClang(){delete _fsig;}
  void insertArg(symIdentArgVarCxtClang *arg){_params.push_back(arg);} 
  void insertRet(symIdentFuncRetCxtClang *ret){_rets.push_back(ret);} 
  void dump()
  { 
    errs()<<"\t\t"<<_func->getNameInfo().getAsString(); errs()<<"\t\t FUNC DECL \t X\n";
    if(_params.size()>0){
      //errs()<<"\t\t ---- Function Parameters----\n";
      /*for(auto it=_params.begin();it!=_params.end();it++)
      {
	(*it)->dump();
	//errs()<<"\n";
	}*/
    }
  }
  IdentType getType(){return FUNCDECL;}
  virtual clang::ValueDecl * getVarDecl() const {return NULL;}
  clang::FunctionDecl *getFuncDecl() const {return _func;}
  fsignature *getFuncSig() const {return _fsig;}
  fsignature * buildSign(){
    std::vector<unsigned> args,rets;
    for(auto it=_params.begin();it!=_params.end();it++)
	args.push_back((*it)->getId());
       
    for(auto it=_rets.begin();it!=_rets.end();it++)
      rets.push_back((*it)->getId());

    _fsig=new fsignature(getId(),args,rets); 
    return _fsig;
  } 
private:
  clang::FunctionDecl * _func;
  fsignature * _fsig;
  std::vector<symIdentArgVarCxtClang *> _params;
  std::vector<symIdentFuncRetCxtClang *> _rets;
};

template <class T>
class symTab
{
public:
  symTab(){}
  virtual ~symTab(){};

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

  fsignature *lookupfunc(clang::FunctionDecl *funDecl) const
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

  void getVarsAndFuncs(std::set<unsigned> &vars, std::set<fsignature *> &funcs) const
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
    for(int i=0; i< _tab.size();i++)
    {
      errs()<<"| "<< i<<"  ";
      _tab[i]->dump(); 
      //errs()<<"\n";
    }
    errs()<< "-----------------------------------------------------------------\n";
  }
  
  // extern template symIdentBase lookupSymb<symIdentBase>(unsigned key);
protected:
  bool valid(unsigned key) const{ return key>=0 && key <_tab.size();}

private:
  std::map<T *,unsigned> _map;
  std::map<clang::ValueDecl *,unsigned> _varmap;
  std::map<clang::FunctionDecl *,unsigned> _funcmap;
  std::vector<T *> _tab;
};


#endif
