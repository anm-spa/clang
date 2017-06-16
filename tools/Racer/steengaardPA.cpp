#include "steengaardPA.h"
//#include "CSteensgaardAnalysisBuilder.h"
//#include "symtab/CSymTabBase.h"
#include "macros.h"
#include <ciso646>
#include <stdexcept>
#include <stdlib.h>
#include "llvm/Support/Casting.h"
using namespace std;

CSteensgaardPA::
CSteensgaardPA()
{
   // Do the pointer analysis
  // PerformPA(builder, ast, symtab);

   // Prepare the points-to-sets to be used by the interface functions.
   //BuildVarToVarsAndVarToLabelsPointToSets();
   //BuildVarToFuncsPointToSets();
}

/*
CSteensgaardPA::
CSteensgaardPA(auto_ptr <CSteensgaardAnalysisBuilder> builder, CGenericProgram *ast, const CSymTabBase *symtab)
  : _symtab(symtab)
{
   // Do the pointer analysis
   PerformPA(builder, ast, symtab);

   // Prepare the points-to-sets to be used by the interface functions.
   BuildVarToVarsAndVarToLabelsPointToSets();
   BuildVarToFuncsPointToSets();
}

CSteensgaardPA::
CSteensgaardPA(auto_ptr <CSteensgaardAnalysisBuilder> builder, CGenericProgram *ast, const CSymTabBase *symtab,
               const set<pair<int,set<int> > > *def_addresses_pairs)
  : _symtab(symtab)
{
   // Do the pointer analysis
   PerformPA(builder, ast, symtab);

   // Go through all the extra def addresses pairs and add it to the pointer analysis
   for(set<pair<int,set<int> > >::const_iterator def_addresses = def_addresses_pairs->begin();
       def_addresses != def_addresses_pairs->end(); def_addresses++)
   {
      int def = def_addresses->first;
      const set<int>& addresses = def_addresses->second;
      // Add def := &y to PA analysis
      for (set<int>::const_iterator address = addresses.begin(); address != addresses.end(); address++) {
         unsigned rhs_key = *address;
         if (symtab->Lookup(rhs_key)->IsFunction()) {
            // To handle function pointers correctly
            ProcessAssignStmt(def, rhs_key);
         } else {
            // To handle other types of pointers correctly
            ProcessAssignAddrStmt(def, rhs_key);
         }
      }
   }
   // Prepare the points-to-sets to be used by the interface functions.
   BuildVarToVarsAndVarToLabelsPointToSets();
   BuildVarToFuncsPointToSets();
} */


