#ifndef RACEANALYSIS_H_
#define RACEANALYSIS_H_

#include "GlobalVarHandler.h"
#include <vector>

class RaceFinder
{
private:
  std::vector<GlobalVarHandler *> paInfo; 
  unsigned lastHandle;
  
public:
  RaceFinder(){
   lastHandle=0;
  }
  ~RaceFinder()
  {
  }

  unsigned currentHandle()
  {
    if(lastHandle>0) return lastHandle-1;
    else return 0;
  }

  void createNewTUAnalysis(GlobalVarHandler * gv)
  {
    paInfo.push_back(gv);
    ++lastHandle;
    // errs()<<"Handle "<<lastHandle-1<<"\n";
  }

  
  VarsLoc getGlobalRead(unsigned l)
  {
    GlobalVarHandler *gv=paInfo[l];
    return gv->getGlobalRead();  
  }
  
  VarsLoc getGlobalWrite(unsigned l)
  {
    GlobalVarHandler *gv=paInfo[l];
    return gv->getGlobalWrite();  
  }

  void printGlobalRead(unsigned handle)
  {
    GlobalVarHandler *gv=paInfo[handle];
    gv->printGlobalRead();  
  }

  void printGlobalWrite(unsigned handle)
  {
    GlobalVarHandler *gv=paInfo[handle];
    gv->printGlobalWrite();  
  }

  void printRaceResult(const VarsLoc res)
  {
    if(res.empty()) return;
    for(auto it=res.begin();it!=res.end();it++)
    {
      GlobalVarHandler *gv0=paInfo[0];
      GlobalVarHandler *gv1=paInfo[1];
      if(gv0 && !gv0->printVarAccessInfo(it->second))
	if(gv1)
	  gv1->printVarAccessInfo(it->second);
    } 
  }

  //This method will be optimized
  VarsLoc set_intersect(const VarsLoc S1, const VarsLoc S2)
  {
    VarsLocIter ibegin1,ibegin2,iend1,iend2;
    VarsLoc result;
    ibegin1=S1.begin(); 
    ibegin2=S2.begin();
    iend1=S1.end(); 
    iend2=S2.end();
    std::set<unsigned> V1,V2,V3;
    std::multimap<unsigned,std::string> map;
    //split VarsLoc
    for(;ibegin1!=iend1;ibegin1++)
    {
      V1.insert(ibegin1->first);
      map.insert(std::pair<unsigned,std::string>(ibegin1->first,ibegin1->second));
    }
    for(;ibegin2!=iend2;ibegin2++)
    {
      V2.insert(ibegin2->first);
      map.insert(std::pair<unsigned,std::string>(ibegin2->first,ibegin2->second));
    }
    std::set_intersection(V1.begin(), V1.end(),V2.begin(),V2.end(), std::inserter(V3,V3.begin()));
    SetIter ib=V3.begin(),ie=V3.end();
    for(;ib!=ie;ib++)
    {
      std::pair <std::multimap<unsigned,std::string>::iterator, std::multimap<unsigned,std::string>::iterator> range;
	 range = map.equal_range(*ib);
	 for(std::multimap<unsigned,std::string>::iterator irange = range.first; irange != range.second; ++irange)
	   result.insert(std::pair<unsigned,std::string>(*ib,irange->second));
    }  
    return result;
  }  

  void extractPossibleRaces()
  {
     errs()<<"\n------------------Data Race Information---------------------\n\n";
     //          Debug Print
     //printGlobalRead(0);
     //printGlobalWrite(0);
     //printGlobalRead(1);
     //printGlobalWrite(1);
     
     
     VarsLoc read1,read2,write1,write2,res1,res2,res3;
     read1=getGlobalRead(0);
     read2=getGlobalRead(1);
     write1=getGlobalWrite(0);
     write2=getGlobalWrite(1);
     
     res1=set_intersect(read1,write2);
     res2=set_intersect(write1,read2);
     res3=set_intersect(write1,write2);
     // errs()<<"\n********************************************\n";
     if(res1.empty() && res2.empty() && res3.empty())
        errs()<<"\t!!!No Data Race found!!! \n";
     else{
       errs()<<"\t!!!Possible Data Race Detected!!!\n\n";
       if(!res1.empty())
	 {
	   errs()<<"Followings are the possible Read-Write Pairs:\n\n";
           printRaceResult(res1);
         }
       if(!res2.empty())
	 {
	   errs()<<"\nFollowings are the possible Write-Read Pairs:\n\n";
           printRaceResult(res2);
	 }
       if(!res3.empty())
	{
	 errs()<<"\nFollowings are the possible Write-Write Pairs:\n\n";
         printRaceResult(res3);
	} 
     }      
   errs()<<"\n------------------------------------------------------------\n\n";
  }
};

#endif
