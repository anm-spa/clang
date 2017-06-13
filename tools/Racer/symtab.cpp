#include "symtab.h"

/*
template <class T>
unsigned symTab<T>::addSymb(T *symb)
{
  unsigned key=_tab.size();
  _tab.resize(key+1);
  _tab[key] = symb;
  _map.insert(std::pair<T,unsigned>(symb,key));  
  symb->setId(key);
  return key;
  }*/

/*
template <class T>
unsigned symTab<T>::lookupId(T *symb)
{
  auto it=_map.find(symb);
  if(it!=_map.end())
    return (it->second).getId();
  else return NULL;
}
*/

/*
template <class T>
void symTab<T>::dump()
{
  errs()<< "-------------------Symbol Table Information-------------------\n";
  errs()<< "| Id |           Symbol      | Type | SubType   |\n"; 
  for(int i=0; i< _tab.size();i++)
  {
    errs()<< i;
    _tab[i]->dump(); 
    errs()<<"\n";
  }
  errs()<< "--------------------------------------------------------------\n";
}
*/


//template<> symIdentBase lookupSymb<symIdentBase>(unsigned key);



/*

The implementation of a non-specialized template must be visible to a translation unit that uses it.

The compiler must be able to see the implementation in order to generate code for all specializations in your code.

This can be achieved in three ways:

1) Move the implementation inside the header.

2) If you want to keep it separate, move it into a different header which you include in your original header:

3) Manually instantiate convert2QString with int in util.cpp and define this specialization as extern function in util.h


*/
