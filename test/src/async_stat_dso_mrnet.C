/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Jul 01 2011 DHA: File created.
 *
 */


extern "C" {
# include <stdio.h>
# include <stdlib.h>
# include <stdint.h>
# include <limits.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <sys/mman.h>
# include <openssl/md5.h>
# include <sys/time.h>
# include <time.h>
}

#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "mrnet/MRNet.h"
#include "Comm/MRNetCommFabric.h"
#include "AsyncFastGlobalFileStat.h"
#include "FgfsTestGetDsoList.h"
#include "FgfsMRNetWrapper.h"


using namespace MRN;
using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;


enum TestType {
    tt_is_unique = 0,
    tt_is_poorly,
    tt_is_well,
    tt_is_fully,
    tt_unknown
};


int main(int argc, char *argv[])
{

    if (getenv("MPA_TEST_ENABLE_VERBOSE")) {
        MPA_registerMsgFd(stdout, 2);
    }

    MRNetCompKind mrnetComponent = mck_unknown;

    if (argc < 5) {
        MPA_sayMessage("TEST", true, 
            "Usage: async_stat_dso_mrnet test_type abs_target_path FE|BE topo_file");
        MPA_sayMessage("TEST", true, "    test_type: 0 check if isUnique");
        MPA_sayMessage("TEST", true, "    test_type: 1 check if isPoorlyDistributed");
        MPA_sayMessage("TEST", true, "    test_type: 2 check if isWellDistributed");
        MPA_sayMessage("TEST", true, "    test_type: 3 check if isFullyDistributed");
        exit(1);
    }

    if (strcmp(argv[3], "FE") == 0) {
        mrnetComponent = mck_frontEnd;
        argv[3][0] = 'B';
    }
    else if (strcmp(argv[3], "BE") == 0) {
        mrnetComponent = mck_backEnd;
    }

    Network *netObj = NULL;
    Stream *channelObj = NULL;
    char *daemonpath = strdup((const char*)argv[0]);
    TestType tt = (TestType) atoi(argv[1]);
    char *topology = argv[4];

    int rc = MRNet_Init(mrnetComponent,
                        &argc,
                        &argv,
                        daemonpath,
                        topology,
                        &netObj,
                        &channelObj);
    if (rc != 0) {
        MPA_sayMessage("TEST", true, "MRNet_Init returns an error.");
        exit(1);
    }

    if (tt > tt_is_fully) {
        MPA_sayMessage("TEST", true, "invalid testType(%d)", tt);
        MRNet_Finalize(mrnetComponent, netObj, channelObj);
        exit(1);
    }

    std::string execPath = argv[2];
    char *pathBuf = NULL;
    char *traverse = NULL;
    unsigned int packedSize = 0;
    std::vector<std::string> dRealpathLibs;
    std::vector<std::string>::const_iterator it;

    if (mrnetComponent == mck_frontEnd) {
        std::vector<std::string> dLibs;
        dLibs.push_back(execPath);
        if (getDependentDSOs(execPath, dLibs) != 0) {
            MPA_sayMessage("TEST",
                           true,
                           "getDependentDSOs returned a neg value.");
            MRNet_Finalize(mrnetComponent, netObj, channelObj);
            exit(1);
        }

        for (it = dLibs.begin(); it != dLibs.end(); it++) {
            char realP[PATH_MAX];
            if (realpath((*it).c_str(), realP)) {
                std::string tmpStr(realP);
                dRealpathLibs.push_back(tmpStr);
                packedSize += (tmpStr.size() + 1);
            }
        }

        pathBuf = (char *) malloc(packedSize);
        traverse = pathBuf;
        for (it = dRealpathLibs.begin(); it != dRealpathLibs.end(); it++) {
            memcpy(traverse, (*it).c_str(), (*it).size()+1);
            traverse += ((*it).size() + 1);
        }

        if (channelObj->send(MRNetHelloTag+1, "%auc",
                         (unsigned char *) pathBuf, packedSize) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send returns an error.");
            MRNet_Finalize(mrnetComponent, netObj, channelObj);
            exit(1);
        }

        channelObj->flush();
    }
    else {
        PacketPtr recvP;
        int tag;

        channelObj->recv(&tag, recvP);
        if (recvP->unpack("%auc", &pathBuf, &packedSize) == -1) {
            MPA_sayMessage("TEST",
                           true,
                           "Stream send returns an error.");
            MRNet_Finalize(mrnetComponent, netObj, channelObj);
            exit(1); 
        }

        traverse = pathBuf;
        while (traverse < (pathBuf + packedSize)) {
            dRealpathLibs.push_back(std::string(traverse));
            traverse += (strlen(traverse) + 1);
        }
    }

    free(pathBuf);

    if (!MRNetCommFabric::initialize((void *)netObj, (void *)channelObj)) {
        MPA_sayMessage("TEST",
                       true,
                       "MRNetCommFabric::initialize returned false");
        MRNet_Finalize(mrnetComponent, netObj, channelObj);
        exit(1);
    }

    CommFabric *cfab = new MRNetCommFabric();


    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          BEGIN MAIN CHECK                                 //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////
    uint32_t startTime;
    if (mrnetComponent == mck_frontEnd) startTime = stampstart();

    rc = AsyncGlobalFileStatus::initialize(cfab);

    if (!rc) {
        if (mrnetComponent == mck_frontEnd) {
            MPA_sayMessage("TEST",
                       true,
                       "AsyncGlobalFileStatus::initialize returned false");
        }

        MRNet_Finalize(mrnetComponent, netObj, channelObj);
        exit(1);
    }

    int nHit = 0;
    for (it = dRealpathLibs.begin(); it != dRealpathLibs.end(); it++) {
        AsyncGlobalFileStatus myStat((*it).c_str());
        if (mrnetComponent == mck_frontEnd) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("TEST", false, "%s", (*it).c_str());
            }
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
        default:
            break;
        }

    }
    if (mrnetComponent == mck_frontEnd) stampstop(startTime);
    ///////////////////////////////////////////////////////////////////////////////
    //                                                                           //
    //                          END MAIN CHECK                                   //
    //                                                                           //
    ///////////////////////////////////////////////////////////////////////////////


    int rnk,sz;
    bool ism;
    float uniqPercent;

    cfab->getRankSize(&rnk, &sz, &ism);
    uniqPercent = 100.0 * ((float)(nHit) / (float) (dRealpathLibs.size()));

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
        default:
            break;
        }
    }

    MRNet_Finalize(mrnetComponent, netObj, channelObj);

    delete cfab;
    cfab = NULL;

    return EXIT_SUCCESS;
}

