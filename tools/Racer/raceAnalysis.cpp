/****************************************************************/
/*          All rights reserved (it will be changed)            */
/*          masud.abunaser@mdh.se                               */
/****************************************************************/

#include "raceAnalysis.h"

void RaceFinder::createNewTUAnalysis(GlobalVarHandler * gv)
{
  paInfo.push_back(gv);
  ++lastHandle;
}

void RaceFinder::printRaceResult(const VarsLoc res, unsigned handle, bool read)
{
  if(res.empty()) return;
  std::string str;
  assert(handle<2);
  GlobalVarHandler *gv=paInfo[handle];
  if(!gv) {
    errs()<<"PA analysis does not produce Global Vars info for the TU "<<gv->getTUName()<<"\n";
    return;
  }
  if(read) str="Read";
  else str="Write";	       
  for(auto it=res.begin();it!=res.end();it++){
    gv->showVarAccessLoc(it->second,str);
  }	 
}

bool RaceFinder::isnameOfNode(CallGraphNode *currNode,std::string ef)
{
  if(!currNode) return false;
  clang::Decl *d=currNode->getDecl();
  if(!d) return false;
  std::string st;
  if (dyn_cast<FunctionDecl>(d)){
    st=d->getAsFunction()->getNameInfo().getAsString();
  }	
  else st="";
  if(st==ef) {return true;}
  else return false;
}  

std::string RaceFinder::nameOfNode(CallGraphNode *currNode)
{
  if(!currNode) errs()<<"Node not found\n";
  clang::Decl *d=currNode->getDecl();
  if(!d) return "Root";
  if (clang::FunctionDecl *fd = dyn_cast<clang::FunctionDecl>(d))
    return fd->getNameInfo().getAsString();
  return "nonode";
}  

// At present it assumes that 
bool RaceFinder::getReachablePath(std::string from, std::string to)
{
  if(from==to) return true;
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
  
  /* errs()<<"\n";
  for(std::string s:cgV){
    errs()<<s<<"<--";
  }
  errs()<<"root\n";*/
  return true;
}  


CallGraphNode *RaceFinder::getCallGraphNode(CallGraphNode *Node, std::string name, bool edgeL)
{
  std::set<CallGraphNode *> visited;
  std::queue<CallGraphNode *> cgQ;
  if(!Node) return NULL;
  cgQ.push(Node);  
  while(!cgQ.empty())
    {
      CallGraphNode *currNode=cgQ.front();
      cgQ.pop();
      visited.insert(currNode);     
      if(isnameOfNode(currNode,name)) return currNode;
      std::string parent=nameOfNode(currNode);
      //errs()<<"size: "<<currNode->size()<<"\n";
      for(auto it=currNode->begin();it!=currNode->end();it++)
	{
	  if(visited.find(*it)==visited.end())
	    cgQ.push(*it);
	  std::string child=nameOfNode(*it);
	  //errs()<<"Child= "<<child<<" Parent= "<<parent<<"\n";
	  parentOf.insert(std::make_pair(child, std::make_pair(parent,edgeL)));
	} 
    }
  return NULL;
}



std::pair<VarsLoc,VarsLoc> RaceFinder::refine(std::pair<VarsLoc,VarsLoc> res)
{
  if(!startPoint) return res;
  VarsLoc r1=res.first, r2=res.second, rRefine1,rRefine2;
  GlobalVarHandler *gv0=paInfo[0], *gv1=paInfo[1];
  
  if(!gv0 && !gv1) {
    errs()<<"PA analysis does not produce Global Vars info for the TU \n";
    return res;
  }
  if(!cg) return res;
  parentOf.clear();
  CallGraphNode *startNode;
  startNode=getCallGraphNode(cg->getRoot(),tFunc1,0);
  for(auto it=r1.begin();it!=r1.end();it++){
    std::set<std::string> s1=gv0->getFuncLocOfVarAccess(it->second);
    for(std::set<std::string>::iterator sit=s1.begin();sit!=s1.end();sit++)
      {
	if(getCallGraphNode(startNode, *sit, 1)){
	if(getReachablePath(tFunc1, *sit))
	  {
	    rRefine1.insert(std::pair<std::string,std::string>(it->first,it->second));
	    break;
	  }
	}
      }
  }
  
  parentOf.clear();
  startNode=getCallGraphNode(cg->getRoot(),tFunc2,0);
  for(auto it=r2.begin();it!=r2.end();it++){
    std::set<std::string> s1=gv1->getFuncLocOfVarAccess(it->second);
    for(std::set<std::string>::iterator sit=s1.begin();sit!=s1.end();sit++)
      if(getCallGraphNode(startNode, *sit, 1))
	if(getReachablePath(tFunc2, *sit))
	  {
	    rRefine2.insert(std::pair<std::string,std::string>(it->first,it->second));
	    break;
	  }
  }
  return std::make_pair(rRefine1,rRefine2);
}

std::pair<VarsLoc,VarsLoc> RaceFinder::set_intersect(const VarsLoc S1, const VarsLoc S2)
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


void RaceFinder::extractPossibleRaces()
{
  errs()<<"\n-------------------------------------------------------------------------\n\n";
  //          Debug Print
  //printGlobalRead(0);
  //printGlobalWrite(0);
  //printGlobalRead(1);
  //printGlobalWrite(1);
  
  VarsLoc read1,read2,write1,write2;
  std::pair<VarsLoc,VarsLoc> res1,res2,res3,refineR1,refineR2,refineR3;
  std::string tu1=paInfo[0]->getTUName(), tu2=paInfo[1]->getTUName();
  
  read1=getGlobalRead(0);
  read2=getGlobalRead(1);
  write1=getGlobalWrite(0);
  write2=getGlobalWrite(1);
  
  res1=set_intersect(read1,write2);
  res2=set_intersect(write1,read2);
  res3=set_intersect(write1,write2);
  
  refineR1=refine(res1);
  refineR2=refine(res2);
  refineR3=refine(res3);

  if(refineR1.first.empty() && refineR2.first.empty() && refineR3.first.empty())
    errs()<<"\t!!!No Data Race found!!! \n";
  else{
    errs()<<"\t!!!Possible Data Race Detected!!!\n\n";
    if(!refineR1.first.empty())
      {
	errs()<<"Simultaneous Read(by "<<tu1<<"), Write(by "<<tu2<<") possible\n";
	errs()<<"===================================================================================================\n\n";
	printRaceResult(refineR1.first,0,true);
	printRaceResult(refineR1.second,1,false);
	
      }
    if(!refineR2.first.empty())
      {
	errs()<<"\nSimultaneous Write(by "<<tu1<<"), Read(by "<<tu2<<") possible\n";
	errs()<<"====================================================================================================\n";
	printRaceResult(refineR2.first,0,false);
	printRaceResult(refineR2.second,1,true);	
      }
    if(!refineR3.first.empty())
      {
	errs()<<"\nSimultaneous Write(by "<<tu1<<"), Write(by "<<tu2<<") possible\n";
	errs()<<"======================================================================================================\n";
	printRaceResult(refineR3.first,0,false);
	printRaceResult(refineR3.second,1,false);
      } 
  }      
  errs()<<"\n------------------------------------------------------------------------------------------------------------\n\n";
}
