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

  /* void printRaceResult(const VarsLoc res) */
  /* { */
  /*   if(res.empty()) return; */
  /*   for(auto it=res.begin();it!=res.end();it++) */
  /*   { */
  /*     GlobalVarHandler *gv0=paInfo[0]; */
  /*     GlobalVarHandler *gv1=paInfo[1]; */
  /*     if(gv0 && !gv0->showVarReadWriteLoc(it->second)) */
  /* 	if(gv1) */
  /* 	  gv1->showVarReadWriteLoc(it->second); */
  /*   }  */
  /* } */


  void printRaceResult(const VarsLoc res, unsigned handle, bool read)
  {
    if(res.empty()) return;
    assert(handle<2);
    GlobalVarHandler *gv=paInfo[handle];
    if(!gv) {
      errs()<<"PA analysis does not produce Global Vars info for the TU "<<gv->getTUName()<<"\n";
      return;
    }
    std::string str;
    if(read) str="Read";
    else str="Write";	       
    for(auto it=res.begin();it!=res.end();it++){
      gv->showVarAccessLoc(it->second,str);
    }	
     
  }

  

  //This method will be optimized
  /* VarsLoc set_intersect(const VarsLoc S1, const VarsLoc S2) */
  /* { */
  /*   VarsLocIter B1,B2,E1,E2; */
  /*   VarsLoc result; */
  /*   B1=S1.begin();  */
  /*   B2=S2.begin(); */
  /*   E1=S1.end();  */
  /*   E2=S2.end(); */
  /*   std::set<std::string> V1,V2,V3; */
  /*   std::multimap<std::string,std::string> map; */
  /*   //split VarsLoc */
  /*   for(;B1!=E1;B1++) */
  /*   { */
  /*     V1.insert(B1->first); */
  /*     map.insert(std::pair<std::string,std::string>(B1->first,B1->second)); */
  /*   } */
  /*   for(;B2!=E2;B2++) */
  /*   { */
  /*     V2.insert(B2->first); */
  /*     map.insert(std::pair<std::string,std::string>(B2->first,B2->second)); */
  /*   } */
  /*   std::set_intersection(V1.begin(), V1.end(),V2.begin(),V2.end(), std::inserter(V3,V3.begin())); */
  /*   std::set<std::string>::iterator B=V3.begin(),E=V3.end(); */
  /*   for(;B!=E;B++) */
  /*   { */
  /*     std::pair <std::multimap<std::string,std::string>::iterator, std::multimap<std::string,std::string>::iterator> range; */
  /*     range = map.equal_range(*B); */
  /*     for(std::multimap<std::string,std::string>::iterator irange = range.first; irange != range.second; ++irange) */
  /* 	result.insert(std::pair<std::string,std::string>(*B,irange->second)); */
  /*   }   */
  /*   return result; */
  /* }   */

  std::pair<VarsLoc,VarsLoc> set_intersect(const VarsLoc S1, const VarsLoc S2)
  {
    VarsLocIter B1,B2,E1,E2;
    VarsLoc result1,result2;
    B1=S1.begin(); 
    B2=S2.begin();
    E1=S1.end(); 
    E2=S2.end();
    std::set<std::string> V1,V2,V3;
    std::multimap<std::string,std::string> map1,map2;
    //split VarsLoc
    for(;B1!=E1;B1++)
    {
      V1.insert(B1->first);
      map1.insert(std::pair<std::string,std::string>(B1->first,B1->second));
    }
    for(;B2!=E2;B2++)
    {
      V2.insert(B2->first);
      map2.insert(std::pair<std::string,std::string>(B2->first,B2->second));
    }
    std::set_intersection(V1.begin(), V1.end(),V2.begin(),V2.end(), std::inserter(V3,V3.begin()));
    std::set<std::string>::iterator B=V3.begin(),E=V3.end();
    for(;B!=E;B++)
    {
      std::pair <std::multimap<std::string,std::string>::iterator, std::multimap<std::string,std::string>::iterator> range1,range2;
      range1 = map1.equal_range(*B);
      range2 = map2.equal_range(*B);
      for(std::multimap<std::string,std::string>::iterator irange = range1.first; irange != range1.second; ++irange)
	result1.insert(std::pair<std::string,std::string>(*B,irange->second));
      for(std::multimap<std::string,std::string>::iterator irange = range2.first; irange != range2.second; ++irange)
	result2.insert(std::pair<std::string,std::string>(*B,irange->second));
    }  
    std::pair<VarsLoc,VarsLoc> result=std::make_pair(result1,result2);
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
          
     VarsLoc read1,read2,write1,write2;
     std::pair<VarsLoc,VarsLoc> res1,res2,res3;
     std::string tu1=paInfo[0]->getTUName(), tu2=paInfo[1]->getTUName();

     read1=getGlobalRead(0);
     read2=getGlobalRead(1);
     write1=getGlobalWrite(0);
     write2=getGlobalWrite(1);
     
     res1=set_intersect(read1,write2);
     res2=set_intersect(write1,read2);
     res3=set_intersect(write1,write2);

     
     // errs()<<"\n********************************************\n";
     if(res1.first.empty() && res2.first.empty() && res3.first.empty())
        errs()<<"\t!!!No Data Race found!!! \n";
     else{
       errs()<<"\t!!!Possible Data Race Detected!!!\n\n";
       if(!res1.first.empty())
	 {
	    errs()<<"Simultaneous Read(by "<<tu1<<"), Write(by "<<tu2<<") possible\n";
	    errs()<<"===================================================================================================\n\n";
	    printRaceResult(res1.first,0,true);
	    //errs()<<"Execution invoked from: "<<tu2<<"\n";
	    printRaceResult(res1.second,1,false);

         }
       if(!res2.first.empty())
	 {
	   errs()<<"\nSimultaneous Write(by "<<tu1<<"), Read(by "<<tu2<<") possible\n";
	   errs()<<"====================================================================================================\n";
	   printRaceResult(res2.first,0,false);
	   printRaceResult(res2.second,1,true);

	 }
       if(!res3.first.empty())
	{
	  errs()<<"\nSimultaneous Write(by "<<tu1<<"), Write(by "<<tu2<<") possible\n";
	  errs()<<"======================================================================================================\n";
	  printRaceResult(res3.first,0,false);
	  printRaceResult(res3.second,1,false);

	} 
     }      
   errs()<<"\n------------------------------------------------------------\n\n";
  }
};

#endif
