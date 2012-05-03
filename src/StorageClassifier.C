/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 22 2011 DHA: File created. Moved StorageClassifier from
 *                         FastGlobalFileStat.h to put that service into an
 *                         independent class.
 *
 */

extern "C" {
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#include "bloom.h"
}

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "StorageClassifier.h"


using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::StorageInfo;
using namespace FastGlobalFileStatus::CommLayer;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//
MountPointsClassifier GlobalFileSystemsStatus::mMpClassifier;

///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//


///////////////////////////////////////////////////////////////////
//
//  class FileSystemsCriteria
//
//

FileSystemsCriteria::FileSystemsCriteria()
    : spaceRequirement(0),
      speedRequirement(SPEED_REQUIRE_NONE),
      distributionRequirement(DISTRIBUTED_REQUIRE_NONE),
      scalabilityRequirement(FS_SCAL_REQUIRE_NONE)
{

}

FileSystemsCriteria::FileSystemsCriteria(nbytes_t bytesNeeded,
                                 nbytes_t bytesToFree,
                                 SpeedRequirement speed,
                                 DistributionRequirement dist,
                                 ScalabilityRequirement scal)
    : speedRequirement(speed),
      distributionRequirement(dist),
      scalabilityRequirement(scal)
{
    spaceRequirement = bytesNeeded - bytesToFree;
}


FileSystemsCriteria::FileSystemsCriteria(const FileSystemsCriteria &criteria)
{
    spaceRequirement = criteria.spaceRequirement;
    spaceToFree = criteria.spaceToFree;
    speedRequirement = criteria.speedRequirement;
    distributionRequirement = criteria.distributionRequirement;
    scalabilityRequirement = criteria.scalabilityRequirement;
}


FileSystemsCriteria::~FileSystemsCriteria()
{

}


void
FileSystemsCriteria::setSpaceRequirement(nbytes_t bytesNeeded,
                                     nbytes_t bytesToFree)
{
    spaceRequirement = bytesNeeded - bytesToFree;
}


void
FileSystemsCriteria::setDistributionRequirement(
                     FileSystemsCriteria::DistributionRequirement dist)
{
    distributionRequirement = dist;
}


void
FileSystemsCriteria::setSpeedRequirement(
                     FileSystemsCriteria::SpeedRequirement speed)
{
    speedRequirement = speed;
}


void
FileSystemsCriteria::setScalabilityRequirement(
                     FileSystemsCriteria::ScalabilityRequirement scale)
{
     scalabilityRequirement = scale;
}


const FileSystemsCriteria::SpaceRequirement
FileSystemsCriteria::getSpaceRequirement() const
{
    return spaceRequirement;
}


const nbytes_t
FileSystemsCriteria::getSpaceToFree() const
{
    return spaceToFree;
}


const FileSystemsCriteria::DistributionRequirement
FileSystemsCriteria::getDistributionRequirement() const
{
    return distributionRequirement;
}


const FileSystemsCriteria::SpeedRequirement
FileSystemsCriteria::getSpeedRequirement() const
{
    return speedRequirement;
}


const FileSystemsCriteria::ScalabilityRequirement
FileSystemsCriteria::getScalabilityRequirement() const
{
    return scalabilityRequirement;
}


bool
FileSystemsCriteria::isRequireNone() const
{
    bool rc = false; 

    if ( (speedRequirement == SPEED_REQUIRE_NONE)
         && (distributionRequirement == DISTRIBUTED_REQUIRE_NONE)
         && (scalabilityRequirement == FS_SCAL_REQUIRE_NONE) ) {
        rc = true;
    }

    return rc;
}


///////////////////////////////////////////////////////////////////
//
//  class GlobalStorageChecker 
//
//

GlobalStorageChecker::GlobalStorageChecker(const char *pth)
    : SyncGlobalFileStatus(pth)
{

}


GlobalStorageChecker::GlobalStorageChecker(const char *pth, const int value)
    : SyncGlobalFileStatus(pth, value)
{

}


GlobalStorageChecker::~GlobalStorageChecker()
{
    // fileSigniture must not be deleted
}


bool
GlobalStorageChecker::initialize(CommFabric *c)
{
    return (SyncGlobalFileStatus::initialize(NULL, c));
}


