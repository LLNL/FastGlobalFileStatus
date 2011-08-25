#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int
main(int argc, char *argv[])
{
  int reqBEs;
  int corePerNode;
  int cpCore2FanoutRatio;
  int FanOutOfTree;
  int heightOfTree;
  int totalBENum;
  int totalCPNum;
  int totalBENodeNum;
  int totalCPNodeNum;
  int totalNodeNum;
  int op;


  if (argc != 5) {
    fprintf(stderr, "Usage: mrnet_node_req op requestedBackEnds corePerNode cpCore2FanoutRatio\n");
    exit(1);
  }

  op = atoi(argv[1]);
  reqBEs = atoi(argv[2]);
  corePerNode = atoi(argv[3]);
  cpCore2FanoutRatio = atoi(argv[4]);
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

  totalBENum = (int) ceil(pow(FanOutOfTree, heightOfTree));
  totalCPNum = (int) ceil((totalBENum - 1) / (FanOutOfTree - 1 )) - 1;
  totalBENodeNum = (int) ceil((double)totalBENum/(double)corePerNode);
  totalCPNodeNum = (int) ceil( ((double)(FanOutOfTree * totalCPNum)/(double)cpCore2FanoutRatio)
			       /(double)corePerNode );
  totalNodeNum = 1 + totalBENodeNum + totalCPNodeNum;

  switch (op) {
  case 1:
      printf("%d\n", heightOfTree);
      break;
  case 2:
      printf("%d\n", FanOutOfTree);
      break;
  case 3:
      printf("%d\n", totalBENum);
      break;
  case 4:
      printf("%d\n", totalCPNum);
      break;
  case 5:
      printf("%d\n", totalBENodeNum);
      break;
  case 6:
      printf("%d\n", totalCPNodeNum);
      break;
  case 7:
      printf("%d\n", totalNodeNum);
      break;
  case 8:
      printf("%d\n", (int)ceil((double)totalCPNum/(double)totalCPNodeNum));
      break;
  default:
      break;
  }

  return EXIT_SUCCESS;
}
