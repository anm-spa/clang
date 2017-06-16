// $Id $

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#include <cassert>
#include <memory>
#include <typeinfo>

// -------------------------------------------------------
// Definition of types:
//   32-bit signed integer:   int32_t
//   32-bit unsigned integer: uint32_t
//   etc...
// Many lower and upper range values will be represented as
// 64bit integers, making sure that all integer values always
// can fit in the limits of 64 bit integer intervals.
// -------------------------------------------------------
// -------------------------------------------------------
// Other useful macros
// -------------------------------------------------------
#ifndef FORALL
#define FORALL(i,L) for ((i) = (L).begin(); (i) != (L).end(); (i)++)
#endif

#ifndef PRINTLIST
#define PRINTLIST(the_class,the_list) { \
list<the_class> & print_list = the_list; \
list<the_class>::iterator the_iterator; \
 \
        if (print_list.size() > 0) { \
    std::cout << "Print the_list" << std::endl; \
    for (the_iterator = print_list.begin(); the_iterator != print_list.end(); ++the_iterator) \
      std::cout << *the_iterator << " " << std::endl; \
      } \
    } \
  }
#endif

#ifndef DYN_CAST
#define DYN_CAST(expr,new_expr,new_type,ref_or_star) try {        \
                new_expr = dynamic_cast<new_type ref_or_star> (expr);        \
                }        \
        catch (exception &) {        \
                std::cerr << __FILE__ << ":" << __LINE__ << ":error in dynamic_cast" << std::endl; \
    exit(EXIT_FAILURE);         \
                } \
  assert(new_expr);
#endif

// This is basically DYN_CAST as a templated function instead of a macro
template <typename TTo, typename TFrom>
inline TTo * DynCast(TFrom * p)
{
   // Dereference the pointer in order to get a reference, try to dynamically cast the reference, and
   // then return the address of the referenced value (if the cast does not work, an exception will be thrown)
   return &dynamic_cast<TTo &>(*p);
}

template <typename TTo, typename TFrom>
inline std::auto_ptr<TTo> DynCast(std::auto_ptr<TFrom> p)
{
   // Release the pointer, dereference it in order to get a reference, try to dynamically cast the reference, and
   // then use the address of the referenced value to create a new auto_ptr and return it (if the cast does not
   // work, an exception will be thrown)
   return std::auto_ptr<TTo>(&dynamic_cast<TTo &>(*p.release()));
}

class Deleter
{
public:
   template <typename T> void operator()(const T* ptr) const {delete ptr;}
};


// Debug trace for functions
#if defined(PARMA_INSTALLED) && defined(PARMA_DEBUG)
#define ENTERF(x) for(int __v__=0; __v__<=DBG_CNT_SBE; __v__++) cout << " "; cout << "Enter: " << x << "[" << DBG_CNT_SBE << "]" << std::endl; DBG_CNT_SBE++;
#define EXITF(x) DBG_CNT_SBE--; for(int __v__=0; __v__<=DBG_CNT_SBE; __v__++) cout << " "; cout << "Exit: " << x << "[" << DBG_CNT_SBE << "]" << std::endl;
#else
#define ENTERF(x)
#define EXITF(x)
#endif

#endif // MACROS_H_INCLUDED
