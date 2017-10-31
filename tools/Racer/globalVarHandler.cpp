/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "globalVarHandler.h"
  
void GlobalVarHandler::insert(unsigned var, string loc)
{ 
  globals.insert(loc);
  globalVarId.insert(var);
  globalVarMap.insert(std::pair<unsigned,string>(var,loc));
}
std::string GlobalVarHandler::getVarAsLoc(unsigned var)
{
  // var should be from the globals set
  auto it=globalVarMap.find(var);
  if(it!=globalVarMap.end())
    return (it->second);
  else 
    assert(0);   //execution should not reach here
}  

void GlobalVarHandler::storeGlobalRead(const std::set<unsigned> &vars, std::string l)
{ 
for(SetIter i=vars.begin();i!=vars.end();i++)
  {
    std::string var=getVarAsLoc(*i);
    globalRead.insert(std::pair<std::string,std::string>(var,l)); 
    //globalRead.insert(std::make_tuple(var,l,funcLoc)); 
  }
}

void GlobalVarHandler::storeGlobalWrite(const std::set<unsigned> &vars, std::string l)
{
  for(SetIter i=vars.begin();i!=vars.end();i++){
    std::string var=getVarAsLoc(*i);
    globalWrite.insert(std::pair<std::string,std::string>(var,l)); 
    //globalWrite.insert(std::make_tuple(var,l,funcLoc)); 
  }
}

void GlobalVarHandler::storeMapInfo(std::string loc,std::string vCurr, std::string vGlobal,std::string funcLoc)
{
  //std::pair<std::string,std::string> vInfo=std::pair<std::string,std::string>(vCurr,vGlobal); 
  std::tuple<std::string,std::string,std::string> vInfo=std::make_tuple(vCurr,vGlobal,funcLoc); 
  locToVarPairMap.insert(std::pair< std::string,std::tuple<std::string,std::string,std::string> >(loc,vInfo));
}

void GlobalVarHandler::showGlobals()
{
  errs()<<"List of global vars\n";
  for(auto i=globals.begin();i!=globals.end();i++)
    errs()<<(*i)<<"\n";
      
}  

void GlobalVarHandler::printGlobalRead()
{
  VarsLocIter I=globalRead.begin(), E=globalRead.end();
  errs()<<"List of Global Reads: \n";
  for(;I!=E;I++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(I->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	//errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<I->second<<"\n"; 
        errs()<< std::get<0>(irange->second) <<" accesses "<<std::get<1>(irange->second)<<" at "<<I->second<<" in "<<std::get<2>(irange->second)<<"\n"; 
    }
}

void GlobalVarHandler::printGlobalWrite()
{
  VarsLocIter I=globalWrite.begin(), E=globalWrite.end();
  errs()<<"List of Global Writes: \n";
  for(;I!=E;I++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(I->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	//errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<I->second<<"\n"; 
	errs()<< std::get<0>(irange->second) <<" accesses "<<std::get<1>(irange->second)<<" at "<<I->second<<" in "<<std::get<2>(irange->second)<<"\n";
    }
}

bool GlobalVarHandler::showVarAccessLoc(std::string loc, std::string accMod)
{
  std::pair <MapType::iterator, MapType::iterator> range;
  range = locToVarPairMap.equal_range(loc);
  if(range.first==locToVarPairMap.end()) return false;
  for (MapType::iterator irange = range.first; irange != range.second; ++irange)
   {
    errs()<<"["<<accMod<<", Loc:"<<loc<<"]:\t";
    if(get<0>(irange->second)!=get<1>(irange->second))
      errs()<<std::get<0>(irange->second) <<"->"<<std::get<1>(irange->second)<<" at "<<std::get<2>(irange->second)<<"\n";
    else errs()<<std::get<0>(irange->second) <<" at "<<std::get<2>(irange->second)<<"\n";
   }   
   return true;
}


std::set<std::string> GlobalVarHandler:: getFuncLocOfVarAccess(std::string loc)
{
  std::set<std::string> Locs;
  std::pair <MapType::iterator, MapType::iterator> range;
  range = locToVarPairMap.equal_range(loc);
  if(range.first==locToVarPairMap.end()) return Locs;
  for (MapType::iterator irange = range.first; irange != range.second; ++irange)
    Locs.insert(std::get<2>(irange->second));
  return Locs;
}


