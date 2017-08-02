/****************************************************/
/*          All rights reserved                     */
/*          masud.abunaser@mdh.se                   */
/****************************************************/
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <deque>

using clang::HeaderSearchOptions;
using clang::tooling::CommonOptionsParser;
using std::set;
using std::string;
using std::vector;
using std::deque;
using namespace llvm;

//namespace {


class Node{
public:
  Node(){}
  ~Node(){
    children.clear();
    parents.clear();
  }
  Node(std::string f, int i){id=i;file=f;} 
  
  void addParent(Node *n){parents.insert(n);}
  void addChild(Node *n){children.insert(n);}
  bool emptyParents(){return parents.empty();}
  bool emptyChildren(){return children.empty();}
  const std::string getName() const{return file;}

  std::set<Node*>::iterator parentIterBegin(){return parents.begin();}
  std::set<Node*>::iterator parentIterEnd(){return parents.end();}
  // void decrOutEdge() {out--;}
  //void incrOutEdge(){out++;}
  int getId() const{return id;}
  int getOutDegree(){return children.size();} 
  bool operator<(const Node &o) const {
        return id < o.id ;
    }

  void showParents()
  {
    std::cout<<"Parents of "<<file<<"\n";
    std::set<Node*>::iterator i=parentIterBegin(), e=parentIterEnd();
    if(i==e) std::cout<<" No Parents \n";
    while(i!=e)
      {
	std::cout<<"\n"<<(*i)->getName()<<"\n";
	i++;
      }
  }  
private:
  int id;
  //  int out;
  std::string file;
  std::set<Node*> children;
  std::set<Node*> parents;
};

struct deleteNode
{
    void operator()(Node*& n) // important to take pointer by reference!
    { 
      delete n;
      n = NULL;  
    }
};

class RepoGraph
{
    std::map<std::string, Node*> nodes;
    std::map<std::string, bool> sysHeaders;
    std::vector<Node*> headers;
    int id;
    RepoGraph(const RepoGraph &);
    RepoGraph &operator= (const RepoGraph &);
public: 
    RepoGraph(): id (0) { }
    ~RepoGraph(){ 
      for_each(headers.begin(), headers.end(), deleteNode());
      vector<Node*>::iterator new_end = remove(headers.begin(), headers.end(), static_cast<Node*>(NULL));
      headers.erase(new_end, headers.end());
      nodes.clear();
    }

    void addSysHeader(const std::string &h)
    {
      sysHeaders.insert (std::make_pair (h, 1));
    }  
    bool sysHeader(const std::string &h)
    {
      std::map<std::string, bool>::iterator it=sysHeaders.find(h);
      return (it != sysHeaders.end()); 
    }
    void recordHeaderDep(const std::string &from,
                     const std::string &to) {
      std::map<std::string, Node*>::iterator it;
      Node *fromNode, *toNode;
      it=nodes.find(from);
      if(it != nodes.end()) fromNode=it->second;
      else {
	fromNode=new Node(from,id++); 
	nodes.insert (std::make_pair (from, fromNode));
	headers.push_back(fromNode);  //check
      }
      it=nodes.find(to);    
      if(it != nodes.end()) toNode=it->second;
      else {
	toNode=new Node(to, id++);
	nodes.insert (std::make_pair (to, toNode));  
	headers.push_back(toNode);
      }		
      toNode->addChild(fromNode);
      fromNode->addParent(toNode);
    }

