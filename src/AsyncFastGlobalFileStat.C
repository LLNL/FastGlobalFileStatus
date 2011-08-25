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

using namespace FastGlobalFileStat;
using namespace FastGlobalFileStat::MountPointAttribute;
using namespace FastGlobalFileStat::CommLayer;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//
std::map<std::string, GlobalProperties>
AsyncGlobalFileStat::mAnnoteMountPoints;

///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//


///////////////////////////////////////////////////////////////////
//
//  class GlobalProperties
//
//

GlobalProperties::GlobalProperties()
    : mUnique(ans_error),
      mPoorlyDist(ans_error),
      mWellDist(ans_error),
      mFullyDist(ans_error),
      mConsistent(ans_error)
{

}


GlobalProperties::GlobalProperties(const GlobalProperties &o)
{
    mUnique = o.mUnique;
    mPoorlyDist = o.mPoorlyDist;
    mWellDist = o.mWellDist;
    mFullyDist = o.mFullyDist;
    mConsistent = o.mConsistent;
    mParDesc = o.mParDesc;
}


GlobalProperties::~GlobalProperties()
{

}


const FGFSInfoAnswer
GlobalProperties::getUnique() const
{
    return mUnique;
}


const FGFSInfoAnswer
GlobalProperties::getPoorlyDist() const
{
    return mPoorlyDist;
}


const FGFSInfoAnswer
GlobalProperties::getWellDist() const
{
    return mWellDist;
}


const FGFSInfoAnswer
GlobalProperties::getFullyDist() const
{
    return mFullyDist;
}


const FGFSInfoAnswer
GlobalProperties::getConsistent() const
{
    return mConsistent;
}


FgfsParDesc &
GlobalProperties::getParDesc()
{
    return mParDesc;
}


void
GlobalProperties::setUnique(FGFSInfoAnswer v)
{
    mUnique = v;
}


void
GlobalProperties::setPoorlyDist(FGFSInfoAnswer v)
{
    mPoorlyDist = v;
}


void
GlobalProperties::setWellDist(FGFSInfoAnswer v)
{
    mWellDist = v;
}


void
GlobalProperties::setFullyDist(FGFSInfoAnswer v)
{
    mFullyDist = v;
}


void
GlobalProperties::setConsistent(FGFSInfoAnswer v)
{
    mConsistent = v;
}


void
GlobalProperties::setParDesc(const FgfsParDesc pd)
{
    // operator= must be defined for FgfsParDesc 
    mParDesc = pd;
}


///////////////////////////////////////////////////////////////////
//
//  class AsyncGlobalFileStat
//
//

AsyncGlobalFileStat::AsyncGlobalFileStat(const char *pth)
    : GlobalFileStatBase(pth)
{

}


AsyncGlobalFileStat::AsyncGlobalFileStat(const char *pth, const int value)
    : GlobalFileStatBase(pth, value)
{

}


AsyncGlobalFileStat::~AsyncGlobalFileStat()
{

}


bool
AsyncGlobalFileStat::initialize(CommLayer::CommFabric *c)
{
    if (!SyncGlobalFileStat::initialize(NULL, c)) {
        return false;
    }

    int rank, size;
    bool isMaster;

    if (!getCommFabric()->getRankSize(&rank, &size, &isMaster)) {
        return false;
    }

    std::map<std::string, MyMntEnt> mpMap = getMpInfo().getMntPntMap();
    std::map<std::string, MyMntEnt>::const_iterator const_i;

    std::vector<std::string> mPointList;
    for (const_i = mpMap.begin(); const_i != mpMap.end(); ++const_i) {
        mPointList.push_back(const_i->first);
    }

    FgfsParDesc parDesc;
    parDesc.setRank(rank);
    parDesc.setSize(size);

    if (isMaster) {
        parDesc.setGlobalMaster();
    }
    else {
        parDesc.unsetGlobalMaster();
    }

    bool rc;

    if ( (rc = getCommFabric()->mapReduce(true, parDesc, mPointList)) ) {
        GlobalProperties gp;
        std::map<std::string, ReduceDesc> &gmap = parDesc.getGroupingMap();
        std::map<std::string, ReduceDesc>::const_iterator mapIter;

        for (mapIter = gmap.begin(); mapIter != gmap.end(); ++mapIter) {
            if ((int)(mapIter->second.getCount()) == size) {
                //
                // Global properties are computed only for those logical
                // mount points that are globally available
                //
                SyncGlobalFileStat gfstat(mapIter->first.c_str());
                gfstat.triage();
                GlobalProperties gprop;
                gprop.setFullyDist(gfstat.isFullyDistributed());
                gprop.setWellDist(gfstat.isWellDistributed());
                gprop.setPoorlyDist(gfstat.isPoorlyDistributed());
                gprop.setUnique(gfstat.isUnique());
                gprop.setParDesc(gfstat.getParallelInfo());
                mAnnoteMountPoints[mapIter->first] = gprop;
            }
            else {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("AsyncGlobalFileStat",
                        true,
                        "%s not globally available.",
                        mapIter->first.c_str());
                }
            }
        }
    }

    return rc;
}


FGFSInfoAnswer
AsyncGlobalFileStat::isFullyDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mAnnoteMountPoints.find(result.dir_branch);

    if (i != mAnnoteMountPoints.end()) {
        answer = i->second.getFullyDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStat::isWellDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mAnnoteMountPoints.find(result.dir_branch);

    if (i != mAnnoteMountPoints.end()) {
        answer = i->second.getWellDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStat::isPoorlyDistributed() const
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mAnnoteMountPoints.find(result.dir_branch);

    if (i != mAnnoteMountPoints.end()) {
        answer = i->second.getPoorlyDist();
    }

    return answer;
}


FGFSInfoAnswer
AsyncGlobalFileStat::isUnique()
{
    FGFSInfoAnswer answer = ans_error;
    MyMntEnt result;

    //
    // This is a bit approximation...
    //
    getMpInfo().isRemoteFileSystem(getPath(), result);
    std::map<std::string, GlobalProperties>::const_iterator i;
    i = mAnnoteMountPoints.find(result.dir_branch);

    if (i != mAnnoteMountPoints.end()) {
        answer = i->second.getUnique();
    }

    return answer;
}


