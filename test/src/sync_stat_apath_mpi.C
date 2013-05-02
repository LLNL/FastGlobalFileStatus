/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Apr 30 2013 DHA: Fix a memory leak
 *        Jul 01 2011 DHA: File created.
 *
 */


extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include <time.h>
}
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "mpi.h"
#include "OpenSSLFileSigGen.h"
#include "Comm/MPICommFabric.h"
#include "SyncFastGlobalFileStat.h"
#include "FgfsTestGetDsoList.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;

enum TestType {
    tt_is_unique = 0,
    tt_is_poorly,
    tt_is_well,
    tt_is_fully,
    tt_is_consistent,
    tt_unknown
};

int
main(int argc, char *argv[])
{

    if (getenv("MPA_TEST_ENABLE_VERBOSE")) {
        MPA_registerMsgFd(stdout, 2);
    }

    int rank, size;
    //
    // Initialize the MPI Communication Fabric
    //
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3) {
        MPA_sayMessage("TEST", true, "Usage: test testType target_exec_path");
        MPA_sayMessage("TEST", true, "    testType: 0 check if isUnique");
        MPA_sayMessage("TEST", true, "    testType: 1 check if isPoorlyDistributed");
        MPA_sayMessage("TEST", true, "    testType: 2 check if isWellDistributed");
        MPA_sayMessage("TEST", true, "    testType: 3 check if isFullyDistributed");
        MPA_sayMessage("TEST", true, "    testType: 4 check if isConsistent");
        return EXIT_FAILURE;
    }

    TestType tt = (TestType) atoi(argv[1]);
    if (tt > tt_is_consistent) {
        MPA_sayMessage("TEST", true, "invalid testType(%d)", tt);
        MPI_Finalize();
        exit(1);
    }

    if (!rank) {
        MPA_sayMessage("TEST", false, "Concurrency: %d", size);
    }

    std::string apath = argv[2];

    //
    // Initialize the synchronous global file stat
    // with MPI Communication Fabric and OpenSSL-based
    // file signiture generator
    //
    bool rc;
    FileSignitureGen *fsig = new OpenSSLFileSignitureGen();
    if (!MPICommFabric::initialize(&argc, &argv)) {
        MPA_sayMessage("TEST",
                       true,
                       "MPICommFabric::initialize returned false");
        return EXIT_FAILURE;
    }
    CommFabric *cfab = new MPICommFabric();


    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                    ****  BEGIN MAIN CHECK ****                            //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////
    uint32_t startTime;
    if (!rank) startTime = stampstart();

    rc = SyncGlobalFileStatus::initialize(fsig, cfab);

    if (!rc) {
        MPA_sayMessage("TEST",
                       true,
                       "SyncGlobalFileStatus::initialize returned false");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    SyncGlobalFileStatus myStat(apath.c_str());
    if (!myStat.triage()) {
        MPA_sayMessage("TEST",
                       true,
                       "triage failed.");
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    switch (tt) {
    case tt_is_unique:
        if (IS_YES(myStat.isUnique())) {
            if(!rank) { 
                MPA_sayMessage("TEST",
                    false,
                    "%s is unique", myStat.getPath());
            }
        }
        else {
            if(!rank) { 
                MPA_sayMessage("TEST",
                    false,
                    "%s is not unique: group count %d", 
                    myStat.getPath(),
                    myStat.getParallelInfo().getNumOfGroups());
            }
        }
        break;
    case tt_is_poorly:
        if (IS_YES(myStat.isPoorlyDistributed())) {
            if(!rank) 
                MPA_sayMessage("TEST",
                    false,
                    "%s is poorly distributed", myStat.getPath());
        }
        else {
            if(!rank) 
                MPA_sayMessage("TEST",
                    false,
                    "%s is not poorly distributed", myStat.getPath());
        }
        break;
    case tt_is_well:
        if (IS_YES(myStat.isWellDistributed())) {
            if(!rank) {
                MPA_sayMessage("TEST",
                    false,
                    "%s is well distributed", myStat.getPath());
            }
        }
        else {
            if(!rank) {
                MPA_sayMessage("TEST",
                    false,
                    "%s is not well distributed", myStat.getPath());
            }
        }
        break;
    case tt_is_fully:
        if (IS_YES(myStat.isFullyDistributed())) {
            if(!rank) {
                MPA_sayMessage("TEST",
                    false,
                    "%s is fully distributed", myStat.getPath());
            }
        }
        else {
            if(!rank) {
                MPA_sayMessage("TEST",
                    false,
                    "%s is not fully distributed", myStat.getPath());
            }    
        }
        break;
    case tt_is_consistent:
        if (IS_YES(myStat.isConsistent())) {
            if(!rank) { 
                MPA_sayMessage("TEST",
                    false,
                    "%s is consistent", myStat.getPath());
            }
        }
        else {
            if(!rank) { 
                MPA_sayMessage("TEST",
                    false,
                    "%s is not consistent", myStat.getPath());
            }
        }
        break;
    default:
        break;
    }
 
    if (!rank) {
      //
      // This will only print when the query above performed the grouping
      //
      std::map<std::string, ReduceDesc> & m = myStat.getParallelInfo().getGroupingMap();
      std::map<std::string, ReduceDesc>::iterator iter;
      for (iter=m.begin(); iter != m.end(); ++iter) {
          MPA_sayMessage("TEST", false, "URI: %s, count: %d, repr: %d", 
              iter->first.c_str(), iter->second.getCount(), iter->second.getFirstRank());
       }
    }

    if (!rank) stampstop(startTime);
    double tmp = 0.0f;
    MPI_Reduce(&accumTime, &tmp, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (!rank) MPA_sayMessage("TEST", false, "max split time: %f", tmp);

    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                    ****  END MAIN CHECK ****                              //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////


    int rnk,sz;
    bool ism;
    cfab->getRankSize(&rnk, &sz, &ism);

    //
    // Delete commFabric and fileGenSig
    //
    delete fsig;
    fsig = NULL;
    delete cfab;
    cfab = NULL;

    MPI_Finalize();
    return EXIT_SUCCESS;
}


