/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 22 2011 DHA: File created from the old FastGlobalFileStat.C.
 *        Feb 15 2011 DHA: Added StorageClassifier support.
 *        Feb 15 2011 DHA: Changed main higher level abstrations for
 *                         GlobalFileStat to be isUnique, isPoorlyDistributed
 *                         isWellDistributed, and isFullyDistributed.
 *        Jan 13 2011 DHA: File created.
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
using namespace FastGlobalFileStat::CommLayer;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//
CommFabric *GlobalFileStatBase::mCommFabric = NULL;
MountPointInfo GlobalFileStatBase::mpInfo(true);


///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//

///////////////////////////////////////////////////////////////////
//
//  class GlobalFileStat
//
//

GlobalFileStatBase::GlobalFileStatBase(const char *pth)
    : mHasErr(false),
      mAlgorithm(algo_unknown),
      mThresholdToSaturate(FGFS_NPROC_TO_SATURATE),
      mHiLoCutoff(FGFS_NOT_FILLED),
      mCardinalityEst(FGFS_NOT_FILLED),
      mNodeLocal(false)
{
    mPath = pth;
    if (!mCommFabric) {
        mHasErr = true;
    }
}


GlobalFileStatBase::GlobalFileStatBase(const char *pth, const int value)
    : mHasErr(false),
      mAlgorithm(algo_unknown),
      mHiLoCutoff(FGFS_NOT_FILLED),
      mCardinalityEst(FGFS_NOT_FILLED),
      mNodeLocal(false)
{
    mPath = pth;
    mThresholdToSaturate = value;
    if (!mCommFabric) {
        mHasErr = true;
    }
}


GlobalFileStatBase::~GlobalFileStatBase()
{
    // mCommFabric must not be deleted!
}


const MountPointInfo &
GlobalFileStatBase::getMpInfo()
{
    return mpInfo;
}


const CommFabric *
GlobalFileStatBase::getCommFabric() 
{
    return mCommFabric;
}


bool
GlobalFileStatBase::initialize(CommFabric *c)
{
    if (!c) {
        return false;
    }

    mCommFabric = c;

    return true;
}


const char *
GlobalFileStatBase::getPath() const
{
    return mPath.c_str();
}


FgfsParDesc &
GlobalFileStatBase::getParallelInfo() 
{
    return mParallelInfo;
}


bool
GlobalFileStatBase::hasError()
{
    return mHasErr;
}


///////////////////////////////////////////////////////////////////
//
//  Protected Interface
//
//

const MyMntEnt &
GlobalFileStatBase::getMyEntry() const
{
    return mEntry;
}


int
GlobalFileStatBase::getThresholdToSaturate() const
{
    return mThresholdToSaturate;
}


int
GlobalFileStatBase::setThresholdToSaturate(const int sg)
{
    int rsg = mThresholdToSaturate;
    mThresholdToSaturate = sg;
    return rsg;
}


int
GlobalFileStatBase::getHiLoCutoff() const
{
    return mHiLoCutoff;
}


int
GlobalFileStatBase::setHiLoCutoff(const int th)
{
    int cval = mHiLoCutoff;
    mHiLoCutoff = th;
    return cval;
}


bool
GlobalFileStatBase::isNodeLocal() const
{
    return mNodeLocal;
}


int
GlobalFileStatBase::getCardinalityEst() const
{
    return mCardinalityEst;
}


int
GlobalFileStatBase::setCardinalityEst(const int d)
{
    int rsd = mCardinalityEst;
    mCardinalityEst = d;
    return rsd;
}


uint32_t
GlobalFileStatBase::getPopCount(uint32_t *filter, uint32_t s)
{
    //
    // This algorithm is picked up from Wikipeda regarding
    // Hamming weight algorithms
    //
    uint32_t m1 = 0x55555555;
    uint32_t m2 = 0x33333333;
    uint32_t m4 = 0x0f0f0f0f;
    uint32_t i, cnt=0;
    uint32_t x;

    for (i=0; i < s; ++i) {
        x = *(filter + i);
        x -= (x >> 1) & m1;
        x = (x & m2) + ((x >> 2) & m2);
        x = (x + (x >> 4)) & m4;
        x += x >>  8;
        cnt += (x + (x >> 16)) & 0x3f;
    }

    return cnt;
}


bool
GlobalFileStatBase::computeCardinalityEst(CommAlgorithms algo/*=bloomfilter*/)
{
    mAlgorithm = algo;
    bool rc = false;

    switch (algo) {
    case bloomfilter:
        rc = bloomfilterCardinalityEst();
        break;

    case sampling:
        rc = samplingCardinalityEst();
        break;

    case hier_commsplit:
        rc = hier_commsplitCardinality();
        break;

    case bloomfilter_hier_commsplit:
        rc = bloomfilterCardinalityEst();
        break;

    case sampling_hier_commsplit:
        rc = samplingCardinalityEst();
        break;

    default:
        break;
    }

    return rc;
}