void
CSteensgaardPA::
initPASolver(std::set<unsigned> &vars, std::set<FuncSignature *> &funcs)
{
  unsigned key_size;
  _vars.insert(vars.begin(), vars.end());
  key_size=vars.size()+funcs.size();
  for(auto it=funcs.begin();it!=funcs.end();++it)
    _funcs.insert((*it)->fid);

  // Keep treack of all vars and funcs
  _vars_and_funcs.insert(_vars.begin(), _vars.end());
  _vars_and_funcs.insert(_funcs.begin(), _funcs.end());

  // Keep treack of all vars except function vars
  _vars_labels_and_temp_vars.insert(_vars.begin(), _vars.end());
  //   _vars_labels_and_temp_vars.insert(_temp_vars.begin(), _temp_vars.end());
  // _vars_labels_and_temp_vars.insert(_labels.begin(), _labels.end());

  _vars_labels_temp_vars_and_funcs.insert(_vars_labels_and_temp_vars.begin(), _vars_labels_and_temp_vars.end());
  _vars_labels_temp_vars_and_funcs.insert(_funcs.begin(), _funcs.end());

  unsigned current_key = key_size; // The lowest unused key
  unsigned nr_of_extra_bots = _vars_labels_and_temp_vars.size()*2 + _funcs.size();
  unsigned nr_of_extra_lambdas = _funcs.size();
  unsigned total_number = current_key + nr_of_extra_bots + nr_of_extra_lambdas;
  for(unsigned i = 0; i < total_number; i++)
    _type_vector.push_back(new CSteensgaardPABotType());

  _pending_vector.resize(total_number);

   // Create mappings v -> ref(i1,i2), i1 -> BOT and i2 -> BOT for
   // each variable, temp variable or label v
   for(set<unsigned>::iterator v=_vars_labels_and_temp_vars.begin(); v!=_vars_labels_and_temp_vars.end(); ++v) {
      // Create two new BOT types
      CSteensgaardPABotType * new_b1_t = new CSteensgaardPABotType();
      CSteensgaardPABotType * new_b2_t = new CSteensgaardPABotType();

      // Create indexes for them
      int b1_index = current_key++;
      int b2_index = current_key++;

      // Store them in new index places
      settype(b1_index, new_b1_t);
      settype(b2_index, new_b2_t);

      // Create a new ref tuple
      CSteensgaardPARefType * new_ref_t = new CSteensgaardPARefType(b1_index, b2_index);

      // Insert the new ref type at number corresponding to the gkey
      settype(*v, new_ref_t);
   }

   // Create mappings f -> ref(i1,i2), i1 -> BOT and i2 -> lambda(..)(..) for
   // each variable v
   for (std::set<FuncSignature *>::iterator funcit=funcs.begin(); funcit!=funcs.end(); ++funcit) {
      // Get the func and its actual parameters and
     int func_key = (*funcit)->fid;
     vector<unsigned> f_args = (*funcit)->params;
     vector<unsigned> f_rets = (*funcit)->rets;

      // Create a new BOT type
      CSteensgaardPABotType * new_b = new CSteensgaardPABotType();

      // Create a new lambda type
      CSteensgaardPALambdaType * new_l = new CSteensgaardPALambdaType(f_args, f_rets);

      // Create indexes for them
      int b_index = current_key++;
      int l_index = current_key++;

      // Store them in type vector
      settype(b_index, new_b);
      settype(l_index, new_l);

      // Create a new ref tuple
      CSteensgaardPARefType * new_ref_t = new CSteensgaardPARefType(b_index, l_index);
      
      // Insert the new ref type at number corresponding to the gkey
      settype(func_key, new_ref_t);
   }

}

 /*
void
CSteensgaardPA::
PerformPA()
{
   CSteensgaardAnalysisBuilder::lambda_t funcs;
   unsigned key_size = 0;
   builder->GetVarsLabelsAndFuncs(ast, symtab, _vars, _labels, funcs, _temp_vars, key_size);
   for (CSteensgaardAnalysisBuilder::lambda_t::iterator funcit=funcs.begin(); funcit!=funcs.end(); ++funcit)
      _funcs.insert(funcit->first);

   // Keep treack of all vars except function vars
   _vars_labels_and_temp_vars.insert(_vars.begin(), _vars.end());
   _vars_labels_and_temp_vars.insert(_temp_vars.begin(), _temp_vars.end());
   _vars_labels_and_temp_vars.insert(_labels.begin(), _labels.end());

   // Keep treack of all vars and funcs
   _vars_and_funcs.insert(_vars.begin(), _vars.end());
   _vars_and_funcs.insert(_funcs.begin(), _funcs.end());

   _vars_labels_temp_vars_and_funcs.insert(_vars_labels_and_temp_vars.begin(), _vars_labels_and_temp_vars.end());
   _vars_labels_temp_vars_and_funcs.insert(_funcs.begin(), _funcs.end());

   unsigned current_key = key_size; // The lowest unused key
   unsigned nr_of_extra_bots = _vars_labels_and_temp_vars.size()*2 + funcs.size();
   unsigned nr_of_extra_lambdas = funcs.size();
   unsigned total_number = current_key + nr_of_extra_bots + nr_of_extra_lambdas;
   for(unsigned i = 0; i < total_number; i++)
      _type_vector.push_back(new CSteensgaardPABotType());

   _pending_vector.resize(total_number);

   // Create mappings v -> ref(i1,i2), i1 -> BOT and i2 -> BOT for
   // each variable, temp variable or label v
   for (set<unsigned>::iterator v=_vars_labels_and_temp_vars.begin(); v!=_vars_labels_and_temp_vars.end(); ++v) {
      // Create two new BOT types
      CSteensgaardPABotType * new_b1_t = new CSteensgaardPABotType();
      CSteensgaardPABotType * new_b2_t = new CSteensgaardPABotType();

      // Create indexes for them
      int b1_index = current_key++;
      int b2_index = current_key++;

      // Store them in new index places
      settype(b1_index, new_b1_t);
      settype(b2_index, new_b2_t);

      // Create a new ref tuple
      CSteensgaardPARefType * new_ref_t = new CSteensgaardPARefType(b1_index, b2_index);

      // Insert the new ref type at number corresponding to the gkey
      settype(*v, new_ref_t);
   }

   // Create mappings f -> ref(i1,i2), i1 -> BOT and i2 -> lambda(..)(..) for
   // each variable v
   for (CSteensgaardAnalysisBuilder::lambda_t::iterator funcit=funcs.begin(); funcit!=funcs.end(); ++funcit) {
      // Get the func and its actual parameters and
      int func_key = funcit->first;
      vector<unsigned> f_args = funcit->second.first;
      vector<unsigned> f_rets = funcit->second.second;

      // Create a new BOT type
      CSteensgaardPABotType * new_b = new CSteensgaardPABotType();

      // Create a new lambda type
      CSteensgaardPALambdaType * new_l = new CSteensgaardPALambdaType(f_args, f_rets);

      // Create indexes for them
      int b_index = current_key++;
      int l_index = current_key++;

      // Store them in type vector
      settype(b_index, new_b);
      settype(l_index, new_l);

      // Create a new ref tuple
      CSteensgaardPARefType * new_ref_t = new CSteensgaardPARefType(b_index, l_index);

      // Insert the new ref type at number corresponding to the gkey
      settype(func_key, new_ref_t);
   }

   // The builder will call back to the various "Process*" functions to extend this analysis
   // with information while examining the statements in the code.
   builder->TraverseCode(ast, this, funcs);

}*/

// For deleting the analysis data structures
CSteensgaardPA::
~CSteensgaardPA(void)
{
   vector< CSteensgaardPAType * >::iterator t;
   FORALL(t, _type_vector)
      delete *t;
}

// -------------------------------------------------------
// -------------------------------------------------------
// Functions for processing statements. Corresponds to the
// functions given in the Steensgard paper.
// -------------------------------------------------------
// -------------------------------------------------------

// Handle stmts of type e1 := e2
void
CSteensgaardPA::
ProcessAssignStmt(int e1, int e2)
{
   // Dereference the variables
   e1 = deref(e1);
   e2 = deref(e2);
   //std::cout<< "e1="<<e1<<" e2="<<e2<<"\n";
   // Get the types of the variables
   CSteensgaardPAType * t1 = gettype(e1);
   CSteensgaardPAType * t2 = gettype(e2);

   // Downcast the types to ref types
   CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
   CSteensgaardPARefType * ref_t2 = static_cast<CSteensgaardPARefType *>(t2);
   assert(ref_t1 && ref_t2);

   // Get the integers representing the vars and the funcs
   int var1 = deref(ref_t1->Var());
   int func1 = deref(ref_t1->Func());
   int var2 = deref(ref_t2->Var());
   int func2 = deref(ref_t2->Func());
   //std::cout<< "var1="<<var1<<" var2="<<var2<<"\n";
   // Conditionally join them if they are not equal
   if(var1 != var2) {
      cjoin(var1, var2);
   }

   if(func1 != func2) {
      cjoin(func1, func2);
   }
}

// Handle stmts of type e1 := &e2
void
CSteensgaardPA::
ProcessAssignAddrStmt(int e1, int e2)
{
   // Dereference the e1 and e2 variables
   e1 = deref(e1);
   e2 = deref(e2);

   // Get the type of the e1 variable
   CSteensgaardPAType * t1 = gettype(e1);
   //t1->Print(&std::cout);    
     // Downcast the type to a ref type
   CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
   assert(ref_t1);
   //ref_t1->Print(&std::cout);
   // Get the integer representing the var
   int var1 = deref(ref_t1->Var());
   //cout<<"e1 ="<<e1<<" e2="<<e2<<"Ref t1 "<<var1<<"\n";
   // Do a join if not equal
   if(var1 != e2) {
     // std::cout<<"addr join\n";
      join(var1, e2);
   }
}

