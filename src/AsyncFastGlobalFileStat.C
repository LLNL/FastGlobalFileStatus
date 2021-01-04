/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        June 22 2011 DHA: File created.
 *
 */

extern "C" {
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include "bloom.h"
}

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "AsyncFastGlobalFileStat.h"


using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//


MountPointsClassifier
AsyncGlobalFileStatus::mMpClassifier;


///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//


///////////////////////////////////////////////////////////////////
//
//  class AsyncGlobalFileStatus
//
//


AsyncGlobalFileStatus::AsyncGlobalFileStatus(const char *pth)
    : GlobalFileStatusAPI(pth), GlobalFileStatusBase()
{

}


AsyncGlobalFileStatus::AsyncGlobalFileStatus(const char *pth, const int value)
    : GlobalFileStatusAPI(pth), GlobalFileStatusBase(value)
{

}


AsyncGlobalFileStatus::~AsyncGlobalFileStatus()
{

}


bool
AsyncGlobalFileStatus::initialize(CommLayer::CommFabric *c)
{
    mMpClassifier.runClassification(c);
    return true;
}


bool 
AsyncGlobalFileStatus::printMpClassifier()
{
    std::map<std::string, GlobalProperties>::const_iterator i;
    for (i = mMpClassifier.mAnnoteMountPoints.begin(); 
	 i !=mMpClassifier.mAnnoteMountPoints.end(); ++i) {
        MPA_sayMessage("AsyncGlobalFileStatus",
		       false,
		       "PATH: %s", i->first.c_str());
        MPA_sayMessage("AsyncGlobalFileStatus",
		       false,
		       "URI: %s", i->second.getParallelDescriptor().getUriString().c_str());
	MPA_sayMessage("AsyncGlobalFileStatus",
		       false,
		       "Grouping: Rank(%d) numGroups(%d) groupId(%d) Size(%d) GlobalMaster(%d) ",
		       i->second.getParallelDescriptor().getRank(),
		       i->second.getParallelDescriptor().getNumOfGroups(),
		       i->second.getParallelDescriptor().getGroupId(),
		       i->second.getParallelDescriptor().getSize(),
		       IS_YES(i->second.getParallelDescriptor().isGlobalMaster()));
	MPA_sayMessage("AsyncGlobalFileStatus",
		       false,
		       "************************************************");
    }

    return (mMpClassifier.mAnnoteMountPoints.size()!=0)? true : false;
}


FGFSInfoAnswer
AsyncGlobalFileStatus::isFullyDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mMpClassifier.mAnnoteMountPoints.find(result.dir_branch);

    if (i != mMpClassifier.mAnnoteMountPoints.end()) {
        answer = i->second.getFullyDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStatus::isWellDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mMpClassifier.mAnnoteMountPoints.find(result.dir_branch);

    if (i != mMpClassifier.mAnnoteMountPoints.end()) {
        answer = i->second.getWellDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStatus::isPoorlyDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mMpClassifier.mAnnoteMountPoints.find(result.dir_branch);

    if (i != mMpClassifier.mAnnoteMountPoints.end()) {
        answer = i->second.getPoorlyDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStatus::isUnique()
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mMpClassifier.mAnnoteMountPoints.find(result.dir_branch);

    if (i != mMpClassifier.mAnnoteMountPoints.end()) {
        answer = i->second.getUnique();
    }

    return answer;
}