FGFSInfoAnswer
GlobalStorageChecker::meetSpaceRequirement(nbytes_t BytesNeededlocally,
                               nbytes_t *BytesNeededGlobally,
                               nbytes_t *BytesNeededWithinGroup,
                               nbytes_t *BytesAvailableWithinGroup,
                               int *distEst)
{

    FGFSInfoAnswer rcAns = ans_error;
    struct statvfs statFsBuf;
    int rc = 0;
    int rcvRc = 0;
    ReduceDataType rdt = REDUCE_LONG_LONG_INT;

    if (IS_NO(getParallelInfo().isGroupingDone())) {
        //
        // TODO: You want this to be generalized comm.-split later.
        //
        if (!computeParallelInfo((GlobalFileStatusAPI *) this)) {
            rcAns = ans_error;
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStatus",
                    true,
                    "Error returned from computeParallelInfo for isUnique");
            }
            goto has_error;
        }

        //
        // Actual grouping including the node-local case.
        //
        (*distEst) = getParallelInfo().getSize() / (getParallelInfo().getNumOfGroups());
    }


   if (IS_YES(getParallelInfo().isRep())) {
       //
       // Only group repr performs statfs
       // Note that we do this on dir_branch, which in most cases
       // will be identical as dir_master normally except for
       // some "stacked" file systems like union fs.
       //
       rc = statvfs(getMyEntry().dir_branch.c_str(), &statFsBuf);
   }

   if (!(getCommFabric()->allReduce(true,
                                    getParallelInfo(),
                                    &rc,
                                    &rcvRc,
                                    1,
                                    REDUCE_INT,
                                    REDUCE_SUM))) {

        if (ChkVerbose(1)) {
            MPA_sayMessage("StorageGlobalFileStatus",
                true,
                "Error returned from allReduce");
        }

        goto has_error;
    }

    if (rcvRc) {
        // If non-zero return values from stat have been seen
        // error
        if (ChkVerbose(1)) {
            MPA_sayMessage("StorageGlobalFileStatus",
                true,
                "Error in rcvRc; one or more representatives calling statfs failed.");
        }

        goto has_error;
    }

    if (sizeof(BytesNeededlocally) == 4) {
        rdt = REDUCE_INT;
    }

    if (!(getCommFabric()->allReduce(true,
                                     getParallelInfo(),
                                     &BytesNeededlocally,
                                     BytesNeededGlobally,
                                     1,
                                     rdt,
                                     REDUCE_SUM))) {

         if (ChkVerbose(1)) {
             MPA_sayMessage("StorageGlobalFileStatus",
                 true,
                 "Error returned from allReduce");
         }

         goto has_error;
     }

    if (!(getCommFabric()->allReduce(false,
                                     getParallelInfo(),
                                     &BytesNeededlocally,
                                     BytesNeededWithinGroup,
                                     1,
                                     rdt,
                                     REDUCE_SUM))) {

         if (ChkVerbose(1)) {
             MPA_sayMessage("StorageGlobalFileStatus",
                 true,
                 "Error returned from allReduce");
         }

         goto has_error;
     }

    if (!(getCommFabric()->broadcast(false,
                                     getParallelInfo(),
                                     (unsigned char*) &statFsBuf,
                                     sizeof(statFsBuf)))) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("StorageGlobalFileStatus",
                true,
                "Error in group broadcast");
        }

        goto has_error;
    }

    (*BytesAvailableWithinGroup) = 0;
    if (!(statFsBuf.f_flag & ST_RDONLY)) {
        (*BytesAvailableWithinGroup) = statFsBuf.f_bavail * statFsBuf.f_bsize;
    }

    rc = (*BytesAvailableWithinGroup >= *BytesNeededWithinGroup)? 0 : 1;

    if (!(getCommFabric()->allReduce(true,
                                     getParallelInfo(),
                                     &rc,
                                     &rcvRc,
                                     1,
                                     REDUCE_INT,
                                     REDUCE_MAX))) {

        if (ChkVerbose(1)) {
            MPA_sayMessage("StorageGlobalFileStatus",
                true,
                "Error returned from allReduce");
        }

        goto has_error;
    }

    rcAns = (rcvRc)? ans_no : ans_yes;

has_error:
    return rcAns;
}


///////////////////////////////////////////////////////////////////
//
//  class GlobalFileSystemsStatus
//
//


GlobalFileSystemsStatus::GlobalFileSystemsStatus( )
    : mHasErr(false)
{

}


GlobalFileSystemsStatus::~GlobalFileSystemsStatus()
{

}


bool
GlobalFileSystemsStatus::initialize(CommLayer::CommFabric *c)
{
    bool rc;
    if (rc = GlobalStorageChecker::initialize(c)) {
        rc = mMpClassifier.runClassification(c);
    }
    return rc;
}


bool sortPredicate(const MyMntEntWScore &d1, const MyMntEntWScore &d2)
{
    return d1.score > d2.score;
}


bool
GlobalFileSystemsStatus::provideBestFileSystems(const FileSystemsCriteria &criteria,
                       std::vector<MyMntEntWScore> &match)
{
    int i = 0;
    bool found = false;
    int score = STORAGE_CLASSIFIER_REQ_UNMET_SCORE;
    int myScore;
    std::string matchedPath = "";

    std::map<std::string, GlobalProperties>::const_iterator iter;
    for (iter = mMpClassifier.mAnnoteMountPoints.begin();
             iter != mMpClassifier.mAnnoteMountPoints.end(); ++iter) {

        myScore = scoreStorage(iter->first, iter->second, criteria);
        if (myScore > STORAGE_CLASSIFIER_REQ_UNMET_SCORE) {
            MyMntEntWScore tmpEnt;
            tmpEnt.score = myScore;
            matchedPath = iter->first;
            found = true;
            getMpInfo().getMntPntInfo2(matchedPath.c_str(), tmpEnt.mpEntry);
            match.push_back(tmpEnt);
        }
    }

    sort(match.begin(), match.end(), sortPredicate);

    return found;
}


