/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "symtab.h"

std::string SymBase::ptrTypeToStr(PtrType ptr)
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

void SymVarCxtClang::dump()
{ 
  errs()<<"\t\t"<<_var->getNameAsString()<< " ("<< scope <<")"; 
  errs()<<"\t\t VAR \t\t"<<ptrTypeToStr(_subtype)<<"\n";
}
  
FuncSignature *SymFuncDeclCxtClang::buildSign()
{
  std::vector<unsigned> args,rets;
  for(auto it=_params.begin();it!=_params.end();it++)
    args.push_back((*it)->getId());   
  for(auto it=_rets.begin();it!=_rets.end();it++)
    rets.push_back((*it)->getId());
  _fsig=new FuncSignature(getId(),args,rets); 
  return _fsig;
 }
