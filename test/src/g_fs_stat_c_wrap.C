/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2012, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        May 03 2012 DHA: File created.
 *
 */

#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "Comm/MPICommFabric.h"
#include "StorageClassifier.h"
#include <vector>
#include "g_fs_stat_c_wrap.h"


using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::StorageInfo;


extern "C" int
g_fs_status_initialize()
{
  int rc = 0;
  CommFabric *cfab = new MPICommFabric();
  if (getenv("MPA_TEST_ENABLE_VERBOSE")) {
    MPA_registerMsgFd(stdout, 2);
  }


  if (!GlobalFileSystemsStatus::initialize(cfab)) {
    rc = -1;
  }

  return rc;
}


extern "C" int
g_fs_status_provide_best_fs(unsigned long bytes, unsigned long bytes_to_free,
			    matched_entry_t **match_array, int *array_size)
{
  std::vector<MyMntEntWScore> matchVector;
  GlobalFileSystemsStatus fsClassifier;
  int ix = 0;

  FileSystemsCriteria::SpaceRequirement amountNeeded 
    = (FileSystemsCriteria::SpaceRequirement) bytes;
  FileSystemsCriteria::SpaceRequirement amountToFree 
    = (FileSystemsCriteria::SpaceRequirement) bytes_to_free;  
  FileSystemsCriteria criteria(amountNeeded, amountToFree);
  
  fsClassifier.provideBestFileSystems(criteria, matchVector);
  
  (*array_size) = (int) matchVector.size();
  (*match_array) = (matched_entry_t *) malloc ((*array_size) * sizeof(**match_array));

  for (ix = 0; ix < matchVector.size(); ++ix) {
    (*match_array)[ix].mount_point_path 
      = strdup(matchVector[ix].mpEntry.getRealMountPointDir().c_str());
    
    (*match_array)[ix].score = matchVector[ix].score;
  }

  return ((*array_size)? 0: -1);
}