const MountPointsClassifier &
GlobalFileSystemsStatus::getMountPointsClassifier() 
{
    return mMpClassifier;    
}


///////////////////////////////////////////////////////////////////
//
//  Private Interface
//
//


int
GlobalFileSystemsStatus::scoreStorage(const std::string &pth,
                                const GlobalProperties &gps,
                                const FileSystemsCriteria &criteria)
{
    //
    // Score for the spaceReq: if space not there, all bets are off
    //     if storage meets the space requirement goto next
    //
    int distEst;
    int score = checkSpaceReq(pth, gps, criteria.getSpaceRequirement(), &distEst);

    if (score == STORAGE_CLASSIFIER_REQ_MET_SCORE) {

        if (!criteria.isRequireNone()) {
            //
            // Some criteria have been give. This means
            // Score becomes MEET (zero) vs. UNMEET(negative)
            //
            score += scoreWSpeedReq(pth, gps, criteria.getSpeedRequirement());
            score += scoreWDistributionReq(pth, gps, criteria.getDistributionRequirement());
            score += scoreWScalabilityReq(pth, gps, criteria.getScalabilityRequirement());
        }
        else {
            //
            // a simple score function
            //
            float ratio = 1.0;
            float floatScore = 0.0;
            ratio = (float) gps.getFsScalability()
                                /( (float) ((gps.getFsScalability() > distEst)?
                                            (float) gps.getFsScalability()
                                            : (float) distEst));


            floatScore = ratio * (((float)gps.getFsSpeed())/((float)STORAGE_CLASSIFIER_MAX_DEVICE_SPEED));
            floatScore *= (float) STORAGE_CLASSIFIER_MAX_SCORE;
            score = (int) floatScore;
        }
    }

    return score;
}


int
GlobalFileSystemsStatus::checkSpaceReq(const std::string &pth,
                       const GlobalProperties &gps,
                       const nbytes_t space,
                       int *distEst)
{
    int score = STORAGE_CLASSIFIER_REQ_UNMET_SCORE;
    nbytes_t bytesNeededGlobally;
    nbytes_t bytesNeededWithinGroup;
    nbytes_t bytesAvailableWithinGroup;

    GlobalStorageChecker sgf(pth.c_str());

    sgf.triage();

    FGFSInfoAnswer ans = sgf.meetSpaceRequirement(space,
                             &bytesNeededGlobally,
                             &bytesNeededWithinGroup,
                             &bytesAvailableWithinGroup,
                             distEst);

    if (IS_YES(ans)) {
        score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
    }

    return score;
}


int
GlobalFileSystemsStatus::scoreWSpeedReq(const std::string &pth,
                       const GlobalProperties &gps,
                       const FileSystemsCriteria::SpeedRequirement speed)
{
    int score = STORAGE_CLASSIFIER_REQ_UNMET_SCORE;

    switch (speed) {
    case FileSystemsCriteria::SPEED_REQUIRE_NONE:
        score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        break;

    case FileSystemsCriteria::SPEED_LOW:
        if (gps.getFsSpeed() == BASE_FS_SPEED) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    case FileSystemsCriteria::SPEED_HIGH:
        if (gps.getFsSpeed() > BASE_FS_SPEED) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    default:
        break;
    }

    return score;
}


int
GlobalFileSystemsStatus::scoreWDistributionReq(const std::string &pth,
                       const GlobalProperties &gps,
                       const FileSystemsCriteria::DistributionRequirement dist)
{
    int score = STORAGE_CLASSIFIER_REQ_UNMET_SCORE;

    switch (dist) {
    case FileSystemsCriteria::DISTRIBUTED_REQUIRE_NONE:
        score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        break;

    case FileSystemsCriteria::DISTRIBUTED_UNIQUE:
        if (gps.getUnique()) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    case FileSystemsCriteria::DISTRIBUTED_LOW:
        if (gps.getPoorlyDist()) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    case FileSystemsCriteria::DISTRIBUTED_HIGH:
        if (gps.getWellDist()) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    default:
        break;
    }

    return score;
}


int
GlobalFileSystemsStatus::scoreWScalabilityReq(const std::string &pth,
                       const GlobalProperties &gps,
                       const FileSystemsCriteria::ScalabilityRequirement scale)
{
    int score = STORAGE_CLASSIFIER_REQ_UNMET_SCORE;

    switch (scale) {
    case FileSystemsCriteria::FS_SCAL_REQUIRE_NONE:
        score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        break;

    case FileSystemsCriteria::FS_SCAL_SINGLE:
        if (gps.getFsScalability() == BASE_FS_SCALABILITY) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    case FileSystemsCriteria::FS_SCAL_MULTI:
        if (gps.getFsScalability() > BASE_FS_SCALABILITY) {
            score = STORAGE_CLASSIFIER_REQ_MET_SCORE;
        }
        break;

    default:
        break;
    }

    return score;
}

