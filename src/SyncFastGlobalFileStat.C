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
#include "SyncFastGlobalFileStat.h"

using namespace FastGlobalFileStat;
using namespace FastGlobalFileStat::MountPointAttribute;
using namespace FastGlobalFileStat::CommLayer;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//
FileSignitureGen *SyncGlobalFileStat::fileSignitureGen = NULL;


///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//

///////////////////////////////////////////////////////////////////
//
//  class SyncGlobalFileStat
//
//

SyncGlobalFileStat::SyncGlobalFileStat(const char *pth)
    : GlobalFileStatBase(pth)
{

}


SyncGlobalFileStat::SyncGlobalFileStat(const char *pth, const int value)
    : GlobalFileStatBase(pth, value)
{

}


SyncGlobalFileStat::~SyncGlobalFileStat()
{
    // fileSigniture must not be deleted
}


bool
SyncGlobalFileStat::initialize(FileSignitureGen *fsg,
                               CommFabric *c)
{
    fileSignitureGen = fsg;
    return (GlobalFileStatBase::initialize(c));
}


bool
SyncGlobalFileStat::triage(CommAlgorithms algo)
{
    int rank, size;
    bool isMaster;

    if (!getCommFabric()->getRankSize(&rank, &size, &isMaster)) {
        return false;
    }

    getParallelInfo().setRank(rank);
    getParallelInfo().setSize(size);

    if (isMaster) {
        getParallelInfo().setGlobalMaster();
    }
    else {
        getParallelInfo().unsetGlobalMaster();
    }

    return computeCardinalityEst(algo);
}


FGFSInfoAnswer
SyncGlobalFileStat::isFullyDistributed() const
{
    return (isNodeLocal())? ans_yes : ans_no;
}


FGFSInfoAnswer
SyncGlobalFileStat::isWellDistributed() const
{
    FGFSInfoAnswer answer = isPoorlyDistributed();
    if (IS_NO(answer)) {
        answer = ans_yes;
    }
    else if (IS_YES(answer)) {
        answer = ans_no;
    }

    return answer;
}


FGFSInfoAnswer
SyncGlobalFileStat::isPoorlyDistributed() const
{
    FGFSInfoAnswer answer = ans_no;

    if (getCardinalityEst() <= getHiLoCutoff()) {
        answer = ans_yes;
    }
    else if (getCardinalityEst() == FGFS_NOT_FILLED) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "Cardinality Est. isn't known; has triage not been called?");
        }
        answer = ans_error;
    }

    return answer;
}


FGFSInfoAnswer
SyncGlobalFileStat::isUnique()
{
    FGFSInfoAnswer answer = ans_no;
    if (IS_YES(isPoorlyDistributed())
        || (getCardinalityEst() < FGFS_NPROC_TO_SATURATE) ) {
        if (IS_NO(getParallelInfo().isGroupingDone())) {
            //
            // parallel info grouping has not been executed
            //
            if (!computeParallelInfo()) {
                answer = ans_error;
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStat",
                        true,
                        "Error returned from computeParallelInfo for isUnique");
                }
                goto return_location;
            }
        }

        if (IS_YES(getParallelInfo().isSingleGroup())) {
            answer = ans_yes;
        }
    }

return_location:
    return answer;
}


FGFSInfoAnswer 
SyncGlobalFileStat::isConsistent(bool serial/*=false*/)
{
    FGFSInfoAnswer answer = ans_no;
    struct stat sb;
    unsigned char *mySig;
    int sigSize;

    if (!serial && IS_YES(isUnique())) {
        answer = ans_yes;
    }
    else {
        if (!serial) {
            if (!(mySig = signiture(&sb, &sigSize))) {
                answer = ans_error;
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "signiture couldn't be computed");
                }
                goto return_location;
            }
        }
        else {
            if (!(mySig = signitureSerial(&sb, &sigSize))) {
                answer = ans_error;
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "signitureSerial couldn't be computed");
                }
                goto return_location;
            }
        }

        unsigned char *sigCopy
            = (unsigned char *) malloc(sigSize);
        if (!sigCopy) {
            answer = ans_error;
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "out of memory!");
            }
            goto return_location;
        }

        memcpy(sigCopy, mySig, sigSize);
        if (!getCommFabric()->broadcast(true,
                                        getParallelInfo(),
                                        sigCopy,
                                        (FgfsCount_t) sigSize)) {
            answer = ans_error;
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "comm fabric returns an error");
            }
            goto return_location;
        }

        unsigned char *t = sigCopy;
        unsigned char *torig = mySig;
        int equal = 0;

        while (t < (sigCopy+sigSize)) {
            if ((*t) ^ (*torig)) {
                equal = 1;
                break;
            }
            t += sizeof(*t);
            torig += sizeof(*torig);
        }
        free(mySig);
        free(sigCopy);

        int allEqual = 0;
        if (!getCommFabric()->allReduce(true,
                                        getParallelInfo(),
                                        (void *) &equal,
                                        (void *) &allEqual,
                                        1,
                                        REDUCE_INT, REDUCE_MAX)) {
            answer = ans_error;
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error returned from globalAllReduceIntMAX");
            }
            goto return_location;
        }

        if (allEqual == 0) {
            answer = ans_yes;
        }
    }

