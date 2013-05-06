/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Aug 26 2011 DHA: File created.
 *
 */

#include "MountPointsClassifier.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//


std::map<std::string, GlobalProperties>
MountPointsClassifier::mAnnoteMountPoints;


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


const FGFSInfoAnswer 
GlobalProperties::isUnique() const
{
     return mUnique;
}


const FGFSInfoAnswer
GlobalProperties::isPoorlyDistributed() const
{
    return mPoorlyDist;
}


const FGFSInfoAnswer
GlobalProperties::isWellDistributed() const
{
     return mWellDist;
}


const FGFSInfoAnswer
GlobalProperties::isFullyDistributed() const
{
    return mFullyDist;
}


const FileSystemType
GlobalProperties::getFsType() const
{
    return mFsType;
}


const int
GlobalProperties::getFsSpeed() const
{
    return mFsSpeed;
}


const int
GlobalProperties::getFsScalability() const
{
    return mFsScalability;
}


const int
GlobalProperties::getDistributionDegree() const
{
    return mDistributionDegree;
}


const std::string &
GlobalProperties::getFsName() const
{
    return mFsName;
}


FgfsParDesc &
GlobalProperties::getParDesc()
{
    return mParDesc;
}


const FgfsParDesc &
GlobalProperties::getParallelDescriptor() const
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
GlobalProperties::setFsType(FileSystemType t)
{
    mFsType = t;
}


void
GlobalProperties::setFsSpeed(int sp)
{
    mFsSpeed = sp;
}


void
GlobalProperties::setFsScalability(int sc)
{
    mFsScalability = sc;
}


void
GlobalProperties::setDistributionDegree(int dist)
{
    mDistributionDegree= dist;
}


void
GlobalProperties::setFsName(const char *str)
{
    mFsName = str;
}


void
GlobalProperties::setParDesc(const FgfsParDesc pd)
{
    // operator= must be defined for FgfsParDesc 
    mParDesc = pd;
}



///////////////////////////////////////////////////////////////////
//
//  class MountPointsClassifier
//
//

MountPointsClassifier::MountPointsClassifier()
{

}


MountPointsClassifier::~MountPointsClassifier()
{

}


bool
MountPointsClassifier::runClassification(CommLayer::CommFabric *c)
{
   if (!SyncGlobalFileStatus::initialize(NULL, c)) {
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

    //
    // mPointList contains logical mount point paths
    // Once mapReduce is done, parDesc.groupingMap contains.
    // 'logical path': {first rank, count}. Note the last
    // boolean being passed here to indicate not to eliminate
    // URI. This isn't an URI form.
    //
    if ( (rc = getCommFabric()->mapReduce(true, parDesc, mPointList, false)) ) {
        //GlobalProperties gp;
        std::map<std::string, ReduceDesc> &gmap = parDesc.getGroupingMap();
        std::map<std::string, ReduceDesc>::const_iterator mapIter;

        for (mapIter = gmap.begin(); mapIter != gmap.end(); ++mapIter) {
            if ((int)(mapIter->second.getCount()) == size) {
                //
                // Global properties are computed only for those logical
                // mount points that are "globally available"
                //
                SyncGlobalFileStatus gfstat(mapIter->first.c_str());
                gfstat.triage();
                GlobalProperties gprop;
                gprop.setFullyDist(gfstat.isFullyDistributed());
                gprop.setWellDist(gfstat.isWellDistributed());
                gprop.setPoorlyDist(gfstat.isPoorlyDistributed());
                gprop.setUnique(gfstat.isUnique());
                gprop.setFsType(gfstat.getMpInfo().determineFSType(gfstat.getMyEntry().type));
                gprop.setFsName(gfstat.getMpInfo().getFSName(gprop.getFsType()));
                if (IS_NO(gfstat.getParallelInfo().isGroupingDone())) {
                    //
                    // We force parallel info grouping for Mount Point classifier
                    // Yes, this can be a scalabilty challenge for billion-way
                    //
                    if (!gfstat.forceComputeParallelInfo()) {
                        if (ChkVerbose(1)) {
                            MPA_sayMessage("SyncGlobalFileStatus",
                            true,
                            "Error returned from computeParallelInfo");
                        }
                    }
                }

                gprop.setDistributionDegree(gfstat.getParallelInfo().getSize()
                                            /gfstat.getParallelInfo().getNumOfGroups());
                int speed = gfstat.getMpInfo().getSpeed(gprop.getFsType());
                int redSpeed;
                int scal = gfstat.getMpInfo().getScalability(gprop.getFsType());
                int redScal;

                rc = getCommFabric()->allReduce(true,
                                                parDesc,
                                                &speed,
                                                &redSpeed,
                                                1,
                                                REDUCE_INT,
                                                REDUCE_MIN);

                if (ChkVerbose(1) && !rc) {
                    MPA_sayMessage("MountPointsClassifier",
                        true,
                        "allReduce for speed returned false.");
                }

                rc = getCommFabric()->allReduce(true,
                                                parDesc,
                                                &scal,
                                                &redScal,
                                                1,
                                                REDUCE_INT,
                                                REDUCE_MIN);

                if (ChkVerbose(1) && !rc) {
                    MPA_sayMessage("MountPointsClassifier",
                        true,
                        "allReduce for scalabilty returned false.");
                }

                gprop.setFsSpeed(redSpeed);
                gprop.setFsScalability(redScal);
                gprop.setParDesc(gfstat.getParallelInfo());
                mAnnoteMountPoints[mapIter->first] = gprop;
            }
            else {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("MountPointsClassifier",
                        true,
                        "%s not globally available.",
                        mapIter->first.c_str());
                }
            }
        }
    }

    return rc;
}


const std::map<std::string, GlobalProperties> &
MountPointsClassifier::getGlobalMountpointsMap() 
{
    return mAnnoteMountPoints;
}

