/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/


//== CallGraph.h - AST-based Call graph  ------------------------*- C++ -*--==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file declares the AST-based CallGraph.
//
//  A call graph for functions whose definitions/bodies are available in the
//  current translation unit. The graph has a "virtual" root node that contains
//  edges to all externally available functions.
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_ANALYSIS_CALLGRAPH_H
#define LLVM_CLANG_ANALYSIS_CALLGRAPH_H

#include "clang/AST/DeclBase.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/SetVector.h"
#include <set>
#include <map>

namespace clang {

class CallGraphNode {
public:
  typedef CallGraphNode* CallRecord;

private:
  /// \brief The function/method declaration.
  Decl *FD;

  /// \brief The list of functions called from this node.
  SmallVector<CallRecord, 5> CalledFunctions;

public:
  CallGraphNode(Decl *D) : FD(D) {}

  typedef SmallVectorImpl<CallRecord>::iterator iterator;
  typedef SmallVectorImpl<CallRecord>::const_iterator const_iterator;

  /// Iterators through all the callees/children of the node.
  inline iterator begin() { return CalledFunctions.begin(); }
  inline iterator end()   { return CalledFunctions.end(); }
  inline const_iterator begin() const { return CalledFunctions.begin(); }
  inline const_iterator end()   const { return CalledFunctions.end();   }

  inline bool empty() const {return CalledFunctions.empty(); }
  inline unsigned size() const {return CalledFunctions.size(); }

  void addCallee(CallGraphNode *N) {
    CalledFunctions.push_back(N);
  }

  // delete an edge from root to N
  void deleteChild(CallGraphNode *N)
  {
    for(SmallVector<CallRecord, 5>::iterator it=CalledFunctions.begin();it!=CalledFunctions.end();it++)
      {
	if(*it==N){
	  CalledFunctions.erase(it);
	  break;
	}
      }
  }
  Decl *getDecl() const { return FD; }
  void setDecl(Decl *D) {FD=D;}
  
  void print(raw_ostream &os) const;
  void dump() const;
};


/// \brief The AST-based call graph.
///
/// The call graph extends itself with the given declarations by implementing
/// the recursive AST visitor, which constructs the graph by visiting the given
/// declarations.


class CallGraphCtu;
typedef CallGraphCtu CallGraph;

class CallGraphCtu : public RecursiveASTVisitor<CallGraphCtu> {

  friend class CallGraphNode;
  
  typedef llvm::DenseMap<const Decl *, std::unique_ptr<CallGraphNode>>
      FunctionMapTy;

  typedef std::multimap<std::string,CallGraphNode * > DeclNameType;
  typedef std::vector<std::pair<Decl *,CallGraphNode *> > NodesCtuType;
 
  /// FunctionMap owns all CallGraphNodes.
  FunctionMapTy FunctionMap;

  NodesCtuType NodesWithCtuDecl;
  DeclNameType ExportedDecl;
  /// This is a virtual root node that has edges to all the functions.
  CallGraphNode *Root;

public:
  CallGraphCtu();
  ~CallGraphCtu();

  /// \brief Populate the call graph with the functions in the given
  /// declaration.
  ///
  /// Recursively walks the declaration to find all the dependent Decls as well.
  void addToCallGraph(Decl *D) {
    TraverseDecl(D);
  }

  void removeRootChild(CallGraphNode *child)
  {
    Root->deleteChild(child);
  } 
 
  void addDeclToExport(CallGraphNode *node){
    Decl *D=node->getDecl();
    if(FunctionDecl *func=dyn_cast<FunctionDecl>(D)){
      std::string name=func->getNameInfo().getAsString();
      ExportedDecl.insert(std::pair<std::string, CallGraphNode *>(name,node));
    }
  }

  void includeCGNodesCtu(Decl *decl, CallGraphNode *parent){
    if(dyn_cast<FunctionDecl>(decl))
      NodesWithCtuDecl.push_back(std::pair<Decl *,CallGraphNode *>(decl,parent));
  }

  bool equalFuncDecls(FunctionDecl *f1, FunctionDecl *f2)
  {
    assert(f1 && f2);
    if(f1->getNumParams()!=f2->getNumParams())
      return false;
    if(f1->getReturnType().getAsString()!=f2->getReturnType().getAsString())
      return false;
    for(unsigned i=0;i<f1->getNumParams();++i)
      {
	std::string arg1=f1->parameters()[i]->getQualifiedNameAsString();
	std::string arg2=f2->parameters()[i]->getQualifiedNameAsString();
	if(arg1!=arg2) return false;
      }
    return true;
  }

  // This method should be called once all the translation units are traversed. 
  void finishGraphConstruction();

  /// \brief Determine if a declaration should be included in the graph.
  static bool includeInGraph(const Decl *D);

  /// \brief Lookup the node for the given declaration.
  CallGraphNode *getNode(const Decl *) const;

  /// \brief Lookup the node for the given declaration. If none found, insert
  /// one into the graph.
  CallGraphNode *getOrInsertNode(Decl *);