bool
GlobalFileStatBase::computeParallelInfo()
{
    bool rc = false;

    switch (mAlgorithm) {
    case bloomfilter:
        rc = plain_parallelInfo();
        break;

    case sampling:
        rc = samplingCardinalityEst();
        break;

    case hier_commsplit:
        rc = hier_commsplitCardinality();
        break;

    case bloomfilter_hier_commsplit:
        rc = bloomfilterCardinalityEst();
        break;

    case sampling_hier_commsplit:
        rc = samplingCardinalityEst();
        break;

    default:

        break;
    }

    return rc;
}


///////////////////////////////////////////////////////////////////
//
//  Private Interface
//
//
bool
GlobalFileStatBase::bloomfilterCardinalityEst()
{
    int isRemote  = 0;
    int anyRemote = 0;
    int P = (int) mParallelInfo.getSize();
    FGFSInfoAnswer answer = ans_error;

    answer = mpInfo.isRemoteFileSystem(getPath(), mEntry);

    isRemote = IS_YES(answer)? 1 : 0;

    if (!(mCommFabric->allReduce(true,
                                 mParallelInfo,
                                 (void *) &isRemote,
                                 (void *) &anyRemote,
                                 1,
                                 REDUCE_INT,
                                 REDUCE_MAX))) {

        if (ChkVerbose(1)) {
            MPA_sayMessage("GlobalFileStatBase",
                           true,
                           "Error in globalAllReduceIntMAX in bloomfilter");
        }
        goto has_error;
    }

    mHiLoCutoff = P/getThresholdToSaturate();


    if (mpInfo.getFileUriInfo(getPath(), mUri)) {
        if (ChkVerbose(1)) {
            MPA_sayMessage(
                "GlobalFileStatBase",
                true,
                "Error in getFileUriInfo");
        }
        goto has_error;
    }

    //printf("anyRemote: %d\n", anyRemote);

    if (!anyRemote || mHiLoCutoff == 0) {
        //
        // all local, none shared
        //
        setCardinalityEst(P);
        if (!anyRemote) {
            mNodeLocal = true;
        }
    }
    else {

        //
        // Given numHashFuncs=2, n is the number of elements (n) in the set,
        // m is the number of bits in the bloom filter, P is the process count,
        // t is the number of 1s in the filter, we first assume that n exceeding
        // numProcesses/128 is already scalable. This means we want to minimize 
        // the false posive rate when at worse case.
        // Given that the density rate (t/m) of 0.5 provides optimized false 
        // positive rate, and the formulata k=m/2 * ln(2) 
        // m = P/64ln(2)
        //
        //
        // Maximum likelihood of the set cardinality given t is
        //
        // S^-1(t) = ln(1-t/m)/(k*ln(1-1/m))
        //
        //int m = (int) (((double)P/(double)getMaxShareGroupNumProc()) * log(2.0)
        //                  + 0.5);
        int m = (int) (((double)P/(double)getThresholdToSaturate()) * log(2.0)
                          + 0.5);

        // Granularity of m, multiples of 4-bytes
        int numUInt32t = (m+(sizeof(BloomFilterAlign_t)*CHAR_BIT-1))
                          / (sizeof(BloomFilterAlign_t)*CHAR_BIT);
        int numBytes = numUInt32t*sizeof(BloomFilterAlign_t);
        m = numBytes*CHAR_BIT;
        const int k = 2;
        uint32_t t;
        double maxLikelihoodCardinality;

        char *recvbuf = (char *) malloc(numBytes * sizeof(char));

        BLOOM *sendBloom = bloom_create(m, k, sax_hash, sdbm_hash);
        if (!sendBloom) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("GlobalFileStatBase",
                    true,
                    "bloom_create failed bloomfilterCardinalityEst");
            }

            goto has_error;
        }

        std::string uri;
        if (!mUri.getUri(uri)) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("GlobalFileStatBase",
                               true,
                               "getUri failed");
            }

            goto has_error;
        }

        bloom_add(sendBloom, uri.c_str());

        if (!(mCommFabric->allReduce(true,
                                     mParallelInfo,
                                     (void *) sendBloom->a,
                                     (void *) recvbuf,
                                     (int) numBytes,
                                     REDUCE_CHAR_ARRAY,
                                     REDUCE_BOR)) ) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("GlobalFileStatBase",
                               true,
                               "Error in globalAllReduceCharBOR");
            }

            goto has_error;
        }

        t = getPopCount((uint32_t *) recvbuf, numBytes/sizeof(uint32_t));
        maxLikelihoodCardinality = (log(1.0 - (double)t/((double)m))) 
                                    / ((double)k * log(1.0 - 1.0/((double)m)));
        setCardinalityEst((int) (maxLikelihoodCardinality + 0.5));
        bloom_destroy(sendBloom);
        free(recvbuf);
    }

    return true;

has_error:
    return false;
}


bool
GlobalFileStatBase::samplingCardinalityEst()
{
    return false;
}


bool
GlobalFileStatBase::hier_commsplitCardinality()
{
    return false;
}


bool
GlobalFileStatBase::plain_parallelInfo()
{
    bool rc;
    std::string uri;

    if (!mUri.getUri(uri)) {
        if (ChkVerbose(0)) {
            MPA_sayMessage("GlobalFileStatBase",
                           true,
                           "getUri failed");
        }
        rc = false;
    }
    else {
        rc = mCommFabric->grouping(true, mParallelInfo, uri);
    }

    return rc;
}
