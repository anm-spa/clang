/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef RACEANALYSIS_H_
#define RACEANALYSIS_H_

#include "clang/AST/Decl.h"
#include "libExt/CallGraphCtu.h"
#include "globalVarHandler.h"
#include <vector>
#include <cassert>
#include <queue>

class RaceFinder
{
private:
  std::vector<GlobalVarHandler *> paInfo; 
  unsigned lastHandle;
  const CallGraph *cg;
  std::string tFunc1,tFunc2;
  bool startPoint=false;
  std::multimap<std::string, std::pair<std::string, bool> > parentOf;
public:
  RaceFinder(){ lastHandle=0;}
  ~RaceFinder() {}
  void setCallGraph(CallGraph *cGraph){cg=cGraph;}
  void setTaskStartPoint(std::string f1,std::string f2){
    tFunc1=f1;tFunc2=f2; startPoint=true;  
  }
  inline unsigned currentHandle(){ return lastHandle>0 ? lastHandle-1 : 0; }
  void createNewTUAnalysis(GlobalVarHandler * gv);
  inline VarsLoc getGlobalRead(unsigned l){ return paInfo[l]->getGlobalRead();}
  inline VarsLoc getGlobalWrite(unsigned l){ return paInfo[l]->getGlobalWrite();}
  inline void printGlobalRead(unsigned handle){ paInfo[handle]->printGlobalRead();}
  inline void printGlobalWrite(unsigned handle){ paInfo[handle]->printGlobalWrite();}
  void printRaceResult(const VarsLoc res, unsigned handle, bool read);
  std::pair<VarsLoc,VarsLoc> set_intersect(const VarsLoc S1, const VarsLoc S2);
  std::pair<VarsLoc,VarsLoc> refine(std::pair<VarsLoc,VarsLoc> res);
  CallGraphNode *getCallGraphNode(CallGraphNode *cgNode, std::string name, bool edgeL);
  std::string nameOfNode(CallGraphNode *currNode);
  bool isnameOfNode(CallGraphNode *currNode,std::string ef);
  bool getReachablePath(std::string from, std::string to);
  void extractPossibleRaces();  
};

#endif
