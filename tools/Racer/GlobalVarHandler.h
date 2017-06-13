#ifndef GLOBALVARHANDLER_H_
#define GLOBALVARHANDLER_H_

using namespace llvm;

typedef std::set<unsigned>::iterator SetIter; 
typedef enum {RD,WR,RW} AccessType;
typedef std::set<std::pair<unsigned,std::string> > VarsLoc;
typedef std::set<std::pair<unsigned,std::string> >::iterator VarsLocIter;
// maps program location to Var accessed and pointing to the global
typedef std::multimap<std::string,std::pair<std::string,std::string> > MapType; 

std::string printAccessType(AccessType acc)
{
  return acc==RD ? "RD":(acc==WR ? "WR": "RW");
}

class GlobalVarHandler
{
private:
  std::set<unsigned> globals;   // Set of all global Vars in a translation unit            

  // map.second is a set of pointer variables pointing to the global at map.first 
  std::map<unsigned,std::set<unsigned> > varToVarsGlobals;  

  // set of all pointers pointing to globals
  std::set<unsigned> pointerToGlobals;            
  VarsLoc varsPtsToGlobalRead;
  VarsLoc varsPtsToGlobalWrite;
  // locToVarPairMap.second.first points to locToVarPairMap.second.second at locToVarPairMap.first
  MapType locToVarPairMap;
public:
  GlobalVarHandler(){}
  ~GlobalVarHandler(){}
  void insert(unsigned var){ globals.insert(var);}
  void insertPtsToGv(const std::set<unsigned> &vars_pointed_to, unsigned globalVar)
  {
    pointerToGlobals.insert(vars_pointed_to.begin(),vars_pointed_to.end());
    varToVarsGlobals.insert(std::pair<unsigned, std::set<unsigned> >(globalVar,vars_pointed_to));
  }
  
  SetIter gVarsBegin(){return globals.begin();}

  SetIter gVarsEnd(){return globals.end();}  

  bool isGv(unsigned p){ return globals.find(p)!=globals.end();}

  bool isPtrToGv(unsigned p)
  { return (pointerToGlobals.find(p)!=pointerToGlobals.end()); }

  void storeGlobalRead(const std::set<unsigned> &vars, std::string l)
  {
   for(SetIter i=vars.begin();i!=vars.end();i++)
     varsPtsToGlobalRead.insert(std::pair<unsigned,std::string>(*i,l)); 
  }

  void storeGlobalWrite(const std::set<unsigned> &vars, std::string l)
  {
   for(SetIter i=vars.begin();i!=vars.end();i++)
     varsPtsToGlobalWrite.insert(std::pair<unsigned,std::string>(*i,l)); 
  }

  void storeMapInfo(std::string loc,std::string vCurr, std::string vGlobal)
  {
    std::pair<std::string,std::string> vInfo=std::pair<std::string,std::string>(vCurr,vGlobal); 
    locToVarPairMap.insert(std::pair< std::string,std::pair<std::string,std::string> >(loc,vInfo));
  }

  VarsLoc getGlobalRead()
  {
    return varsPtsToGlobalRead;
  }
  VarsLoc getGlobalWrite()
  {
    return varsPtsToGlobalWrite;
  }

  VarsLocIter globalReadBegin()
  {
    return varsPtsToGlobalRead.begin();
  }

  VarsLocIter globalReadEnd()
  {
    return varsPtsToGlobalRead.end();
  }
  
  VarsLocIter globalWriteBegin()
  {
    return varsPtsToGlobalWrite.begin();
  }

  VarsLocIter globalWriteEnd()
  {
    return varsPtsToGlobalWrite.end();
  }

  void printGlobalRead()
  {
    VarsLocIter i=varsPtsToGlobalRead.begin(), e=varsPtsToGlobalRead.end();
    errs()<<"List of Global Reads: \n";
    for(;i!=e;i++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(i->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<i->second<<"\n"; 
    }
   }

  void printGlobalWrite()
  {
    VarsLocIter i=varsPtsToGlobalWrite.begin(), e=varsPtsToGlobalWrite.end();
    errs()<<"List of Global Writes: \n";
    for(;i!=e;i++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(i->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<i->second<<"\n"; 
    }
   }

  bool printVarAccessInfo(std::string loc)
  {
    std::pair <MapType::iterator, MapType::iterator> range;
    range = locToVarPairMap.equal_range(loc);
    if(range.first==locToVarPairMap.end()) return false;
    for (MapType::iterator irange = range.first; irange != range.second; ++irange)
      errs()<< irange->second.first <<"->"<<irange->second.second<<" @\t"<<loc<<"\n";
    return true;
  }

};

#endif