return_location:
    return answer;
}


unsigned char *
SyncGlobalFileStat::signiture(struct stat *sb, int *sigSize)
{
    unsigned char *retbuf = NULL;
    int fd = -1;

    *sigSize = 0;
    memset(sb, '\0', sizeof(*sb));

    if (!fileSignitureGen) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "fileSignitureGen has not been registered");
        }
        goto has_error;
    }

    if (IS_YES(isPoorlyDistributed())) {
        int rc = 0;
        int rcvRc;

        if (IS_NO(getParallelInfo().isGroupingDone())) {
            //
            // parallel info grouping has not been executed
            //
            if (!computeParallelInfo()) {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStat",
                        true,
                        "Error returned from computeParallelInfo");
                }
                goto has_error;
            }
        }

        if (IS_YES(getParallelInfo().isRep())) {
            //
            // Only group repr performs stat
            //
            rc = stat(getPath(), sb);
            if (rc == 0) {
                if (S_ISREG(sb->st_mode) && (S_IRUSR & sb->st_mode)) {
                    int fd = open(getPath(), O_RDONLY);
                    if (fd >= 0) {
                        retbuf = fileSignitureGen->signiture(fd,
                                     (int)sb->st_size,
                                     sigSize);
                        if (!retbuf || !sigSize) {
                            rc++;
                        }
                        close(fd);
                    }
                    else {
                        rc++;
                    }
                }
                else {
                    rc++;
                }
            }
        }

        if (!(getCommFabric()->allReduce(true,
                                         getParallelInfo(),
                                         &rc,
                                         &rcvRc,
                                         1,
                                         REDUCE_INT,
                                         REDUCE_SUM))) {

            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error returned from globalAllReduceIntSUM");
            }
            goto has_error;
        }

        if (rcvRc) {
            // If non-zero return values from stat have been seen
            // error
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error in rcvRc; one or more representatives calling stat failed.");
            }

            goto has_error;
        }

        if (!(getCommFabric()->broadcast(false,
                                         getParallelInfo(),
                                         (unsigned char*) sb,
                                         sizeof(*sb)))) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error in groupBroadcastBytes");
            }

            goto has_error;
        }

        if (!(getCommFabric()->broadcast(false,
                                         getParallelInfo(),
                                         (unsigned char*)sigSize,
                                         sizeof(*sigSize)))) {

            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error in groupBroadcastBytes 2");
            }

            goto has_error;
        }

        if (!retbuf) {
            retbuf = (unsigned char *) malloc((size_t)*sigSize);
        }

        if (!(getCommFabric()->broadcast(false,
                                         getParallelInfo(),
                                         retbuf,
                                         *sigSize))) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "Error in groupBroadcastBytes 3");
            }

            goto has_error;
        }
    }
    else {
        //
        // if well-distributed, all can perform stat
        //
        if (stat(getPath(), sb) < 0) {
            goto has_error;
        }

        if (!S_ISREG(sb->st_mode) || !(S_IRUSR & sb->st_mode)) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "not a regular file");
            }
            goto has_error;
        }

        if ( (fd = open(getPath(), O_RDONLY)) < 0) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "open failed");
            }
            goto has_error;
        }

        retbuf = fileSignitureGen->signiture(fd,
                     (int)sb->st_size,
                     sigSize);

        if  (!retbuf || !sigSize) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStat",
                    true,
                    "signiture generation failed");
            }
            goto has_error;
        }

        close(fd);
    }

    return retbuf;

has_error:
    return NULL;
}



unsigned char *
SyncGlobalFileStat::signitureSerial(struct stat *sb, int *sigSize)
{
    unsigned char *retbuf = NULL;
    int fd = -1;

    *sigSize = 0;
    memset(sb, '\0', sizeof(*sb));

    if (!fileSignitureGen) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "fileSignitureGen isn't registered");
        }

        goto has_error;
    }

    if (stat(getPath(), sb) < 0) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "stat failed");
        }

        goto has_error;
    }

    if (!S_ISREG(sb->st_mode) || !(S_IRUSR & sb->st_mode)) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "not a regular file");
        }

        goto has_error;
    }

    if ( (fd = open(getPath(), O_RDONLY)) < 0) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "open failed");
        }

        goto has_error;
    }

    retbuf = fileSignitureGen->signiture(fd, (int)sb->st_size, sigSize);
    if  (!retbuf || !sigSize) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStat",
                true,
                "signiture generation failed");
        }

        goto has_error;
    }

    close(fd);

    return retbuf;

has_error:
    return NULL;
}


