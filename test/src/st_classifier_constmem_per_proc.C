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

    // each process asks for 1MB 
    FileSystemsCriteria sCriteriaOneMB(oneMB);

    // each process asks for 1GB 
    FileSystemsCriteria sCriteriaOneGB(oneGB);

    // each process asks for 2GB 
    FileSystemsCriteria sCriteriaTwoGB(2*oneGB);

    // each process asks for 4GB 
    FileSystemsCriteria sCriteriaFourGB(4*oneGB);

    GlobalFileSystemsStatus sClassifer;

    std::vector<MyMntEntWScore> matchVectorMB;
    std::vector<MyMntEntWScore> matchVectorGB;
    std::vector<MyMntEntWScore> matchVector2GB;
    std::vector<MyMntEntWScore> matchVector4GB;

    //
    // Constant size memory needed per process and rely on
    // default requirements for other parameters
    //
    sClassifer.provideBestFileSystems(sCriteriaOneMB, matchVectorMB);
    sClassifer.provideBestFileSystems(sCriteriaOneGB, matchVectorGB);
    sClassifer.provideBestFileSystems(sCriteriaTwoGB, matchVector2GB);
    sClassifer.provideBestFileSystems(sCriteriaFourGB, matchVector4GB);

    if (!rank) {
        int i = 0;
        
        MPA_sayMessage("TEST", false, 
             "************* %lu Bytes **************************", oneMB*size);
        if (matchVectorMB.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %lu Bytes", 
                oneMB*size);
        }
        else {
            for (i=0; i < matchVectorMB.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneMB*size, 
                    matchVectorMB[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVectorMB[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "\n************* %lu Bytes **************************", oneGB*size);
        if (matchVectorGB.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                oneGB*size);
        }
        else {
            for (i=0; i < matchVectorGB.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    oneGB*size, 
                    matchVectorGB[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVectorGB[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "\n************* %lu Bytes **************************", 2*oneGB*size);
        if (matchVector2GB.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                2*oneGB*size);
        }
        else {
            for (i=0; i < matchVector2GB.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    2*oneGB*size, 
                    matchVector2GB[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector2GB[i].score
                );
             }
        }

        MPA_sayMessage("TEST", false, 
            "\n************* %lu Bytes **************************", 4*oneGB*size);
        if (matchVector4GB.empty()) {
            MPA_sayMessage("TEST", 
                false, 
                "No matching storage found, which has aggregate %luBytes", 
                4*oneGB*size);
        }
        else {
            for (i=0; i < matchVector4GB.size(); i++) {
                MPA_sayMessage("TEST",
                    false,
                    "[Score Rank: %d] for %luBytes is %s (Score: %d)",
                    i,
                    4*oneGB*size, 
                    matchVector4GB[i].mpEntry.getRealMountPointDir().c_str(),
                    matchVector4GB[i].score
                );
             }
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

