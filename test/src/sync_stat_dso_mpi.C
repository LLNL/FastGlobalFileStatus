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

    std::string execPath = argv[2];
    char *pathBuf = NULL;
    int packedSize = 0;
    std::vector<std::string> dRealpathLibs;
    std::vector<std::string>::const_iterator it;

    if (!rank) {
        std::vector<std::string> dLibs;
        dLibs.push_back(execPath);
        if (getDependentDSOs(execPath, dLibs) != 0) {
            MPA_sayMessage("TEST",
                           true,
                           "getDependentDSOs returned a neg value.");

            MPI_Finalize();
            return EXIT_FAILURE;
        }

        for (it = dLibs.begin(); it != dLibs.end(); it++) {
            char realP[PATH_MAX];
            if (realpath((*it).c_str(), realP)) {
                std::string tmpStr(realP);
                dRealpathLibs.push_back(tmpStr);
                packedSize += (tmpStr.size() + 1);
            }
        }
    }

    MPI_Bcast(&packedSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    pathBuf = (char *) malloc(packedSize);
    if (!pathBuf) {
        MPA_sayMessage("TEST",
                       true,
                       "malloc returned null.");
        MPI_Finalize();  
        return EXIT_FAILURE;
    }

    char *traverse = NULL;
    if (!rank) {
        traverse = pathBuf;
        for (it = dRealpathLibs.begin(); it != dRealpathLibs.end(); it++) {
            memcpy(traverse, (*it).c_str(), (*it).size()+1);
            traverse += ((*it).size() + 1);
        }
    }

    MPI_Bcast(pathBuf, packedSize, MPI_CHAR, 0, MPI_COMM_WORLD);
    traverse = pathBuf;

    if (rank) {
        while (traverse < (pathBuf + packedSize)) {
            dRealpathLibs.push_back(std::string(traverse));
            traverse += (strlen(traverse) + 1);
        }
    }

    free (pathBuf);

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

    int nHit = 0;
    for (it = dRealpathLibs.begin(); it != dRealpathLibs.end(); it++) {
        SyncGlobalFileStatus myStat((*it).c_str());
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
                nHit++;
            }
            break;
        case tt_is_poorly:
            if (IS_YES(myStat.isPoorlyDistributed())) {
                nHit++;
            }
            break;
        case tt_is_well:
            if (IS_YES(myStat.isWellDistributed())) {
                nHit++;
            }
            break;
        case tt_is_fully:
            if (IS_YES(myStat.isFullyDistributed())) {
                nHit++;
            }
            break;
        case tt_is_consistent:
            if (IS_YES(myStat.isConsistent())) {
                nHit++;
            }
            break;
        default:
            break;
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

    float uniqPercent = 100.0 * ((float)(nHit) / (float) (dRealpathLibs.size()));
    if (ism) {
        switch (tt) {
        case tt_is_unique:
            MPA_sayMessage("TEST",
                       false,
                       "%d percent of %d DSOs are served uniquely.",
                       (int) uniqPercent, dRealpathLibs.size());
            break;
        case tt_is_poorly:
            MPA_sayMessage("TEST",
                       false,
                       "%d percent of %d DSOs are poorly distributed.",
                       (int) uniqPercent, dRealpathLibs.size());
            break;
        case tt_is_well:
            MPA_sayMessage("TEST",
                       false,
                       "%d percent of %d DSOs are well distributed.",
                       (int) uniqPercent, dRealpathLibs.size());
            break;
        case tt_is_fully:
            MPA_sayMessage("TEST",
                       false,
                       "%d percent of %d DSOs are fully distributed.",
                       (int) uniqPercent, dRealpathLibs.size());
            break;
        case tt_is_consistent:
            MPA_sayMessage("TEST",
                       false,
                       "%d percent of %d DSOs are consistent.",
                       (int) uniqPercent, dRealpathLibs.size());
            break;
        default:
            break;
        }
    }


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


