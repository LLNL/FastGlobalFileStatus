/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2012, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Feb 23 2012 DHA: File created.
 *
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>

extern "C" {
#include <unistd.h>
}

////////////////////////////////////////////////////////////////////
//
// Macros
// 
//
const char messagePrefix[] = "[TOPOGEN]";


////////////////////////////////////////////////////////////////////
//
// Data Types
// 
//
class Host {
public:
  Host();
  const std::string &getHostName() { return hostName; }
  void setHostName(const std::string h) { hostName = h; }
  const int &getAvailCores() { return availCores; }
  void setAvailCores(const int ac) { availCores = ac; }
  const int &getUsedCores() { return usedCores; }
  void setUsedCores(const int uc) { usedCores = uc; }

  bool incrUsedCores();

private:
  std::string hostName;
  int availCores; 
  int usedCores;
};


class Node { 
public:
  Node();
  ~Node();
  const std::string & getHn() const { return hn; }
  void setHn(const std::string h) { hn = h; }
  const int & getRank() const { return rank; }
  void setRank(const int r) { rank = r; }
  std::vector<Node> & getChildren() { return children; }
  const std::vector<Node> & getChildren() const 
    { return children; }

private:
  std::string hn; 
  int rank;
  std::vector<Node> children;
};


class HostVector {
public:
  HostVector();
  bool next(Node &n);
  void appendToHostlist(const Host& h);

private:
  std::vector<Host> mHosts;
  int curIndex;
};



////////////////////////////////////////////////////////////////////
//
// Static Data
// 
//
static HostVector _BEHost;
static HostVector _CPHost;
static Node _rootNode;



////////////////////////////////////////////////////////////////////
//
// Interfaces
// 
//
Host::Host() : usedCores(0), availCores(0)
{
  
}


bool
Host::incrUsedCores()
{
  bool rc = true;
  usedCores++;
  
  if (usedCores == availCores) {
    rc = false;
  }
  else if (usedCores > availCores) {
    rc = false;
    std::cerr << messagePrefix 
	      << " Used cores are more than available cores!" 
	      << std::endl;
  }
  
  return rc;
}


Node::Node() : hn("NotFilled"), rank(-1)
{
  
}


Node::~Node()
{
  if (!children.empty()) {
    children.clear();
  }    
}


HostVector::HostVector() : curIndex(0) 
{ 

}


void
HostVector::appendToHostlist(const Host& h)
{
  mHosts.push_back(h);
}


bool
HostVector::next(Node &n)
{
  bool rc = true;
  if (mHosts.size() > curIndex) {
    n.setHn(mHosts[curIndex].getHostName());
    n.setRank(mHosts[curIndex].getUsedCores());
    if (!mHosts[curIndex].incrUsedCores()) {
	curIndex++;
      }
  }
  else {
    rc = false;
  }

  return rc;
}


static bool
buildTopology (Node &n, int fanIn, int lvl, int level)
{
  int i;
  bool rc = true;
  if (lvl < level) {

    if ( !_CPHost.next(n)) {
      std::cerr << messagePrefix 
		<< " Not enough CP hosts" 
		<< std::endl;  
      return false;
    }
    for (i=0; i < fanIn; i++) {
      n.getChildren().push_back(Node()); 
    }

    lvl++;
    for (i=0; i < fanIn; i++) {
      rc = buildTopology (n.getChildren()[i], fanIn, lvl, level);
    }
  }
  else {
    if (!_BEHost.next(n)) {
      std::cerr << messagePrefix 
		<< " Not enough BE hosts" 
		<< std::endl; 
      return false;
    }
  }

  return rc;
}


static bool
emitTopology (std::ofstream &tout, const Node &n)
{
  bool rc = true;

  std::vector<Node>::const_iterator i;

  if (!n.getChildren().empty()) {
    tout << n.getHn() << ":" << n.getRank() << " => " 
	 << std::endl;

    for (i=n.getChildren().begin(); 
	    i != n.getChildren().end(); ++i) {
      tout << "\t\t" 
	   << (*i).getHn() << ":" << (*i).getRank() 
	   << std::endl;
    }
    
    tout << "\t\t;" 
	 << std::endl;
  }

  for (i=n.getChildren().begin(); 
          i != n.getChildren().end(); ++i) {
    rc = emitTopology (tout, (*i));
  }
  
  return rc;
}


