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
    int nCps = 0;
    int nNodeForCp = 0;
    char *hostListFN;
    char *hostListFNCp;
    FILE *fptrCp;
    FILE *fptr;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 6) {
        if (!rank) {
            fprintf(stderr,
	       "Usage: gen-hostlist hostFileNameBe hostFileNameCp nCoresPerNode nNodeForCp nCps\n");
        }
        MPI_Finalize();
        exit(1);
    }

    hostListFN = argv[1];
    hostListFNCp = argv[2];
    nCoresPerNode = atoi(argv[3]);    
    nNodeForCp = atoi(argv[4]);
    nCps = atoi(argv[5]);

    if (gethostname(hostn, HOSTNAME_LENGTH) < 0) {
        if (!rank) {
            fprintf(stderr,
                "gethostname returned neg\n");
        }
        MPI_Finalize();
        exit(1);
    }

    if (!rank) {
      	fptrCp = fopen(hostListFNCp, "w");
        fptr = fopen(hostListFN, "w");
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
	  if (i >= (size - nNodeForCp)) {
	    fprintf(fptrCp, "%s:%d\n", (*iter).c_str(), (int) ceil((double)nCps/(double)nNodeForCp));
	  }
	  else {
              fprintf(fptr, "%s:%d\n", (*iter).c_str(), nCoresPerNode);
	  }
	  i++;
        }

	fclose(fptrCp);	
	fclose(fptr);
    }

   MPI_Finalize();

   return EXIT_SUCCESS;
}
