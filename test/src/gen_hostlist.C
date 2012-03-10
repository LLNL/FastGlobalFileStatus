/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Jul 11 2011 DHA: File created.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <math.h>

#include <vector>
#include <string>

#define HOSTNAME_LENGTH 128

int main(int argc, char *argv[])
{
    char hostn[HOSTNAME_LENGTH];
    char *hostsBuf;
    int rank;
    int size;
    int nCoresPerNode = 0;
    int corePerCp = 0;
    int nNodeForCpOnly = 0;
    int nCoreBEN = 0;
    char *hostFile;
    char *hostListFN;
    char *hostListFNCp;
    FILE *fptrCp;
    FILE *fptr;
    FILE *fp;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 8) {
        if (!rank) {
            fprintf(stderr,
	       "Usage: gen-hostlist hostFile hostFileNameBe "
               "hostFileNameCp nCoresPerNode nNodeForCpOnly"
	       "corePerCp nCoreBEN\n");
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    hostFile = argv[1];
    hostListFN = argv[2];
    hostListFNCp = argv[3];
    nCoresPerNode = atoi(argv[4]);    
    nNodeForCpOnly = atoi(argv[5]);
    corePerCp = atoi(argv[6]);
    nCoreBEN = atoi(argv[7]);

    if (gethostname(hostn, HOSTNAME_LENGTH) < 0) {
        if (!rank) {
            fprintf(stderr,
                "gethostname returned neg\n");
        }
        MPI_Finalize();
	return EXIT_FAILURE;
    }

    if (!rank) {
      	fptrCp = fopen(hostListFNCp, "w");
        fptr = fopen(hostListFN, "w");
        fp = fopen(hostFile, "w");
        hostsBuf = (char *) malloc(HOSTNAME_LENGTH*size*sizeof(char));
    }

    MPI_Gather ((void*)hostn,
                128,
                MPI_CHAR,
                (void*) hostsBuf,
                128,
                MPI_CHAR,
                0,
                MPI_COMM_WORLD);

    if (!rank) {
        char *t = hostsBuf;
        int i=0;
        std::vector<std::string> gatheredHn;
        for (i=0; i < 128*size; i += 128) {
            gatheredHn.push_back(std::string(t));
            t += 128;
        }

        std::vector<std::string>::iterator iter;
	i=0;
        for (iter = gatheredHn.begin(); iter != gatheredHn.end(); iter++) {
          fprintf(fp, "%s:%d\n", (*iter).c_str(), nCoresPerNode);
	  if ((i > 0) && (i <= nNodeForCpOnly)) {
            //
            // CP only nodes
            //
	    fprintf(fptrCp, 
		    "%s:%d\n", 
		    (*iter).c_str(), 
		    (int) floor ((double)nCoresPerNode/(double)corePerCp)); 
	  }
	  else {
            //
            // BE nodes
            //
            fprintf(fptr, 
		    "%s:%d\n", 
		    (*iter).c_str(), nCoreBEN);
	  }
	  i++;
        }

	fprintf(fp, "\n");
        fprintf(fptr, "\n");
        fprintf(fptrCp, "\n");

        fflush(fp);
        fflush(fptr);
        fflush(fptrCp);

        fclose(fp);
	fclose(fptr);
	fclose(fptrCp);	
    }

   MPI_Finalize();

   return EXIT_SUCCESS;
}