#if 0
const double
SyncGlobalFileStat::provideStorageClassification (
                    const StorageCriteria &criteria) 
{
    //
    // I'll think about this later
    //
    struct statfs sb;
    double score = STORAGE_CLASSIFIER_MIN_SCORE + 1.0;
    int rc = 0;
    int rcvRc = 0;
    int insufficient = 0;
    int rcvInsu = 0;
    int64_t spaceNeeded = criteria.getSpaceRequirement()
                          - criteria.getSpaceToFree();
    int64_t aggregateSize = 0;

    if (!computeParallelInfo()) {
        goto has_error;
    }

    if (!(getCommFabric()->groupAllReduceInt64SUM(parallelInfo,
                          &spaceNeeded, &aggregateSize))) {
        std::cerr << "Error in groupAllReduceInt64SUM" << std::endl;
        goto has_error;
    }

    if (IS_YES(parallelInfo.isRep())) {
        rc = statfs(getPath(), &sb);
        if (!(rc < 0)) {
            if ( (int64_t)(sb.f_bavail * sb.f_bsize) < aggregateSize ) {
                insufficient = 1;
            }
        }
    }

    if (!(getCommFabric()->globalAllReduceIntSUM(&rc, &rcvRc))) {
        std::cerr << "Error in globalAllReduceIntSUM" << std::endl;
        goto has_error;
    }

    if (!(commFabric->globalAllReduceIntMAX(&insufficient, &rcvInsu))) {
        std::cerr << "Error in globalAllReduceIntMAX" << std::endl;
        goto has_error;
    }

    if (rcvInsu != 0) {
        if ( (criteria.getDistributionRequirement() ==  StorageCriteria::NO_REQUIREMENT)
             && (criteria.getSpeedRequirement() ==  StorageCriteria::NO_REQUIREMENT)
             && (criteria.getFsScalabilityRequirement() ==  StorageCriteria::NO_REQUIREMENT)) {
            //
            // No requirement is given except for spaceRequirement
            // In this case, we use a built-in scoring system 
            //
            double deviceSpeed = STORAGE_CLASSIFIER_BASE_DEVICE_SPEED;
            if (IS_YES(isFullyDistributed())) {
                // TODO: Assume for a moment that a local device is 4x faster
                // than a remote
                deviceSpeed *= 4.0;
            }

            double fsScale = STORAGE_CLASSIFIER_BASE_FS_SCALABILITY;
            if (mpInfo.determineFSType(myEntry.type) == fs_lustre) {
                // TODO: Assume for a moment that lustre has 6x better scalability
                // than a non-parallel device
                fsScale *= 6.0;
            }

            double sharedDegree = ((double)parallelInfo.size)/((double)cardinalityEst);
            score = STORAGE_CLASSIFIER_MIN_MATCH_SCORE
                    + (deviceSpeed / (sharedDegree/fsScale));
        }
        else {
            //
            // In this case, we test one set requirement at a time
            //
            score = STORAGE_CLASSIFIER_MIN_MATCH_SCORE;

            if (criteria.getDistributionRequirement() !=  StorageCriteria::NO_REQUIREMENT) {
                switch(criteria.getDistributionRequirement()) {
                case StorageCriteria::DISTRIBUTED_NONE:
                    if (!IS_YES(isUnique())) {
                        score -= 1.0;
                    }
                    break;
                case StorageCriteria::DISTRIBUTED_LOW:
                    if (!IS_YES(isPoorlyDistributed())) {
                        score -= 1.0;
                    }
                    break;
                case StorageCriteria::DISTRIBUTED_HIGH:
                    if (!IS_YES(isWellDistributed())) {
                        score -= 1.0;
                    }
                    break;
                case StorageCriteria::DISTRIBUTED_FULL:
                    if (!IS_YES(isFullyDistributed())) {
                        score -= 1.0;
                    }
                    break;
                default:
                    score -= 1.0;
                    break;
                }
            }

            if ((score > (STORAGE_CLASSIFIER_MIN_MATCH_SCORE-0.5))
                && (criteria.getSpeedRequirement() != StorageCriteria::NO_REQUIREMENT)) {

                switch(criteria.getSpeedRequirement()) {
                case StorageCriteria::SPEED_LOW:
                    if (IS_YES(isFullyDistributed())) {
                        score -= 1.0;
                    }
                    break;
                case StorageCriteria::SPEED_HIGH:
                    if (!IS_YES(isFullyDistributed())) {
                        score -= 1.0;
                    }
                    break;
                default:
                    score -= 1.0;
                    break;
                }
            }

            if ((score > (STORAGE_CLASSIFIER_MIN_MATCH_SCORE-0.5))
                && (criteria.getFsScalabilityRequirement() != StorageCriteria::NO_REQUIREMENT)) {

                switch(criteria.getFsScalabilityRequirement()) {
                case StorageCriteria::FS_SCAL_SINGLE:
                    if (mpInfo.determineFSType(myEntry.type) == fs_lustre) {
                        //
                        // Assume that when fs_lustre, everyone has the same type
                        //
                        score -= 1.0;
                    }
                    break;
                case StorageCriteria::FS_SCAL_MULTI:
                    if (mpInfo.determineFSType(myEntry.type) != fs_lustre) {
                        //
                        // Assume that when fs_lustre, everyone has the same type
                        //
                        score -= 1.0;
                    }
                    break;
                default:
                    score -= 1.0;
                    break;
                }
            }
        }
    }

has_error:
    return score;
}
#endif

