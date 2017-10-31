/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef LLVM_CLANG_CALLGRAPHANALYSIS_H
#define LLVM_CLANG_CALLGRAPHANALYSIS_H

#include "clang/AST/AST.h"
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include "libExt/CallGraphCtu.h"

#include "clang/Analysis/AnalysisContext.h"
#include <queue>
#include <set>
#include <map>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;


class CGReachabilityInf : public ASTConsumer {
private:
  CallGraph &CG;
  std::string startFunc;
  std::string endFunc;
  std::multimap<std::string, std::pair<std::string, bool> > parentOf;
public:
  explicit CGReachabilityInf(CompilerInstance *CI,CallGraph &g) : CG(g)
  {startFunc="none";endFunc="none";}
  explicit CGReachabilityInf(CompilerInstance *CI, std::string func1, std::string func2,CallGraph &g):CG(g)
  {startFunc=func1;endFunc=func2;}

    bool isnameOfNode(CallGraphNode *currNode,std::string ef)
    {
      if(!currNode) return false;
      clang::Decl *d=currNode->getDecl();
      if(!d) return false;
      std::string st;
      if (FunctionDecl *fd = dyn_cast<FunctionDecl>(d)){
	st=d->getAsFunction()->getNameInfo().getAsString();
      }	
      else st="";
      if(st==ef) {return true;}
      else return false;
    }  
    std::string nameOfNode(CallGraphNode *currNode)
      {
	clang::Decl *d=currNode->getDecl();
	if(!d) return "Root";
	return d->getAsFunction()->getNameInfo().getAsString();
      }  

    CallGraphNode *getCallGraphNode(CallGraphNode *cgNode, std::string name, bool edgeL)
    {
      std::set<CallGraphNode *> visited;
      std::queue<CallGraphNode *> cgQ;
      cgQ.push(cgNode);
      while(!cgQ.empty())
	{
	  CallGraphNode *currNode=cgQ.front();
	  cgQ.pop();
	  visited.insert(currNode);     
	  if(isnameOfNode(currNode,name)) return currNode;
	  for(auto it=currNode->begin();it!=currNode->end();it++)
	    {
	      if(visited.find(*it)==visited.end())
		cgQ.push(*it);
	      parentOf.insert(std::make_pair(nameOfNode(*it), std::make_pair(nameOfNode(currNode),edgeL)));
	    }  
	}
      return NULL;
    }


    // At present it assumes that 
    bool getReachablePath(std::string from, std::string to)
    {
      std::pair <std::multimap<std::string, std::pair<std::string, bool> >::iterator, std::multimap<std::string,std::pair<std::string, bool> >::iterator > range;
      std::vector<std::string> cgV;
      cgV.push_back(to);
      std::string temp=to;
      do{
      range = parentOf.equal_range(temp);
      std::multimap<std::string, std::pair<std::string, bool> >::iterator irange = range.first;
      for(; irange != range.second; ++irange)
	{
	  if(irange->second.second)
	    {
	      temp=irange->second.first;
	      cgV.push_back(temp);
	      break;
	    }
	}
      if(irange == range.second) return false;
      }while(temp!=from);

      errs()<<"\n";
      for(std::string s:cgV){
	errs()<<s<<"<--";
      }
      errs()<<"root\n";
      return true;
    }  

    virtual void HandleTranslationUnit(ASTContext &Context) {     
      TranslationUnitDecl *tu=Context.getTranslationUnitDecl();
      CG.addToCallGraph(tu);
      /*  if(CallGraphNode *startNode=getCallGraphNode(CG.getRoot(), startFunc, 0))
	if(getCallGraphNode(startNode, endFunc, 1))
	getReachablePath(startFunc, endFunc);*/
    }   
   
};

#endif