  /// Iterators through all the elements in the graph. Note, this gives
  /// non-deterministic order.
  typedef FunctionMapTy::iterator iterator;
  typedef FunctionMapTy::const_iterator const_iterator;
  iterator begin() { return FunctionMap.begin(); }
  iterator end()   { return FunctionMap.end();   }
  const_iterator begin() const { return FunctionMap.begin(); }
  const_iterator end()   const { return FunctionMap.end();   }

  /// \brief Get the number of nodes in the graph.
  unsigned size() const { return FunctionMap.size(); }

  /// \ brief Get the virtual root of the graph, all the functions available
  /// externally are represented as callees of the node.
  CallGraphNode *getRoot() const { return Root; }

  /// Iterators through all the nodes of the graph that have no parent. These
  /// are the unreachable nodes, which are either unused or are due to us
  /// failing to add a call edge due to the analysis imprecision.
  typedef llvm::SetVector<CallGraphNode *>::iterator nodes_iterator;
  typedef llvm::SetVector<CallGraphNode *>::const_iterator const_nodes_iterator;

  void print(raw_ostream &os) const;
  void dump() const;
  void viewGraph() const;

  void addNodesForBlocks(DeclContext *D);

  /// Part of recursive declaration visitation. We recursively visit all the
  /// declarations to collect the root functions.
  bool VisitFunctionDecl(FunctionDecl *FD) {
    // We skip function template definitions, as their semantics is
    // only determined when they are instantiated.
    if (includeInGraph(FD) && FD->isThisDeclarationADefinition()) {
      // Add all blocks declared inside this function to the graph.
      addNodesForBlocks(FD);
      // If this function has external linkage, anything could call it.
      // Note, we are not precise here. For example, the function could have
      // its address taken.
      //addDeclToExport(FD);
      addNodeForDecl(FD, FD->isGlobal());
    }
    return true;
  }

  /// Part of recursive declaration visitation.
  bool VisitObjCMethodDecl(ObjCMethodDecl *MD) {
    if (includeInGraph(MD)) {
      addNodesForBlocks(MD);
      addNodeForDecl(MD, true);
    }
    return true;
  }

  // We are only collecting the declarations, so do not step into the bodies.
  bool TraverseStmt(Stmt *S) { return true; }

  bool shouldWalkTypesOfTypeLocs() const { return false; }

private:
  /// \brief Add the given declaration to the call graph.
  void addNodeForDecl(Decl *D, bool IsGlobal);

  /// \brief Allocate a new node in the graph.
  CallGraphNode *allocateNewNode(Decl *);
};


} // end clang namespace

// Graph traits for iteration, viewing.
namespace llvm {
template <> struct GraphTraits<clang::CallGraphNode*> {
  typedef clang::CallGraphNode NodeType;
  typedef clang::CallGraphNode *NodeRef;
  typedef NodeType::iterator ChildIteratorType;

  static NodeType *getEntryNode(clang::CallGraphNode *CGN) { return CGN; }
  static inline ChildIteratorType child_begin(NodeType *N) {
    return N->begin();
  }
  static inline ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

template <> struct GraphTraits<const clang::CallGraphNode*> {
  typedef const clang::CallGraphNode NodeType;
  typedef const clang::CallGraphNode *NodeRef;
  typedef NodeType::const_iterator ChildIteratorType;

  static NodeType *getEntryNode(const clang::CallGraphNode *CGN) { return CGN; }
  static inline ChildIteratorType child_begin(NodeType *N) { return N->begin();}
  static inline ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

template <> struct GraphTraits<clang::CallGraphCtu*>
  : public GraphTraits<clang::CallGraphNode*> {

  static NodeType *getEntryNode(clang::CallGraphCtu *CGN) {
    return CGN->getRoot();  // Start at the external node!
  }

  static clang::CallGraphNode *
  CGGetValue(clang::CallGraphCtu::const_iterator::value_type &P) {
    return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<clang::CallGraphCtu::iterator, decltype(&CGGetValue)>
      nodes_iterator;

  static nodes_iterator nodes_begin(clang::CallGraphCtu *CG) {
    return nodes_iterator(CG->begin(), &CGGetValue);
  }
  static nodes_iterator nodes_end  (clang::CallGraphCtu *CG) {
    return nodes_iterator(CG->end(), &CGGetValue);
  }

  static unsigned size(clang::CallGraphCtu *CG) {
    return CG->size();
  }
};

template <> struct GraphTraits<const clang::CallGraphCtu*> :
  public GraphTraits<const clang::CallGraphNode*> {
  static NodeType *getEntryNode(const clang::CallGraphCtu *CGN) {
    return CGN->getRoot();
  }

  static clang::CallGraphNode *
  CGGetValue(clang::CallGraphCtu::const_iterator::value_type &P) {
    return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  typedef mapped_iterator<clang::CallGraphCtu::const_iterator,
                          decltype(&CGGetValue)>
      nodes_iterator;

  static nodes_iterator nodes_begin(const clang::CallGraphCtu *CG) {
    return nodes_iterator(CG->begin(), &CGGetValue);
  }
  static nodes_iterator nodes_end(const clang::CallGraphCtu *CG) {
    return nodes_iterator(CG->end(), &CGGetValue);
  }
  static unsigned size(const clang::CallGraphCtu *CG) {
    return CG->size();
  }
};

} // end llvm namespace

#endif
