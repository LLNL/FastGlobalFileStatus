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

    //
    // Mount Point Classifier runs as part of initialization as well.
    //
    GlobalFileSystemsStatus::initialize(cfab);

    const MountPointsClassifier &mpClassifier 
        = GlobalFileSystemsStatus::getMountPointsClassifier();

    std::map<std::string, GlobalProperties>::const_iterator iter;
    for (iter = mpClassifier.getGlobalMountpointsMap().begin();
             iter != mpClassifier.getGlobalMountpointsMap().end(); ++iter) {

        //
        // iter->first mount point name
        // iter->second GlobalProperties
        //
        const std::string &curMountPoint = iter->first;
        const GlobalProperties &globalProperties = iter->second;

        if (!rank) {
            MPA_sayMessage("TEST", false,
                "[PATH: %s ]", 
                curMountPoint.c_str());
            
            //
            // High-level Distribution Properties
            //

            if (IS_YES(globalProperties.isUnique())) {
                MPA_sayMessage("TEST", false, "isUnique");
            }
            if (IS_YES(globalProperties.isPoorlyDistributed())) {
                MPA_sayMessage("TEST", false, "isPoorlyDistributed");
            }
            if (IS_YES(globalProperties.isWellDistributed())) {
                MPA_sayMessage("TEST", false, "isWellDistributed");
            }
            if (IS_YES(globalProperties.isFullyDistributed())) {
                MPA_sayMessage("TEST", false, "isFullyDistributed");
            }

            //
            // File System Properties
            //
            MPA_sayMessage("TEST", false,
                "Serial Access Speed: %d", 
                globalProperties.getFsSpeed());
            MPA_sayMessage("TEST", false,
                "Scalability of underlying file system: %d", 
                globalProperties.getFsScalability());
            MPA_sayMessage("TEST", false,
                "number of processes per server: %d", 
                globalProperties.getDistributionDegree());
            MPA_sayMessage("TEST", false,
                "FS type: %d", 
                globalProperties.getFsType());
            MPA_sayMessage("TEST", false,
                "FS type: %s", 
                globalProperties.getFsName().c_str());
            MPA_sayMessage("TEST", false,
                "*****     *****     *****     *****     *****     *****");
         }

         if (rank % 8 == 0) {

            //
            // Low-level Grouping Info
            //
            MPA_sayMessage("TEST", false,
                "[MPI rank: %d] Number of eq. groups: %d", 
                rank, globalProperties.getParallelDescriptor().getNumOfGroups());
            MPA_sayMessage("TEST", false,
                "[MPI rank: %d] Group Id: %u", 
                rank, globalProperties.getParallelDescriptor().getGroupId());
            MPA_sayMessage("TEST", false,
                "[MPI rank: %d] Rep Id: %u", 
                rank, globalProperties.getParallelDescriptor().getRepInGroup());
            MPA_sayMessage("TEST", false,
                "[MPI rank: %d] Rank in Group: %u", 
                rank, globalProperties.getParallelDescriptor().getRankInGroup());
            MPA_sayMessage("TEST", false,
                "[MPI rank: %d] URI string: %s", 
                rank, globalProperties.getParallelDescriptor().getUriString().c_str());

            MPA_sayMessage("TEST", false,
                "*****     *****     *****     *****     *****     *****");
        }
    }

    MPI_Finalize();

    return EXIT_SUCCESS;
}