void
CSteensgaardPA::
ProcessAssignFunPtrAddrStmt(int e1, int e2)
{
   // Dereference the e1 and e2 variables
   e1 = deref(e1);
   e2 = deref(e2);

   // Get the type of the e1 variable
   CSteensgaardPAType * t1 = gettype(e1);
   //t1->Print(&std::cout);    
     // Downcast the type to a ref type
   CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
   assert(ref_t1);
   //ref_t1->Print(&std::cout);
   // Get the integer representing the var
   int func1 = deref(ref_t1->Func());
   //cout<<"e1 ="<<e1<<" e2="<<e2<<"Ref t1 "<<func1<<"\n";
   // Do a join if not equal
   if(func1 != e2) {
     // std::cout<<"addr join\n";
      joinWithoutUnification(func1, e2);
   }
}


// Handle stmts of type e1 := *e2
void
CSteensgaardPA::
ProcessAssignFromIndStmt(int e1, int e2)
{
   // Dereference the variables
   e1 = deref(e1);
   e2 = deref(e2);

   // Get the types of the variables
   CSteensgaardPAType * t1 = gettype(e1);
   CSteensgaardPAType * t2 = gettype(e2);

   // Downcast the types to ref types
   CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
   CSteensgaardPARefType * ref_t2 = static_cast<CSteensgaardPARefType *>(t2);
   assert(ref_t1 && ref_t2);

   // Get the integers representing the vars and the funcs
   int var1 = deref(ref_t1->Var());
   int func1 = deref(ref_t1->Func());
   int var2 = deref(ref_t2->Var());

   // Check if the second variable is bot
   if(gettype(var2)->IsBot()) {
      settype(var2, ref_t1->Copy());
   } else {
      // Get the type of the var2 variable
      CSteensgaardPAType * t3 = gettype(var2);
      assert(t3->IsRef());

      // Downcast the types to a ref type
      CSteensgaardPARefType * ref_t3 = static_cast<CSteensgaardPARefType *>(t3);

      // Get the integers representing the vars and the funcs
      int var3 = deref(ref_t3->Var());
      int func3 = deref(ref_t3->Func());

      // Do conditional join if not equal
      if(var1 != var3) {
         cjoin(var1, var3);
      }
      if(func1 != func3) {
         cjoin(func1, func3);
      }
   }
}

// Handle stmts of type *x := y
void
CSteensgaardPA::
ProcessAssignToIndStmt(int e1, int e2)
{
   // Dereference the variables
   e1 = deref(e1);
   e2 = deref(e2);

   // Get the types of the variables
   CSteensgaardPAType * t1 = gettype(e1);
   CSteensgaardPAType * t2 = gettype(e2);

   // Downcast the types to ref types
   CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
   CSteensgaardPARefType * ref_t2 = static_cast<CSteensgaardPARefType *>(t2);
   assert(ref_t1 && ref_t2);

   // Get the integers representing the vars and the funcs
   int var1 = deref(ref_t1->Var());
   int var2 = deref(ref_t2->Var());
   int func2 = deref(ref_t2->Func());

   // Check if the first variable is bot
   if(gettype(var1)->IsBot()) {
      settype(var1, ref_t2->Copy());
   } else {
      // Get the type of the var1 variable
      CSteensgaardPAType * t3 = gettype(var1);

      // Downcast the types to a ref type
      CSteensgaardPARefType * ref_t3 = static_cast<CSteensgaardPARefType *>(t3);
      assert(ref_t3);

      // Get the integers representing the vars and the funcs
      int var3 = deref(ref_t3->Var());
      int func3 = deref(ref_t3->Func());

      // Do conditional join if not equal
      if(var3 != var2) {
         cjoin(var3, var2);
      }

      if(func3 != func2) {
         cjoin(func3, func2);
      }
   }
}

// Handle stmts of type x = alloc(...)
void
CSteensgaardPA::
ProcessAssignAllocStmt(int x)
{
   // Dereference the variables
   x = deref(x);

   // Get the type of the variable
   CSteensgaardPAType * t = gettype(x);

   // Downcast the type to ref types
   CSteensgaardPARefType * ref_t = static_cast<CSteensgaardPARefType *>(t);
   assert(ref_t);

   // Get the integers representing the vars and the funcs
   int var = deref(ref_t->Var());

   // Get the current size of the type vector (where to start inserting elements)
   int t_vector_size = _type_vector.size();

   if(gettype(var)->IsBot()) {
      // Create two new BOT types
      CSteensgaardPABotType * new_b1_t = new CSteensgaardPABotType();
      CSteensgaardPABotType * new_b2_t = new CSteensgaardPABotType();

      // Create indexes for them
      int b1_index = t_vector_size;
      t_vector_size++;
      int b2_index = t_vector_size;

      // Store them in new index places
      _type_vector[b1_index] = new_b1_t;
      _type_vector[b2_index] = new_b2_t;

      // Create a new ref tuple
      CSteensgaardPARefType * new_t = new CSteensgaardPARefType(b1_index, b2_index);
      settype(var, new_t);
   }
}

// Handle call stmts of type ret1 .. retm = f(arg1, arg2, ... argn) (i.e.,
// function calls) where p is a function with a number of rets (actual
// return values) and a number of args (actual argument values).
void
CSteensgaardPA::
ProcessAssignCallStmt(const vector<unsigned> &a_rets, int p, const vector<unsigned> &a_args)
{
   // Get the type of the procedure variable
   p = deref(p);
   CSteensgaardPAType * p_t = gettype(p);

   // Downcast the type to ref types
   CSteensgaardPARefType * ref_p_t = static_cast<CSteensgaardPARefType *>(p_t);
   assert(ref_p_t);

   // Get the lambda part of the things pointed at
   int lam = ref_p_t->Func();
   lam = deref(lam);

   // Create a lambda expression to be associated with p
   auto_ptr<CSteensgaardPALambdaType> new_lam_t(new CSteensgaardPALambdaType(a_args, a_rets));

   // Check if the function is bot
   if(gettype(lam)->IsBot()) {
      // Yes, associate the new lambda expression to func
      settype(lam, new_lam_t.release());
   } else {
      CSteensgaardPAType * lam_t = gettype(lam);
      assert(lam_t->IsLambda());
      // Unify the old and new lambda expressions
      cunify(lam_t, new_lam_t.get());
   }
}

