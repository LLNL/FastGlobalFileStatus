/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Sep 19 2011 DHA: File created.
 *
 */

#include <stdlib.h>
#include "mpi.h"
#include "Comm/MPICommFabric.h"
#include "StorageClassifier.h"
#include <vector>

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::StorageInfo;

const nbytes_t oneMB = 1024*1024;
const nbytes_t oneGB = 1024*1024*1024;
//const nbytes_t oneTB = 1024*1024*1024*1024;
const nbytes_t oneTB = 1099511627776;

int main(int argc, char *argv[])
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
    CommFabric *cfab = new MPICommFabric();
    GlobalFileSystemsStatus::initialize(cfab);

    GlobalFileSystemsStatus sClassifer;

    // each process asks for 1MB except for rank 1
    FileSystemsCriteria sCriteria1(oneMB);
    FileSystemsCriteria sCriteria2(oneMB);
    FileSystemsCriteria sCriteria3(oneMB);
    FileSystemsCriteria sCriteria4(oneMB);
    FileSystemsCriteria sCriteria5(oneMB);
    FileSystemsCriteria sCriteria6(oneMB);
    if (rank == 1) {
        sCriteria1.setSpaceRequirement(4*oneGB,0);
        sCriteria2.setSpaceRequirement(8*oneGB,0);
        sCriteria3.setSpaceRequirement(16*oneGB,0);
        sCriteria4.setSpaceRequirement(32*oneGB,0);
        sCriteria5.setSpaceRequirement(oneTB,0);
        sCriteria6.setSpaceRequirement(4*oneTB,0);
    }

    std::vector<MyMntEntWScore> matchVector1;
    std::vector<MyMntEntWScore> matchVector2;
    std::vector<MyMntEntWScore> matchVector3;
    std::vector<MyMntEntWScore> matchVector4;
    std::vector<MyMntEntWScore> matchVector5;
    std::vector<MyMntEntWScore> matchVector6;

    //
    // Constant size memory needed per process and rely on
    // default requirements for other parameters
    //
    sClassifer.provideBestFileSystems(sCriteria1, matchVector1);
    sClassifer.provideBestFileSystems(sCriteria2, matchVector2);
    sClassifer.provideBestFileSystems(sCriteria3, matchVector3);
    sClassifer.provideBestFileSystems(sCriteria4, matchVector4);
    sClassifer.provideBestFileSystems(sCriteria5, matchVector5);
    sClassifer.provideBestFileSystems(sCriteria6, matchVector6);

    if (!rank) {
        int i = 0;
        
        MPA_sayMessage("TEST", false, 
             "************* %lu Bytes **************************", 
             oneMB*(size-1)+4*oneGB);
        if (matchVector1.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %ludBytes", 
                oneMB*(size-1)+4*oneGB);
        }
        else {
            for (i=0; i < matchVector1.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+4*oneGB, 
                    matchVector1[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector1[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************",
            oneMB*(size-1)+8*oneGB);
        if (matchVector2.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                oneMB*(size-1)+8*oneGB);
        }
        else {
            for (i=0; i < matchVector2.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+8*oneGB, 
                    matchVector2[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector2[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************", 
            oneMB*(size-1)+16*oneGB);
        if (matchVector3.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                oneMB*(size-1)+8*oneGB);
        }
        else {
            for (i=0; i < matchVector3.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+8*oneGB, 
                    matchVector3[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector3[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************",
            oneMB*(size-1)+32*oneGB);
        if (matchVector4.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                oneMB*(size-1)+32*oneGB);
        }
        else {
            for (i=0; i < matchVector4.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+32*oneGB, 
                    matchVector4[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector4[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************",
            oneMB*(size-1)+32*oneGB);
        if (matchVector4.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                oneMB*(size-1)+32*oneGB);
        }
        else {
            for (i=0; i < matchVector4.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+32*oneGB, 
                    matchVector4[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector4[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************",
            oneMB*(size-1)+oneTB);
        if (matchVector5.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %lu Bytes", 
                oneMB*(size-1)+oneTB);
        }
        else {
            for (i=0; i < matchVector5.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %lu Bytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+oneTB, 
                    matchVector5[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector5[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "************* %lu Bytes **************************",
            oneMB*(size-1)+4*oneTB);
        if (matchVector6.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %lu Bytes", 
                oneMB*(size-1)+4*oneTB);
        }
        else {
            for (i=0; i < matchVector6.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %lu Bytes is %s (Score: %d)",
                    i,
                    oneMB*(size-1)+4*oneTB, 
                    matchVector6[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector6[i].score
                );
             }
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