////////////////////////////////////////////////////////////////////
//
// Main
// 
//


int
main (int argc, char *argv[])
{

  if (argc != 5) {
    std::cerr << messagePrefix 
      << "Usage: myTopoGene BEHostFile CPHostFile TopoSpec OutFile" 
      << std::endl;     

    return EXIT_FAILURE;
  }


  std::string behostfile = argv[1];
  std::string cphostfile = argv[2];
  std::string topo = argv[3];
  std::string topooutfile = argv[4];
  std::ifstream befile(behostfile.c_str());
  std::ifstream cpfile(cphostfile.c_str());
  std::ofstream topofile(topooutfile.c_str());
  size_t found;


  //
  // Parsing topololgy: it has to be fanout^depth
  //
  int fanIn;
  int level; 
  int lvl = 0;

  if ( (found = topo.find_first_of("^"))  == std::string::npos) {
    std::cerr << messagePrefix 
	      << " Ill-formed topology specification." 
	      << std::endl;
    return EXIT_FAILURE;
  }

  fanIn = atoi(topo.substr(0,found).c_str());
  level = atoi(topo.substr(found+1).c_str());


  //
  // Excluding the hostname from the hostlist to use
  // we want to give the tool FEN dedicated access to the node
  //
  char fehostname[128];
  if (gethostname(fehostname, 128) == -1) {
    std::cerr << messagePrefix 
	      << " gethostname returned an error." 
	      << std::endl;
    return EXIT_FAILURE;
  }
  std::string fen(fehostname);


  //
  // Push entries in BE hostlist file into _BEHost
  //
  std::string line;
  if ( befile.is_open() ) {
    while ( !befile.eof() ) {
      line = "";
      getline(befile,line);

      if (line == "") {
	continue;
      }

      found = line.find_first_of(":");
      if (found != std::string::npos) {
        Host aHost;
        aHost.setHostName(line.substr(0, found));
        aHost.setAvailCores(atoi(line.substr(found+1).c_str()));
	if (aHost.getHostName() != fen) {
	  _BEHost.appendToHostlist(aHost);
	}
      }
      else {
        std::cerr << messagePrefix 
		  << " Ill-formed BE hostname field." 
		  << std::endl;
      }
    }
  }
  else {
    std::cerr << messagePrefix 
	      << " " 
	      << behostfile 
	      << " Not opened." 
	      << std::endl;
  }
  befile.close();


  //
  // Push entries in CP hostlist file into _CPHost
  //
  if ( cpfile.is_open() ) {
    Host fenHost;
    fenHost.setHostName(fen);
    fenHost.setAvailCores(1);
    _CPHost.appendToHostlist(fenHost);

    while ( !cpfile.eof() ) {
      line = "";
      getline(cpfile,line);

      if (line == "") {
	continue;
      }

      found = line.find_first_of(":");
      if (found != std::string::npos) {
        Host aHost;
        aHost.setHostName(line.substr(0, found));
	aHost.setAvailCores(atoi(line.substr(found+1).c_str()));
	if (aHost.getHostName() != fen) {
	  _CPHost.appendToHostlist(aHost);
	}
      }
      else {
        std::cerr << messagePrefix
		  << " Ill-formed CP hostname field." 
		  << std::endl;
      }
    }
  }
  cpfile.close();

  
  //
  // Building a topology
  //
  if (!buildTopology(_rootNode, fanIn, lvl, level)) {
    std::cerr << messagePrefix 
	      << " buildToplogy returned an error." 
	      << std::endl;
    return EXIT_FAILURE;
  }


  //
  // Emitting a tree to topofile stream
  //
  //
  // Note that the emtting part is separated from buildTopolgy in case
  // we need a different tree walking alogrithm for emtting 
  // from the building alorithm. For now, we use the
  // same alorithm for both as MRNet appears to 
  // be OK with the emitted topology file. 
  //  
  if (!topofile.is_open() ) {
    std::cerr << messagePrefix 
	      << " Failed to open " 
	      << topo << "."
	      << std::endl;
    return EXIT_FAILURE;
  }

  if (!emitTopology(topofile, _rootNode)) {
    std::cerr << messagePrefix 
	      << " emitTopology failed." 
	      << std::endl;
    return EXIT_FAILURE;   
  }

  topofile << std::endl;
  topofile.close();

  return EXIT_SUCCESS;
}