// Function for creating a new ref. Will create two new bot
// types. Needed when we have expressions which consists of several
// expressions itself.
int
CSteensgaardPA::
CreateNewVar(void)
{
   // Create two new BOT types and their indexes
   CSteensgaardPABotType * new_b1_t = new CSteensgaardPABotType();
   CSteensgaardPABotType * new_b2_t = new CSteensgaardPABotType();
   int b1_index = _type_vector.size() + 1;
   int b2_index = _type_vector.size() + 2;

   // Create a new ref tuple
   CSteensgaardPARefType * new_ref_t = new CSteensgaardPARefType(b1_index, b2_index);
   int new_ref_index = _type_vector.size();

   _vars.insert(new_ref_index);
   // Store it in the vector
   _type_vector.push_back(new_ref_t);
   _type_vector.push_back(new_b1_t);
   _type_vector.push_back(new_b2_t);

   assert(gettype(new_ref_index)->IsRef());
   assert(gettype(b1_index)->IsBot());
   assert(gettype(b2_index)->IsBot());

   // Create new empty sets for the pending vector
   _pending_vector.resize(_pending_vector.size() + 3);
   assert(_pending_vector.size() == _type_vector.size());


   // ADDED FROM HERE: Masud
   //std::cout<<"Variable "<<new_ref_index;
   //new_ref_t->Print(&std::cout); 
   // Keep treack of all vars except function vars
   _vars_labels_and_temp_vars.insert(new_ref_index);
   

   // Keep treack of all vars and funcs
   _vars_and_funcs.insert(new_ref_index);
     _vars_labels_temp_vars_and_funcs.insert(new_ref_index);
   _vars_labels_temp_vars_and_funcs.insert(new_ref_index);

   

      // Store them in new index places
      settype(b1_index, new_b1_t);
      settype(b2_index, new_b2_t);
   
      // Insert the new ref type at number corresponding to the gkey
      settype(new_ref_index, new_ref_t);
   
   return new_ref_index;
}

void
CSteensgaardPA::
PrintInternals(ostream &o) const
{
   o << "\n****** Types ******\n";

   int i = 0;
   for (vector< CSteensgaardPAType * >::const_iterator t=_type_vector.begin(); t!=_type_vector.end(); ++t) {
      o << i << " -> " << *(*t) << endl;
      i++;
   }
   o << endl;

   o << "****** Pending ******\n";
   i = 0;
   for (vector< set < unsigned > > ::const_iterator ps=_pending_vector.begin(); ps!=_pending_vector.end(); ++ps) {
      o << i << " -> { ";
      set<unsigned>::const_iterator v;
      FORALL(v, *ps) {
         o << *v << " ";
      }
      i++;
      o << "}" << endl;
   }
   o << endl;
}

const set<unsigned> &
CSteensgaardPA::
GetPointsToVarsSet(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_vars.find(id);
   if (pts_it != _var_to_vars.end()) {
      return pts_it->second;
   } else {
      static const set<unsigned> empty_set;
      return empty_set;
   }
}

void
CSteensgaardPA::
GetPointsToVars(unsigned id, set<unsigned> * vars_pointed_to) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_vars.find(id);
   if (pts_it != _var_to_vars.end()) {
      for(set< unsigned >::const_iterator v = pts_it->second.begin();
          v != pts_it->second.end(); ++v) {
            vars_pointed_to->insert(*v);
      }
   }
}
void CSteensgaardPA::GetPointedAtVar(unsigned id, std::set<unsigned> * vars_pointed_at) const
{
  map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_vars.begin();
  for(;pts_it!= _var_to_vars.end();pts_it++)
    {
      set< unsigned >::const_iterator v=pts_it->second.find(id);
      if(v!=pts_it->second.end())
	vars_pointed_at->insert(pts_it->first);
               
    }
}

bool
CSteensgaardPA::
HasPointsToVars(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_vars.find(id);
   if (pts_it != _var_to_vars.end()) {
      if(pts_it->second.size() > 0)
         return true;
      else
         return false;
   }
   else {
      return false;
   }
}

const set<unsigned> &
CSteensgaardPA::
GetPointsToLabelsSet(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_labels.find(id);
   if (pts_it != _var_to_labels.end()) {
      return pts_it->second;
   } else {
      static const set<unsigned> empty_set;
      return empty_set;
   }
}

void
CSteensgaardPA::
GetPointsToLabels(unsigned id, set<unsigned> * labels_pointed_to) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_labels.find(id);
   if (pts_it != _var_to_labels.end()) {
      for(set< unsigned >::const_iterator v = pts_it->second.begin();
          v != pts_it->second.end(); ++v) {
            labels_pointed_to->insert(*v);
      }
   }
}

bool
CSteensgaardPA::
HasPointsToLabels(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_labels.find(id);
   if (pts_it != _var_to_labels.end()) {
      if(pts_it->second.size() > 0)
         return true;
      else
         return false;
   }
   else {
      return false;
   }
}

const std::set<unsigned> &
CSteensgaardPA::
GetPointsToFuncsSet(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_funcs.find(id);
   if (pts_it != _var_to_funcs.end()) {
      return pts_it->second;
   } else {
      static const set<unsigned> empty_set;
      return empty_set;
   }
}

void
CSteensgaardPA::
GetPointsToFuncs(unsigned id, set<unsigned> * funcs_pointed_to) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_funcs.find(id);
   if (pts_it != _var_to_funcs.end()) {
      for(set< unsigned >::const_iterator v = pts_it->second.begin();
          v != pts_it->second.end(); ++v) {
            funcs_pointed_to->insert(*v);
      }
   }
}

bool
CSteensgaardPA::
HasPointsToFuncs(unsigned id) const
{
   map< unsigned, set< unsigned > >::const_iterator pts_it = _var_to_funcs.find(id);
   if (pts_it != _var_to_funcs.end()) {
      if(pts_it->second.size() > 0)
         return true;
      else
         return false;
   }
   else {
      return false;
   }
}

