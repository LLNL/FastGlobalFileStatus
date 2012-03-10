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

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//
FileSignitureGen *SyncGlobalFileStatus::fileSignitureGen = NULL;


///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//

///////////////////////////////////////////////////////////////////
//
//  class SyncGlobalFileStatus
//
//

SyncGlobalFileStatus::SyncGlobalFileStatus(const char *pth)
    : GlobalFileStatusAPI(pth), GlobalFileStatusBase()
{

}


SyncGlobalFileStatus::SyncGlobalFileStatus(const char *pth, const int value)
    : GlobalFileStatusAPI(pth), GlobalFileStatusBase(value)
{

}


SyncGlobalFileStatus::~SyncGlobalFileStatus()
{
    // fileSigniture must not be deleted
}


bool
SyncGlobalFileStatus::initialize(FileSignitureGen *fsg,
                               CommFabric *c)
{
    fileSignitureGen = fsg;
    return (GlobalFileStatusBase::initialize(c));
}


bool
SyncGlobalFileStatus::triage(CommAlgorithms algo)
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

    return computeCardinalityEst((GlobalFileStatusAPI *)this, algo);
}


FGFSInfoAnswer
SyncGlobalFileStatus::isFullyDistributed() const
{
    return (isNodeLocal())? ans_yes : ans_no;
}


FGFSInfoAnswer
SyncGlobalFileStatus::isWellDistributed() const
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
SyncGlobalFileStatus::isPoorlyDistributed() const
{
    FGFSInfoAnswer answer = ans_no;

    if (getCardinalityEst() <= getHiLoCutoff()) {
        answer = ans_yes;
    }
    else if (getCardinalityEst() == FGFS_NOT_FILLED) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
                true,
                "Cardinality Est. isn't known; has triage not been called?");
        }
        answer = ans_error;
    }

    return answer;
}


FGFSInfoAnswer
SyncGlobalFileStatus::isUnique()
{
    FGFSInfoAnswer answer = ans_no;
    if (IS_YES(isPoorlyDistributed())
        || (getCardinalityEst() < FGFS_NPROC_TO_SATURATE) ) {
        if (IS_NO(getParallelInfo().isGroupingDone())) {
            //
            // parallel info grouping has not been executed
            //
            if (!computeParallelInfo((GlobalFileStatusAPI *) this)) {
                answer = ans_error;
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStatus",
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
SyncGlobalFileStatus::isConsistent(bool serial/*=false*/)
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
                    MPA_sayMessage("SyncGlobalFileStatus",
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
                    MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
SyncGlobalFileStatus::signiture(struct stat *sb, int *sigSize)
{
    unsigned char *retbuf = NULL;
    int fd = -1;

    *sigSize = 0;
    memset(sb, '\0', sizeof(*sb));

    if (!fileSignitureGen) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
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
            if (!computeParallelInfo((GlobalFileStatusAPI *) this)) {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
                    true,
                    "Error returned from allReduce");
            }
            goto has_error;
        }

        if (rcvRc) {
            // If non-zero return values from stat have been seen
            // error
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
                    true,
                    "not a regular file");
            }
            goto has_error;
        }

        if ( (fd = open(getPath(), O_RDONLY)) < 0) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStatus",
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
                MPA_sayMessage("SyncGlobalFileStatus",
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
SyncGlobalFileStatus::signitureSerial(struct stat *sb, int *sigSize)
{
    unsigned char *retbuf = NULL;
    int fd = -1;

    *sigSize = 0;
    memset(sb, '\0', sizeof(*sb));

    if (!fileSignitureGen) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
                true,
                "fileSignitureGen isn't registered");
        }

        goto has_error;
    }

    if (stat(getPath(), sb) < 0) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
                true,
                "stat failed");
        }

        goto has_error;
    }

    if (!S_ISREG(sb->st_mode) || !(S_IRUSR & sb->st_mode)) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
                true,
                "not a regular file");
        }

        goto has_error;
    }

    if ( (fd = open(getPath(), O_RDONLY)) < 0) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
                true,
                "open failed");
        }

        goto has_error;
    }

    retbuf = fileSignitureGen->signiture(fd, (int)sb->st_size, sigSize);
    if  (!retbuf || !sigSize) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("SyncGlobalFileStatus",
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


bool
SyncGlobalFileStatus::forceComputeParallelInfo()
{
    bool rc = true;
    if (IS_NO(getParallelInfo().isGroupingDone())) {
        //
        // parallel info grouping has not been executed
        //
        if (!computeParallelInfo((GlobalFileStatusAPI *) this)) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("SyncGlobalFileStatus",
                    true,
                    "Error returned from computeParallelInfo");
            }
            rc = false;
        }
    }

    return rc; 
}

