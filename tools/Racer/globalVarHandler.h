/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#ifndef GLOBALVARHANDLER_H_
#define GLOBALVARHANDLER_H_

#include "llvm/Support/raw_ostream.h"
#include<set>
#include<map>
#include<cassert>

using namespace std;
using namespace llvm;

typedef std::set<unsigned>::iterator SetIter; 
typedef enum {RD,WR,RW} AccessType;

//.first is var represented as declared location, .second is location of access
typedef std::set<std::pair<std::string,std::string> > VarsLoc;
typedef std::set<std::pair<std::string,std::string> >::iterator VarsLocIter;

// maps program location to Var accessed and pointing to the global
typedef std::map<std::string,std::pair<std::string,std::string> > MapType; 

class GlobalVarHandler
{
private:
  std::set<string> globals;   // Set of all global Vars in a translation unit            
  std::map<unsigned,string> globalVarMap;
  std::set<unsigned> globalVarId;
  std::string mainFile;
  VarsLoc globalRead;
  VarsLoc globalWrite;

  // locToVarPairMap.second.first points to locToVarPairMap.second.second at locToVarPairMap.first
  MapType locToVarPairMap;
public:
  GlobalVarHandler(std::string f){mainFile=f;}
  ~GlobalVarHandler(){}
  
  void insert(unsigned var, string loc);
  
  inline std::string getTUName(){ return mainFile;}
  std::string getVarAsLoc(unsigned var);
  inline SetIter gVarsBegin(){return globalVarId.begin();}
  inline SetIter gVarsEnd(){return globalVarId.end();}  
  inline bool isGv(unsigned p){ return globalVarId.find(p)!=globalVarId.end();}
  void storeGlobalRead(const std::set<unsigned> &vars, std::string l);
  void storeGlobalWrite(const std::set<unsigned> &vars, std::string l);
  void storeMapInfo(std::string loc,std::string vCurr, std::string vGlobal);
  inline VarsLoc getGlobalRead() { return globalRead; }
  inline VarsLoc getGlobalWrite() { return globalWrite;}
  inline VarsLocIter globalReadBegin(){ return globalRead.begin();}
  inline VarsLocIter globalReadEnd() { return globalRead.end(); }
  inline VarsLocIter globalWriteBegin() {return globalWrite.begin();}
  inline VarsLocIter globalWriteEnd() { return globalWrite.end(); }
  void showGlobals();
  void printGlobalRead();
  void printGlobalWrite();
  bool showVarAccessLoc(std::string loc, std::string accMod);
};

#endif