void
CSteensgaardPA::
BuildVarToVarsAndVarToLabelsPointToSets()
{
   // Help map holding var to vars and var to labels point to sets
   std::map< unsigned, std::set< unsigned > > var_to_vars_and_labels;

   // A vector mapping from the dereffed var and all
   // variables which goes to the dereffed var through its forward
   // references
   vector <set < unsigned > > deref_to_refs(_type_vector.size());
   //std::cout<<"type vector"<<_type_vector.size();
   // Join all variables with the same dereferenced variable in a set.
   // The set is indexed with the dereferenced variable.
   set<unsigned>::iterator v;
   FORALL(v, _vars_labels_and_temp_vars) {
      // Get the corresponding dereferenced variable
      unsigned d_v = deref(*v);
      if (d_v >= deref_to_refs.size())
         deref_to_refs.resize(d_v+1);
      deref_to_refs[d_v].insert(*v);
      // std::cout<<d_v<<" points to "<<*v<<"\n";
   }

   // Loop through all program variables to store the set of
   // variables they point at
   FORALL(v, _vars) {
      // Get the corresponding dereferenced variable
      int d_v = deref(*v);

      // Get the type of the variable (it must be a ref)
      CSteensgaardPARefType * ref_t = static_cast<CSteensgaardPARefType *>(gettype(d_v));
      assert(ref_t);

      // Get the variable in the ref type
      int ref_v = ref_t->Var();
      // Get the corresponding dereferenced variable
      int d_ref_v = deref(ref_v);

      // Get the set of vars pointed at
      // Insert the connection in one of the sets to return

      // Check if the variable pointed at was a variable or a label
      var_to_vars_and_labels[*v] = deref_to_refs[d_ref_v];
      //std::cout<<ref_v<<" points to "<<d_ref_v<<"\n";
   }

   // Go through the map and extract the vars and labels pointed to
   // into two separate maps
   std::map< unsigned, std::set< unsigned > >::iterator v2vl;
   FORALL(v2vl, var_to_vars_and_labels) {
      unsigned v = (*v2vl).first;
      std::set< unsigned >::iterator vl;
      FORALL(vl, (*v2vl).second) {
         // Check if the key pointed to is a variable or a label and
         // insert the key in the correct map.
         if(_vars.find(*vl) != _vars.end()) {
            // Create a vars set to the var if no such set existed
            if(_var_to_vars.find(v) == _var_to_vars.end())
               _var_to_vars[v] = std::set< unsigned >();
            _var_to_vars[v].insert(*vl);
         }
         else {
            // Create a labels set to the var if no such set existed
            if(_var_to_labels.find(v) == _var_to_labels.end())
               _var_to_labels[v] = std::set< unsigned >();
            _var_to_labels[v].insert(*vl);
         }
      }
   }
}

std::set<unsigned> CSteensgaardPA::getPtsToFuncsWithPartialPA(unsigned var)
{
  set < unsigned >  lambda_refs;
  int d_v = deref(var);
    
  // Get the type of the variable (it must be a ref)
  CSteensgaardPAType * t = gettype(d_v);
    
    // Downcast the type to ref types
  CSteensgaardPARefType * ref_t = static_cast<CSteensgaardPARefType *>(t);
  assert(ref_t);

    // Get the variable in the ref type
  unsigned lam = ref_t->Func();

    // Get the corresponding dereferenced variable
  unsigned d_lam = deref(lam);

  if(!gettype(d_lam)->IsBot())  // function pointer points to function
    {
      CSteensgaardPAType * d_lamt = gettype(d_lam);
      CSteensgaardPARefType * d_lamref_t = static_cast<CSteensgaardPARefType *>(d_lamt);
      assert(d_lamref_t);
	 
      unsigned lam2=d_lamref_t->Func();
      unsigned lam_pointsto=deref(lam2);
      if(gettype(lam_pointsto)->IsLambda()) {
	lambda_refs.insert(d_lam);
	set<unsigned>::iterator v;
	FORALL(v, _funcs) {
	  unsigned dv = deref(*v);
	  if(dv==d_lam && *v!=d_lam) lambda_refs.insert(*v);
	}  
      }        
    } 
  return lambda_refs;
}


void
CSteensgaardPA::
BuildVarToFuncsPointToSets()
{
   // Create a vector keeping mapping from the dereffed var and all
   // variables which goes to the dereffed var through its forward
   // references
   map< unsigned, set < unsigned > > lambda_to_refs;

   // Loop through all program variables to get their pointed to lambda
   // expression (if any) and insert the variable in the lambda
   // expressions set.
   set<unsigned>::iterator v;
   FORALL(v, _vars_and_funcs) {
      // Get the correspondoing dereferenced variable
      int d_v = deref(*v);

      // Get the type of the variable (it must be a ref)
      CSteensgaardPAType * t = gettype(d_v);

      // Downcast the type to ref types
      CSteensgaardPARefType * ref_t = static_cast<CSteensgaardPARefType *>(t);
      assert(ref_t);

      // Get the variable in the ref type
      int lam = ref_t->Func();

      // Get the corresponding dereferenced variable
      int d_lam = deref(lam);

      //cout<<"Var "<<*v<<" deref "<<d_v<<" lam "<<lam<<" d_lam "<<d_lam<<"\n";
      // Check if it is a lambda var
      if(gettype(d_lam)->IsLambda()) {   //direct function
         // Yes, insert the var in the lambda expressions set
	//cout<<*v<<" points to "<<d_lam<<"\n";
         lambda_to_refs[d_lam].insert(*v);
      }
      else if(!gettype(d_lam)->IsBot())  // function pointer points to function
	{
	  CSteensgaardPAType * d_lamt = gettype(d_lam);
	  CSteensgaardPARefType * d_lamref_t = static_cast<CSteensgaardPARefType *>(d_lamt);
	  assert(d_lamref_t);
	 
	  int lam2=d_lamref_t->Func();
	 
	  int lam_pointsto=deref(lam2);
	 	   // Check if it is a lambda var
	   if(gettype(lam_pointsto)->IsLambda()) {
         // Yes, insert the var in the lambda expressions set
	     //   cout<<*v<<" points to "<<lam_pointsto<<"\n";
	     lambda_to_refs[lam_pointsto].insert(*v);
	   }        
	} 
   }
   // Go through all created maps, setting all func refs to point to
   // each other. However, we skip the possibility that functions can
   // point to vars or other functions. Also, we skip that function
   // pointer vars can point to themselves.
   for(unsigned int l=0; l < _type_vector.size(); l++) {
      // Skip all non-lambda vars
      if(gettype(l)->IsLambda()) {
         // Get the set of functions and vars that goes to each other
         set<unsigned> rs = lambda_to_refs[l];
         set<unsigned>::iterator r;
         FORALL(r, rs) {
            // Only associate function alias sets to variables, i.e. not to
            // labels, temp variables or to to the functions themselves.
            if((_funcs.find(*r) == _funcs.end()) && (_vars.find(*r) != _vars.end())) {
               // r is a function pointer variable. Create a set from rs
               // holding all functions and variables except r.
               set<unsigned> rs_not_incl_r;
               set<unsigned>::iterator t;
               FORALL(t, rs) {
                  // Skip inserting t in the set if it equals r (the
                  // current function pointer variable). Also, make sure that
                  // it refers to a function.
                  if((*t != *r) && (_funcs.find(*t) != _funcs.end()))
		    {
                     rs_not_incl_r.insert(*t);
		     //cout<<*r<<" points to "<<*t<<"\n";
		    }
               }
               _var_to_funcs[*r] = rs_not_incl_r;
	      }
         }
      }
   }
}

