/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2012, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Feb 22 2012 DHA: File created.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int
main(int argc, char *argv[])
{
  int reqBEs;
  int corePerNode;
  int cpBranchPerCore; 
  int FanOutOfTree;    
  int heightOfTree;    
  int nCorePerCp;      
  int totalBENum;      
  int totalCPNum;
  int totalBENodeNum;
  int cpsPerNode;
  int totalCPOnlyNodeNum;
  int totalNodeNum;
  int op;

  if (argc != 5) {
    fprintf(stderr,
      "Usage: mrnet_node_req op numBackEnds corePerNode branchPerCore\n");
    return EXIT_FAILURE;
  }

  op = atoi(argv[1]);
  reqBEs = atoi(argv[2]);
  corePerNode = atoi(argv[3]);
  cpBranchPerCore = atoi(argv[4]);
  FanOutOfTree = 1;
  heightOfTree = 1;

  if (reqBEs <= 256) {
    FanOutOfTree = (int) ceil(sqrt((double)reqBEs));
    heightOfTree = 2;
  }
  else if ((reqBEs > 256) && (reqBEs <= 4096)) {
    FanOutOfTree = (int) ceil(cbrt((double)reqBEs));
    heightOfTree = 3;
  }
  else {
    FanOutOfTree = (int) ceil(sqrt(sqrt((double)reqBEs)));
    heightOfTree = 4;
  }

  nCorePerCp = (int) ceil((double)FanOutOfTree/(double)cpBranchPerCore);
  totalBENum = (int) ceil(pow(FanOutOfTree, heightOfTree));
  totalBENodeNum = (int) ceil((double)totalBENum /(double) corePerNode);
  totalCPNum = (int) ceil((totalBENum - 1) / (FanOutOfTree - 1 )) - 1;
  cpsPerNode = corePerNode / nCorePerCp;
  totalCPOnlyNodeNum = (int) ceil((double)totalCPNum / (double)cpsPerNode);
  totalNodeNum = 1 + totalBENodeNum + totalCPOnlyNodeNum;



  switch (op) {
  case 1:
    /*
     * op=1: height of the tree 
     */
    fprintf(stdout, "%d\n", heightOfTree);
    break;

  case 2:
    /*
     * op=2: fan-out (==fan-out)
     */
    fprintf(stdout, "%d\n", FanOutOfTree);
    break;

  case 3:
    /*
     * op=3: back-end count
     */
    fprintf(stdout, "%d\n", totalBENum);
    break;
    
  case 4: 
    /*
     * op=4: communication process count
     */ 
    fprintf(stdout, "%d\n", totalCPNum);
    break;

  case 5:
    /*
     * op=5: required number of nodes for back-ends
     */  
    fprintf(stdout, "%d\n", totalBENodeNum);
    break;

  case 6:
    /*
     * op=6: number of cores per node
     */ 
    fprintf(stdout, "%d\n", corePerNode);
    break;


  case 7:
    /*
     * op=7: required number of nodes for comm. processes
     */ 
    fprintf(stdout, "%d\n", totalCPOnlyNodeNum);
    break;

  case 8:
    /*
     * op=8: required number of nodes
     */ 
    fprintf(stdout, "%d\n", totalNodeNum);
    break;


  case 9:
    /*
     * op=8: required number of cores per comm. processes
     */ 
    fprintf(stdout, "%d\n", nCorePerCp);
    break;

  default:
   fprintf(stderr,"Unknown operator"); 
    break;
  }

  return EXIT_SUCCESS;
}