   void viewGraph()
   {
     std::deque<Node*> toBeVisited;
     std::deque<Node*>::iterator qit;

     std::vector<int> nodesOutDegree(headers.size());
     std::vector<int> visited(headers.size());
     std::vector<Node*>::iterator hit=headers.begin(), eit=headers.end();
     int num=0;
     while(hit!=eit)
     {
       int out=(*hit)->getOutDegree();
       nodesOutDegree[(*hit)->getId()]=out;
       visited[(*hit)->getId()]=0;
       if(out==0) {
	 toBeVisited.push_back(*hit);
	 visited[(*hit)->getId()]++;
	 //std::cout<<"\n\n Queue has "<<(*hit)->getName()<<"\n";
	 num++;
       }
       hit++;
     }  

     while(!toBeVisited.empty())
     {
       
       qit=toBeVisited.begin();
       if(nodesOutDegree[(*qit)->getId()]==0)
       {	 
	 std::cout<<"\n"<<(*qit)->getName();
	 if(sysHeader((*qit)->getName())) std::cout<<"(system header)";
         std::set<Node*>::iterator i=(*qit)->parentIterBegin(), e=(*qit)->parentIterEnd();
	 toBeVisited.pop_front();	 
	 num--;
	 if(i==e){ std::cout<<"\n\t\t\t<---\n";}
	 while(i!=e)
	 {
	   std::cout<<"\n\t\t\t<---"<<(*i)->getName()<<"\n";
	   //std::cout<<"Parent Id :"<<(*i)->getId()<<"\n";
	   //std::cout<<" Out degree (before): "<<nodesOutDegree[(*i)->getId()]<<"\n";
	   nodesOutDegree[(*i)->getId()]--;
	   //std::cout<<" Out degree (after): "<<nodesOutDegree[(*i)->getId()]<<"\n";
	   //std::cout<<" Visit status: "<< visited[(*i)->getId()] <<"\n";
	   if(visited[(*i)->getId()]<1) // Not yet scheduled to visit
	     { 
	       toBeVisited.push_back(*i);
	       visited[(*i)->getId()]++;
	       num++;
	     }  
	   i++;
	 }  
         //std::cout<<"Queue Elements: "<<num<<"\n";
       } 
       else if(nodesOutDegree[(*qit)->getId()]>0 && visited[(*qit)->getId()]<3) {
         //std::cout<<"Reentry to queue "<<(*qit)->getName()<<"\n";
	 visited[(*qit)->getId()]++;
	 toBeVisited.pop_front();
	 toBeVisited.push_back(*qit);
	 
       }
       else if(visited[(*qit)->getId()]>=3)
	 {
	   std::cout<<"\n\nCircular dependency found\n\n";
	   return;
	 }
     }
   }
};

class PPCallbacksExtend : public clang::PPCallbacks
{
    clang::SourceManager &_sm;
    RepoGraph &_inc;

public:
    PPCallbacksExtend (clang::SourceManager &sm,
                 RepoGraph &inc)
        : _sm (sm), _inc (inc)
        {}

    // InclusionDirective
    virtual void InclusionDirective (clang::SourceLocation HashLoc,
                                     const clang::Token &,
                                     clang::StringRef,
                                     bool IsAngled,
                                     clang::CharSourceRange,
                                     const clang::FileEntry *File,
                                     clang::StringRef,
                                     clang::StringRef,
                                     const clang::Module *) {
      
      const char *path = realpath (_sm.getBufferName(HashLoc).str().c_str(), NULL); 
      if(_sm.isInSystemHeader(HashLoc)) 
	{
	  if (path != NULL) _inc.addSysHeader(path);
	  return;
	}    
      
      if (path == NULL) 
	std::cout << "Invalid path " << _sm.getBufferName (HashLoc).str().c_str()<<"\n";	 
      
      if (File != NULL) {
	const char *fpath = realpath (File->getName().str().c_str(), NULL);
	if (fpath == NULL)  std::cout << "Invalid path " << File->getName().str().c_str()<<"\n";
		
	if ((fpath != NULL)&& (path != NULL)) {
	  _inc.recordHeaderDep (fpath, path);
	}
      }
    }
  // InclusionDirective
};

//}  // namespace

class HeaderAnalysisAction : public clang::PreprocessorFrontendAction {
 public:
  HeaderAnalysisAction(RepoGraph &inc)
    : _inc (inc) { }

  void ExecuteAction() override;
  // void EndSourceFileAction() override;
 private:
  //clang::CompilerInstance *_ci;
  RepoGraph &_inc;
  std::string main_source_file_;
  // Maps file names to their contents as read by Clang's source manager.
  std::set<std::string> source_file_paths_;
};

void HeaderAnalysisAction::ExecuteAction() {
  clang::CompilerInstance &ci=getCompilerInstance();
    clang::SourceManager &sm (ci.getSourceManager ());
    //PPCallbacks *ppc = new PPCallbacks (sm, _inc);
    clang::Preprocessor &pp (ci.getPreprocessor ());
    
    std::unique_ptr<PPCallbacksExtend> ppc=llvm::make_unique<PPCallbacksExtend>(sm,_inc);
    pp.addPPCallbacks (std::move(ppc));
    //pp.addPPCallbacks (ppc);
    clang::Token token;
    pp.EnterMainSourceFile ();
    do {
      pp.Lex (token);
    }
    while (token.isNot (clang::tok::eof));
}

class IncAnalFrontendActionFactory : public clang::tooling::FrontendActionFactory {

    RepoGraph &_inc;

public:
    IncAnalFrontendActionFactory (RepoGraph &inc)
    : _inc (inc) { }

    virtual clang::FrontendAction *create () {
        return new HeaderAnalysisAction(_inc);
    }
};

/*
static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  llvm::cl::OptionCategory category("Header Dependency Analysis Tool");
  CommonOptionsParser options(argc, argv, category);
  RepoGraph repo;
  clang::tooling::ClangTool tool(options.getCompilations(),
                                 options.getSourcePathList());
  IncAnalFrontendActionFactory act (repo);
  tool.run(&act);
  repo.viewGraph();
}

*/