/************************* convert CSymTabBase to ValueDecl***************************************/

void
CSteensgaardPA::
PrintMapToSet(const map<unsigned, set<unsigned> > &x, ostream &o, const SymTab<SymBase> *st) const
{
   for (map<unsigned, set<unsigned> >::const_iterator it=x.begin(); it!=x.end(); ++it) {
     if (st){
       if(ValueDecl *val= (st->lookupSymb(it->first))->getVarDecl()) 
	 o << val->getNameAsString().c_str();
       else if(FunctionDecl *fd=st->lookupSymb(it->first)->getFuncDecl())
	 o<< fd->getNameInfo().getName().getAsString();
       else o << "TVAR "<<it->first;   // update later
     }
	 else
         o << it->first;
      o << " -> { ";
      const set<unsigned> &vars = it->second;
      for (set<unsigned>::const_iterator target_it=vars.begin(); target_it!=vars.end(); ++target_it) {
        if (st){
	  ValueDecl *val= (st->lookupSymb(*target_it))->getVarDecl();     
          if(val) o << val->getNameAsString().c_str();
	  else if(FunctionDecl *fd=st->lookupSymb(*target_it)->getFuncDecl())
	    o<< fd->getNameInfo().getName().getAsString();
	  else
	    o << "TVAR "<<*target_it;  //update later
	}    
	    else
            o << *target_it;
         o << " ";
      }
      o << "}" << endl;
   }
}  



void
CSteensgaardPA::
PrintAsPointsToSets(const SymTab<SymBase> *st, ostream &o) const
{
   o << "\n****** Data ******\n";
   PrintMapToSet(_var_to_vars, o, st);
   o << "\n****** Functions ******\n";
   PrintMapToSet(_var_to_funcs, o, st);
   o << "\n****** Labels ******\n";
   PrintMapToSet(_var_to_labels, o, st);
}

void
CSteensgaardPA::
PrintAsPointsToSets(ostream &o) const
{
   o << "\n****** Data ******\n";
   PrintMapToSet(_var_to_vars, o);
   o << "\n****** Functions ******\n";
   PrintMapToSet(_var_to_funcs, o);
   o << "\n****** Labels ******\n";
   PrintMapToSet(_var_to_labels, o);
}

// Follow all forward references until we find a ref(x,y),
// lambda(args)(rets) or bot type. Will at the same time also shortcut
// all forward reference chains longer than 1.
int
CSteensgaardPA::
deref(int var)
{
   // most common case: no dereferencing needed
   CSteensgaardPAType *t = _type_vector[var]; // get the type associated with the variable
   if (!t->IsForw())
      return var;

   //std::cout<<"\n!!!Forward Reference!!!"<<var<<" !!!\n";
   // second most common case: only one dereferencing needed
   CSteensgaardPAForwType *forw_t = static_cast<CSteensgaardPAForwType *>(t);
   var = forw_t->ForwVar(); // dereference
   t = _type_vector[var];
   if (!t->IsForw())
      return var;
   // now run the general handling
   vector<CSteensgaardPAForwType*> shortcuts; // to keep forward references that should be shortcutted
   shortcuts.push_back(forw_t);
   do {
      // invariant: 'forw_t' is a forward reference to 't', which is a forward reference as well
      // invariant: forw_t == shortcuts.back()
      forw_t = static_cast<CSteensgaardPAForwType *>(t);
      shortcuts.push_back(forw_t);
      var = forw_t->ForwVar(); // dereference
      t = _type_vector[var];
   } while (t->IsForw());
   shortcuts.pop_back(); // the last forward reference does not need to be shortcutted
   // Fix all variable references that should be shortcutted
   vector<CSteensgaardPAForwType*>::iterator v;
   FORALL(v, shortcuts) {
      // Reset the old forward reference to a new value
      (*v)->SetForwVar(var);
   }
   return var;
}

// To get the type of a certain variable
CSteensgaardPAType *
CSteensgaardPA::
gettype(int var)
{
   // Get the type associated with the variable
   CSteensgaardPAType * t = _type_vector[var];
   return t;
}

// To set the type of the argument variable. Will delete the
// previous type assocaited with the variable.
void
CSteensgaardPA::
settype(int e, CSteensgaardPAType * t)
{
   // Delete the previous type
   CSteensgaardPAType * org_t = _type_vector[e];
   assert(org_t);
   //  std::cout<<"Previous type ";
   //org_t->Print(&std::cout);
   if(org_t == t) return;
   delete org_t;
   //std::cout<<"type "<<e<<"replaced by ";
   //t->Print(&std::cout);
 
   // Set the new type
   _type_vector[e] = t;

   // for x in pending(e) do join(e,x)
   set<unsigned> ps = _pending_vector[e]; // note: deliberate deep copy
   set<unsigned>::const_iterator x;
   FORALL(x, ps) {
      join(e, *x);
   }
}

