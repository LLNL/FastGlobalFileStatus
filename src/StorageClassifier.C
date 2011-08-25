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
 *                         independent module.
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
#include "FastGlobalFileStat.h"

using namespace FastGlobalFileStat;
using namespace FastGlobalFileStat::MountPointAttribute;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//

///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//

///////////////////////////////////////////////////////////////////
//
//  class StorageClassifier
//
//

StorageClassifier::StorageClassifier(CommFabric *c,
                       void *net/*=NULL*/,
                       void *dedicatedChannel/*=NULL*/)
    : hasErr(false)
{
    if (!initCommFabric(c, net, dedicatedChannel)) {
        hasErr = true;
    }
}


StorageClassifier::~StorageClassifier()
{
    if (commFabric) {
        delete commFabric;
        commFabric = NULL;
    }
}

bool
StorageClassifier::provideBestStorage(
                       const StorageCriteria &criteria,
                       MyMntEnt &mme)
{
    std::vector<std::string> mntPoints;
    GlobalFileStat *gfsObj = NULL;
    int i = 0;
    bool found = false;
    double score = STORAGE_CLASSIFIER_MIN_SCORE;
    double myScore;

    if ((criteria.getSpaceRequirement() == StorageCriteria::NO_REQUIREMENT)
       || !((criteria.getDistributionRequirement() == StorageCriteria::NO_REQUIREMENT)
           &&(criteria.getSpeedRequirement() == StorageCriteria::NO_REQUIREMENT)
           &&(criteria.getFsScalabilityRequirement()== StorageCriteria::NO_REQUIREMENT))) {

        return found;
    }


    if (!getGloballyAvailMntPoints(mntPoints)) {
        return found;
    }

    for (i=0; i < (int) mntPoints.size(); i++) {
        //
        // Test for each mount point
        //
        gfsObj = new GlobalFileStat(mntPoints[i].c_str(),
                                    commFabric,
                                    commFabric->getNet(),
                                    commFabric->getChannel());

        myScore = gfsObj->provideStorageClassification(criteria);

        if (myScore > score) {
            score = myScore;
            //
            // the MyMntEnt object is "copied" into mme
            //
            mme = gfsObj->getMyEntry();
            found = true;
        }

        delete gfsObj;
        gfsObj = NULL;
    }

    return found;
}


bool
StorageClassifier::provideMatchingStorage(
                                 const StorageCriteria &criteria,
                                 std::vector<MyMntEnt> &match)
{
    std::vector<std::string> mntPoints;
    GlobalFileStat *gfsObj = NULL;
    int i = 0;
    double score;


    if ((criteria.getSpaceRequirement() == StorageCriteria::NO_REQUIREMENT)
       || ((criteria.getDistributionRequirement() == StorageCriteria::NO_REQUIREMENT)
           &&(criteria.getSpeedRequirement() == StorageCriteria::NO_REQUIREMENT)
           &&(criteria.getFsScalabilityRequirement()== StorageCriteria::NO_REQUIREMENT))) {

        return false;
    }

    if (!getGloballyAvailMntPoints(mntPoints)) {
        return false;
    }

    for (i=0; i < (int) mntPoints.size(); i++) {

        gfsObj = new GlobalFileStat(mntPoints[i].c_str(),
                                    commFabric,
                                    commFabric->getNet(),
                                    commFabric->getChannel() );

        score = gfsObj->provideStorageClassification(criteria);

        if (score > STORAGE_CLASSIFIER_MIN_MATCH_SCORE) {
            //
            // This will make a copy of myMntEntry and apend
            // it to the match vector.
            match.push_back(gfsObj->getMyEntry());
        }

        delete gfsObj;
        gfsObj = NULL;
    }

    return ((match.empty())? false : true); 
}


///////////////////////////////////////////////////////////////////
//
//  class StorageCriteria
//
//

StorageCriteria::StorageCriteria()
    : spaceRequirement(StorageCriteria::NO_REQUIREMENT),
      spaceToFree(StorageCriteria::NO_REQUIREMENT),
      speedRequirement(StorageCriteria::NO_REQUIREMENT),
      distributionRequirement(StorageCriteria::NO_REQUIREMENT),
      fsScalabilityRequirement(StorageCriteria::NO_REQUIREMENT)
{

}


StorageCriteria::StorageCriteria(const StorageCriteria &criteria)
{
    spaceRequirement = criteria.spaceRequirement;
    spaceToFree = criteria.spaceToFree;
    speedRequirement = criteria.speedRequirement;
    distributionRequirement = criteria.distributionRequirement;
    fsScalabilityRequirement = criteria.fsScalabilityRequirement;
}

StorageCriteria::~StorageCriteria()
{

}

void 
StorageCriteria::setSpaceRequirement(int64_t bytesNeeded, int64_t bytesToFree)
{
    spaceRequirement = bytesNeeded - bytesToFree;
}

void 
StorageCriteria::setDistributionRequirement(int distDegree)
{
    distributionRequirement = distDegree;
}

void 
StorageCriteria::setSpeedRequirement(int speedReq)
{
    speedRequirement = speedReq;
}

void 
StorageCriteria::setFsScalabilityRequirement(int scalReq)
{
     fsScalabilityRequirement = scalReq;
}

const int64_t 
StorageCriteria::getSpaceRequirement() const
{
    return spaceRequirement;
}

const int64_t 
StorageCriteria::getSpaceToFree() const
{
    return spaceToFree;
}

const int 
StorageCriteria::getDistributionRequirement() const
{
    return distributionRequirement;
}

const int 
StorageCriteria::getSpeedRequirement() const
{
    return speedRequirement;
}

const int 
StorageCriteria::getFsScalabilityRequirement() const
{
    return fsScalabilityRequirement;
}
