#ifndef GLOBALVARHANDLER_H_
#define GLOBALVARHANDLER_H_

using namespace llvm;

typedef std::set<unsigned>::iterator SetIter; 
typedef enum {RD,WR,RW} AccessType;

//.first is var represented as declared location, .second is location of access
typedef std::set<std::pair<std::string,std::string> > VarsLoc;
typedef std::set<std::pair<std::string,std::string> >::iterator VarsLocIter;

// maps program location to Var accessed and pointing to the global
typedef std::multimap<std::string,std::pair<std::string,std::string> > MapType; 

std::string printAccessType(AccessType acc)
{
  return acc==RD ? "RD":(acc==WR ? "WR": "RW");
}

class GlobalVarHandler
{
private:
  std::set<string> globals;   // Set of all global Vars in a translation unit            
  std::map<unsigned,string> globalVarMap;
  std::set<unsigned> globalVarId;

  // map.second is a set of pointer variables pointing to the global at map.first 
  //std::map<unsigned,std::set<unsigned> > varToVarsGlobals;  

  // set of all pointers pointing to globals
  // std::set<unsigned> pointerToGlobals;     // elements of this set are from the symbol table        
  VarsLoc globalRead;
  VarsLoc globalWrite;
  // locToVarPairMap.second.first points to locToVarPairMap.second.second at locToVarPairMap.first
  MapType locToVarPairMap;
public:
  GlobalVarHandler(){}
  ~GlobalVarHandler(){}
  
  void insert(unsigned var, string loc){ 
    globals.insert(loc);
    globalVarId.insert(var);
    globalVarMap.insert(std::pair<unsigned,string>(var,loc));
  }

  
  std::string getVarAsLoc(unsigned var)
  {
    // var should be from the globals set
    auto it=globalVarMap.find(var);
    if(it!=globalVarMap.end())
      return (it->second);
    else 
      assert(0);   //execution should not reach here
  }  
  
  /*
  void insertPtsToGv(const std::set<unsigned> &vars_pointed_to, unsigned globalVar)
  {
    pointerToGlobals.insert(vars_pointed_to.begin(),vars_pointed_to.end());
    varToVarsGlobals.insert(std::pair<unsigned, std::set<unsigned> >(globalVar,vars_pointed_to));
  }

  */
  
  SetIter gVarsBegin(){return globalVarId.begin();}

  SetIter gVarsEnd(){return globalVarId.end();}  

  bool isGv(unsigned p){ return globalVarId.find(p)!=globalVarId.end();}

  /*
  bool isPtrToGv(unsigned p)
  { return (pointerToGlobals.find(p)!=pointerToGlobals.end()); }
  */

  void storeGlobalRead(const std::set<unsigned> &vars, std::string l)
  {
   for(SetIter i=vars.begin();i!=vars.end();i++)
     {
       std::string var=getVarAsLoc(*i);
       globalRead.insert(std::pair<std::string,std::string>(var,l)); 
     }
  }

  void storeGlobalWrite(const std::set<unsigned> &vars, std::string l)
  {
    for(SetIter i=vars.begin();i!=vars.end();i++){
      std::string var=getVarAsLoc(*i);
      globalWrite.insert(std::pair<std::string,std::string>(var,l)); 
    }
  }

  void storeMapInfo(std::string loc,std::string vCurr, std::string vGlobal)
  {
    std::pair<std::string,std::string> vInfo=std::pair<std::string,std::string>(vCurr,vGlobal); 
    locToVarPairMap.insert(std::pair< std::string,std::pair<std::string,std::string> >(loc,vInfo));
  }

  VarsLoc getGlobalRead()
  {
    return globalRead;
  }
  VarsLoc getGlobalWrite()
  {
    return globalWrite;
  }

  VarsLocIter globalReadBegin()
  {
    return globalRead.begin();
  }

  VarsLocIter globalReadEnd()
  {
    return globalRead.end();
  }
  
  VarsLocIter globalWriteBegin()
  {
    return globalWrite.begin();
  }

  VarsLocIter globalWriteEnd()
  {
    return globalWrite.end();
  }

  void printGlobalRead()
  {
    VarsLocIter I=globalRead.begin(), E=globalRead.end();
    errs()<<"List of Global Reads: \n";
    for(;I!=E;I++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(I->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<I->second<<"\n"; 
    }
   }

  void printGlobalWrite()
  {
    VarsLocIter I=globalWrite.begin(), E=globalWrite.end();
    errs()<<"List of Global Writes: \n";
    for(;I!=E;I++)
    {
      std::pair <MapType::iterator, MapType::iterator> range=locToVarPairMap.equal_range(I->second);
      for (MapType::iterator irange = range.first; irange != range.second; ++irange)
	errs()<< irange->second.first <<" accesses "<<irange->second.second<<" at "<<I->second<<"\n"; 
    }
   }

  bool showVarReadWriteLoc(std::string loc)
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