// Conditional join between two variables. If the second one
// is bot the first one will be inserted into its pending set.
void
CSteensgaardPA::
cjoin(int e1, int e2)
{
   if(gettype(e2)->IsBot()) {
     //std::cout<<"Pending\n";
      _pending_vector[e2].insert(e1);
   } else {
     //std::cout<<"joining\n";
      join(e1, e2);
   }
}

// Unconditional join. Somewhat rewritten compared to code in the paper.
void
CSteensgaardPA::
join(int e1, int e2)
{
   // To get the variables their forward references ending at
   e1 = deref(e1);
   e2 = deref(e2);

   // To avoid unnneccessary work
   if(e1 == e2) return;

   // Get if e1 or e2 is bot
   bool t1_is_bot = gettype(e1)->IsBot();
   bool t2_is_bot = gettype(e2)->IsBot();

   // If both are bots
   if(t1_is_bot && t2_is_bot) {
     //std::cout<<"Both bot \n";
      // Set e1 to be a forw(e2) type
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      // Add all pending items of e1 to e2's pending items
      _pending_vector[e2].insert(_pending_vector[e1].begin(), _pending_vector[e1].end());
   } else if(t1_is_bot && !t2_is_bot) {
     //std::cout<<"bot NotBot\n";    
      // If e1 is bot but e2 is not bot
      // Set e1 to be a forw(e2) type
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      // Join all e1's pending items with e2
      set<unsigned> pv_e1 = _pending_vector[e1]; // note: deliberate deep copy
      set<unsigned>::const_iterator p;
      FORALL(p, pv_e1) {
         join(*p, e2);
      }
   } else if(!t1_is_bot && t2_is_bot) {
     //std::cout<<"not Bot bot \n";
      // If e1 is not bot and e2 is not bot
      // Set e2 to be a forw(e1) type
      CSteensgaardPAForwType * new_e2_t = new CSteensgaardPAForwType(e1);
      settype(e2, new_e2_t);
      // Join all e2's pending items with e1
      set<unsigned> pv_e2 = _pending_vector[e2]; // note: deliberate deep copy
      set<unsigned>::const_iterator p;
      FORALL(p, pv_e2) {
         join(*p, e1);
      }
   } else if(!t1_is_bot && !t2_is_bot) {
     //std::cout<<"Both not bot \n";
      // Neither t1 or t2 is bot
      // Unify the two types
      auto_ptr<CSteensgaardPAType> e1_t(gettype(e1)->Copy());
      CSteensgaardPAType * e2_t = gettype(e2);

      // Set e1 to be a forw(e2) type (will remove e1_t)
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      unify(e1_t.get(), e2_t);
   } else {
      // We should not be able to get here
      assert(0);
   }
}


// Required for function pointer assignment.If type of both e1 and e2 are not Bot, we shall not unify as join
void
CSteensgaardPA::
joinWithoutUnification(int e1, int e2)
{
   // To get the variables their forward references ending at
   e1 = deref(e1);
   e2 = deref(e2);

   // To avoid unnneccessary work
   if(e1 == e2) return;

   // Get if e1 or e2 is bot
   bool t1_is_bot = gettype(e1)->IsBot();
   bool t2_is_bot = gettype(e2)->IsBot();

   // If both are bots
   if(t1_is_bot && t2_is_bot) {
     //std::cout<<"Both bot \n";
      // Set e1 to be a forw(e2) type
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      // Add all pending items of e1 to e2's pending items
      _pending_vector[e2].insert(_pending_vector[e1].begin(), _pending_vector[e1].end());
   } else if(t1_is_bot && !t2_is_bot) {
     //std::cout<<"bot NotBot\n";    
      // If e1 is bot but e2 is not bot
      // Set e1 to be a forw(e2) type
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      // Join all e1's pending items with e2
      set<unsigned> pv_e1 = _pending_vector[e1]; // note: deliberate deep copy
      set<unsigned>::const_iterator p;
      FORALL(p, pv_e1) {
         join(*p, e2);
      }
   } else if(!t1_is_bot && t2_is_bot) {
     //std::cout<<"not Bot bot \n";
      // If e1 is not bot and e2 is not bot
      // Set e2 to be a forw(e1) type
      CSteensgaardPAForwType * new_e2_t = new CSteensgaardPAForwType(e1);
      settype(e2, new_e2_t);
      // Join all e2's pending items with e1
      set<unsigned> pv_e2 = _pending_vector[e2]; // note: deliberate deep copy
      set<unsigned>::const_iterator p;
      FORALL(p, pv_e2) {
         join(*p, e1);
      }
   } else if(!t1_is_bot && !t2_is_bot) {
     //std::cout<<"Both not bot \n";
      // Neither t1 or t2 is bot
      // Unify the two types
      auto_ptr<CSteensgaardPAType> e1_t(gettype(e1)->Copy());
      CSteensgaardPAType * e2_t = gettype(e2);

      // Set e1 to be a forw(e2) type (will remove e1_t)
      CSteensgaardPAForwType * new_e1_t = new CSteensgaardPAForwType(e2);
      settype(e1, new_e1_t);
      //unify(e1_t.get(), e2_t);  // Not required
   } else {
      // We should not be able to get here
      assert(0);
   }
}



