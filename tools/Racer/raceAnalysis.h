/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef RACEANALYSIS_H_
#define RACEANALYSIS_H_

#include "globalVarHandler.h"
#include <vector>
#include <cassert>

class RaceFinder
{
private:
  std::vector<GlobalVarHandler *> paInfo; 
  unsigned lastHandle;
  
public:
  RaceFinder(){ lastHandle=0;}
  ~RaceFinder() {}
  inline unsigned currentHandle(){ return lastHandle>0 ? lastHandle-1 : 0; }
  void createNewTUAnalysis(GlobalVarHandler * gv);
  inline VarsLoc getGlobalRead(unsigned l){ return paInfo[l]->getGlobalRead();}
  inline VarsLoc getGlobalWrite(unsigned l){ return paInfo[l]->getGlobalWrite();}
  inline void printGlobalRead(unsigned handle){ paInfo[handle]->printGlobalRead();}
  inline void printGlobalWrite(unsigned handle){ paInfo[handle]->printGlobalWrite();}
  void printRaceResult(const VarsLoc res, unsigned handle, bool read);
  std::pair<VarsLoc,VarsLoc> set_intersect(const VarsLoc S1, const VarsLoc S2);
  void extractPossibleRaces();  
};

#endif