// Unconditional unify
void
CSteensgaardPA::
unify(CSteensgaardPAType * t1, CSteensgaardPAType * t2)
{
   assert(t1);
   assert(t2);
   // If we should unify two ref types
   if(t1->IsRef() and t2->IsRef()) {
      // Downcast the types to ref types
      CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
      CSteensgaardPARefType * ref_t2 = static_cast<CSteensgaardPARefType *>(t2);

      // Get the integers representing the vars and the funcs
      int var1 = ref_t1->Var();
      int func1 = ref_t1->Func();
      int var2 = ref_t2->Var();
      int func2 = ref_t2->Func();

      // Join them if they are not equal
      if(var1 != var2) {
         join(var1, var2);
      }
      if(func1 != func2) {
         join(func1, func2);
      }
   } else if(t1->IsLambda() and t2->IsLambda()) {
      // If we should unify two lambda types
      // Downcast the types to lambda types
      CSteensgaardPALambdaType * ref_t1 = static_cast<CSteensgaardPALambdaType *>(t1);
      CSteensgaardPALambdaType * ref_t2 = static_cast<CSteensgaardPALambdaType *>(t2);

      // Get the integers representing the args and the rets
      // note: deliberate deep copies
      vector<unsigned> args1 = ref_t1->Args();
      vector<unsigned> rets1 = ref_t1->Rets();
      vector<unsigned> args2 = ref_t2->Args();
      vector<unsigned> rets2 = ref_t2->Rets();

      // Do the join pairwise on the arguments
      vector<unsigned>::const_iterator a1;
      vector<unsigned>::const_iterator a2;
      for(a1 = args1.begin(), a2 = args2.begin(); a1 != args1.end() && a2 != args2.end(); a1++, a2++) {
         join(*a1, *a2);
      }
      // Do the join pairwise on the returns
      vector<unsigned>::const_iterator r1;
      vector<unsigned>::const_iterator r2;
      for(r1 = rets1.begin(), r2 = rets2.begin(); r1 != rets1.end() && r2 != rets2.end(); r1++, r2++) {
         join(*r1, *r2);
      }
   } else {
      // We should not be able to get here
      assert(0);
   }
}

// Conditional unify
void
CSteensgaardPA::
cunify(CSteensgaardPAType * t1, CSteensgaardPAType * t2)
{
   // If we should unify two ref types
   if(t1->IsRef() and t2->IsRef()) {
      // Downcast the types to ref types
      CSteensgaardPARefType * ref_t1 = static_cast<CSteensgaardPARefType *>(t1);
      CSteensgaardPARefType * ref_t2 = static_cast<CSteensgaardPARefType *>(t2);

      // Get the integers representing the vars and the funcs
      int var1 = ref_t1->Var();
      int func1 = ref_t1->Func();
      int var2 = ref_t2->Var();
      int func2 = ref_t2->Func();
      // Join them if they are not equal
      if(var1 != var2) {
         cjoin(var1, var2);
      }

      if(func1 != func2) {
         cjoin(func1, func2);
      }
   } else if(t1->IsLambda() and t2->IsLambda()) {
      // If we should unify two lambda types
      // Downcast the types to lambda types
      CSteensgaardPALambdaType * ref_t1 = static_cast<CSteensgaardPALambdaType *>(t1);
      CSteensgaardPALambdaType * ref_t2 = static_cast<CSteensgaardPALambdaType *>(t2);

      // Get the integers representing the args and the rets
      // note: deliberate deep copies
      vector<unsigned> args1 = ref_t1->Args();
      vector<unsigned> rets1 = ref_t1->Rets();
      vector<unsigned> args2 = ref_t2->Args();
      vector<unsigned> rets2 = ref_t2->Rets();

      // Do the join pairwise on the arguments
      vector<unsigned>::const_iterator a1;
      vector<unsigned>::const_iterator a2;
      for(a1 = args1.begin(), a2 = args2.begin(); a1 != args1.end() && a2 != args2.end(); a1++, a2++) {
         cjoin(*a1, *a2);
      }

      // Do the join pairwise on the returns
      vector<unsigned>::const_iterator r1;
      vector<unsigned>::const_iterator r2;
      for(r1 = rets1.begin(), r2 = rets2.begin(); r1 != rets1.end() && r2 != rets2.end(); r1++, r2++) {
         cjoin(*r1, *r2);
      }
   } else {
      // We should not be able to get here
      assert(0);
   }
}

// Alternative printing function
ostream &operator << (ostream &o, const CSteensgaardPA &a)
{
   a.PrintAsPointsToSets(o);
   a.PrintInternals(o);
   return o;
}

// -------------------------------------------------------
// CSteensgaardPARefType class
// -------------------------------------------------------.

CSteensgaardPARefType::
CSteensgaardPARefType(int var, int func)
{
   _var = var;
   _func = func;
}

// To copy the current reference type
CSteensgaardPAType *
CSteensgaardPARefType::
Copy(void)
{
   return new CSteensgaardPARefType(_var, _func);
}

// Print of reference type
void
CSteensgaardPARefType::
Print(ostream * o)
{
   *o << "ref(" << _var << " " << _func << ")";
}

// -------------------------------------------------------
// CSteensgaardPALambdaType class
// -------------------------------------------------------
CSteensgaardPALambdaType::
CSteensgaardPALambdaType(const vector<unsigned>& args, const vector<unsigned>& rets)
{
   _args = args;
   _rets = rets;
}

// To copy the current lambda type
CSteensgaardPAType *
CSteensgaardPALambdaType::
Copy(void)
{
   return new CSteensgaardPALambdaType(_args, _rets);
}

// Printing of lambda type
void
CSteensgaardPALambdaType::
Print(ostream * o)
{
   *o << "lambda(";
   vector<unsigned>::iterator a;
   FORALL(a, _args) {
      *o << *a;
      if((a+1) != _args.end())
         *o << " ";
   }
   *o << ")(";
   vector<unsigned>::iterator r;
   FORALL(r, _rets) {
      *o << *r;
      if((r+1) != _rets.end())
         *o << " ";
   }
   *o << ")";
}

// -------------------------------------------------------
// CSteensgaardPAForwType class
// -------------------------------------------------------.
CSteensgaardPAForwType::
CSteensgaardPAForwType(int forward_var)
{
   _forward_var = forward_var;
}

// To copy the current forward reference type
CSteensgaardPAType *
CSteensgaardPAForwType::
Copy(void)
{
   return new CSteensgaardPAForwType(_forward_var);
}

// Print of forward type
void
CSteensgaardPAForwType::
Print(ostream * o)
{
   *o << "forw(" << _forward_var << ")";
}

// -------------------------------------------------------
// CSteensgaardPABotType class
// -------------------------------------------------------.
// To copy the current forward reference type
CSteensgaardPAType *
CSteensgaardPABotType::
Copy(void)
{
   return new CSteensgaardPABotType();
}

// Print of forward type
void
CSteensgaardPABotType::
Print(ostream * o)
{
   *o << "bot";
}

// -------------------------------------------------------
// Alternative printing function
// -------------------------------------------------------
ostream &operator << (ostream &o, CSteensgaardPAType &a)
{
   a.Print(&o);
   return o;
}
