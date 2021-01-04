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
#include <limits.h>
#include <fcntl.h>
#include <sys/vfs.h>
#include "bloom.h"
}

#include <iostream>
#include <sstream>
#include <stdexcept>
#include "FastGlobalFileStat.h"
#include "config.h"

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace FastGlobalFileStatus::CommLayer;

///////////////////////////////////////////////////////////////////
//
//  static data
//
//
CommFabric *GlobalFileStatusBase::mCommFabric = NULL;
MountPointInfo GlobalFileStatusBase::mpInfo(true);


///////////////////////////////////////////////////////////////////
//
//  Public Interface
//
//


///////////////////////////////////////////////////////////////////
//
//  class GlobalFileStatusAPI
//
//


GlobalFileStatusAPI::GlobalFileStatusAPI(const char *pth)
    : mCardinalityEst(FGFS_NOT_FILLED),
      mNodeLocal(false)
{
    mPath = pth;
}


const char *
GlobalFileStatusAPI::getPath() const
{
    return mPath.c_str();
}


CommLayer::FgfsParDesc &
GlobalFileStatusAPI::getParallelInfo()
{
    return mParallelInfo;
}


const CommLayer::FgfsParDesc &
GlobalFileStatusAPI::getParallelInfo() const
{
    return mParallelInfo;
}


MountPointAttribute::FileUriInfo &
GlobalFileStatusAPI::getUriInfo()
{
    return mUriInfo;
}


const MountPointAttribute::FileUriInfo &
GlobalFileStatusAPI::getUriInfo() const
{
    return mUriInfo;
}


MyMntEnt &
GlobalFileStatusAPI::getMyEntry()
{
    return mEntry;
}


const MyMntEnt &
GlobalFileStatusAPI::getMyEntry() const
{
    return mEntry;
}


bool
GlobalFileStatusAPI::isNodeLocal() const
{
    return mNodeLocal;
}


bool
GlobalFileStatusAPI::setNodeLocal(bool b)
{
    mNodeLocal = b;
    return true;
}


int
GlobalFileStatusAPI::getCardinalityEst() const
{
    return mCardinalityEst;
}


int
GlobalFileStatusAPI::setCardinalityEst(const int d)
{
    int rsd = mCardinalityEst;
    mCardinalityEst = d;
    return rsd;
}


///////////////////////////////////////////////////////////////////
//
//  class GlobalFileStatusBase
//
//


GlobalFileStatusBase::GlobalFileStatusBase()
    : mHasErr(false),
      mAlgorithm(algo_unknown),
      mThresholdToSaturate(FGFS_NPROC_TO_SATURATE),
      mHiLoCutoff(FGFS_NOT_FILLED)
{
    if (!mCommFabric) {
        mHasErr = true;
    }
}


GlobalFileStatusBase::GlobalFileStatusBase(const int value)
    : mHasErr(false),
      mAlgorithm(algo_unknown),
      mHiLoCutoff(FGFS_NOT_FILLED)
{
    mThresholdToSaturate = value;
    if (!mCommFabric) {
        mHasErr = true;
    }
}


GlobalFileStatusBase::~GlobalFileStatusBase()
{
    // mCommFabric must not be deleted!
}


const MountPointInfo &
GlobalFileStatusBase::getMpInfo()
{
    return mpInfo;
}


const CommFabric *
GlobalFileStatusBase::getCommFabric()
{
    return mCommFabric;
}


bool
GlobalFileStatusBase::initialize(CommFabric *c)
{
    if (!c) {
        return false;
    }

    mCommFabric = c;

    return true;
}


bool
GlobalFileStatusBase::hasError()
{
    return mHasErr;
}


///////////////////////////////////////////////////////////////////
//
//  Protected Interface
//
//

int
GlobalFileStatusBase::getThresholdToSaturate() const
{
    return mThresholdToSaturate;
}


int
GlobalFileStatusBase::setThresholdToSaturate(const int sg)
{
    int rsg = mThresholdToSaturate;
    mThresholdToSaturate = sg;
    return rsg;
}


int
GlobalFileStatusBase::getHiLoCutoff() const
{
    return mHiLoCutoff;
}


int
GlobalFileStatusBase::setHiLoCutoff(const int th)
{
    int cval = mHiLoCutoff;
    mHiLoCutoff = th;
    return cval;
}


uint32_t
GlobalFileStatusBase::getPopCount(uint32_t *filter, uint32_t s)
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
GlobalFileStatusBase::computeCardinalityEst(GlobalFileStatusAPI *gfsObj,
                                            CommAlgorithms algo/*=bloomfilter*/)
{
    mAlgorithm = algo;
    bool rc = false;

    switch (algo) {
    case bloomfilter:
        rc = bloomfilterCardinalityEst(gfsObj);
        break;

    case sampling:
        rc = samplingCardinalityEst(gfsObj);
        break;

    case hier_commsplit:
        rc = hier_commsplitCardinality(gfsObj);
        break;

    case bloomfilter_hier_commsplit:
        rc = bloomfilterCardinalityEst(gfsObj);
        break;

    case sampling_hier_commsplit:
        rc = samplingCardinalityEst(gfsObj);
        break;

    default:
        break;
    }

    return rc;
}


bool
GlobalFileStatusBase::computeParallelInfo(GlobalFileStatusAPI *gfsObj,
                                          CommAlgorithms algo/*=bloomfilter*/)

{
    bool rc = false;
    CommAlgorithms algoToUse;
    algoToUse = mAlgorithm;

    if (algoToUse == algo_unknown) {
        algoToUse = algo;
    }

    switch (algoToUse) {
    case bloomfilter:
        rc = plain_parallelInfo(gfsObj);
        break;

    case sampling:
        rc = samplingCardinalityEst(gfsObj);
        break;

    case hier_commsplit:
        rc = hier_commsplitCardinality(gfsObj);
        break;

    case bloomfilter_hier_commsplit:
        rc = bloomfilterCardinalityEst(gfsObj);
        break;

    case sampling_hier_commsplit:
        rc = samplingCardinalityEst(gfsObj);
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
GlobalFileStatusBase::bloomfilterCardinalityEst(GlobalFileStatusAPI *gfsObj)
{
    int isRemote  = 0;
    int anyRemote = 0;
    int P = (int) (gfsObj->getParallelInfo().getSize());
    FGFSInfoAnswer answer = ans_error;

    answer = mpInfo.isRemoteFileSystem(gfsObj->getPath(), gfsObj->getMyEntry());

    isRemote = IS_YES(answer)? 1 : 0;

    if (!(mCommFabric->allReduce(true,
                                 gfsObj->getParallelInfo(),
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

    //
    // divide the process count by the saturation threshold
    // If mHiLoCutoff = 0, not enough process count to saturate 
    // any file system.
    //
    //
    mHiLoCutoff = P/getThresholdToSaturate();

    if (mpInfo.getFileUriInfo(gfsObj->getPath(), gfsObj->getUriInfo())) {
        if (ChkVerbose(1)) {
            MPA_sayMessage(
                "GlobalFileStatBase",
                true,
                "Error in getFileUriInfo");
        }
        goto has_error;
    }

    if (!anyRemote || mHiLoCutoff == 0) {
        //
        // all local, none shared
        //
        gfsObj->setCardinalityEst(P);
        if (!anyRemote) {
            gfsObj->setNodeLocal(true);
        }
    }
    else {

        //
        // Given numHashFuncs=2, n is the number of elements (n) in the set,
        // m is the number of bits in the bloom filter, P is the process count,
        // t is the number of 1s in the filter.
        //
        // Now, we want to determine the m size such that it minimizes
        // the false posive rate at worse case.
        // Given that the density rate (t/m) of 0.5 provides optimimal false 
        // positive rate, and the density is achieved when 
        // k=m/n * ln(2), where k=2, m is the number of bits and n is the number
        // of unique items. At worse case there can be P unique items
        // (n=P): m = 2*P / ln(2)
        //
        // Optimizations: any locally attached storage will be detected w/o
        // this triaging. Therefore, one can optimize it by calculating
        // unique hostnames
        //
        // Since we are no longer concerned about local, the worse will
        // be distributed case: So the stack can be configured w/
        // the worse number of distributed servers. On LLNL Linux,
        // there is one system nfs server per a scalable unit,
        //  == 156 nodes with 16 cores on Zin, for example. 
        //  Worst distributed case = 20. 
        //
        // 
        //
        //
        //
        // Maximum likelihood of the set cardinality given t is
        //
        // S^-1(t) = ln(1-t/m)/(k*ln(1-1/m))
        //
	// int m = (int) (((double)P/(double)getThresholdToSaturate()) * log(2.0)
        //                  + 0.5);
        //
        //
        
#ifdef MAX_DEGREE_DISTRIBUTION

        // log(2.0): 0.693147 
        int m = (int) ceil(double(2*MAX_DEGREE_DISTRIBUTION) / 0.693147);
        if (ChkVerbose(1)) {
            MPA_sayMessage("GlobalFileStatusBase",
                false,
                "max distribution degree given.");
        }
#else
        // site-wide worse case not-known. This is absolutely the worst case
        // assuming each process will access different server
        // log(2.0): 0.693147 
        int m = (int)ceil(((double)2*P) / 0.693147); 
        if (ChkVerbose(1)) {
            MPA_sayMessage("GlobalFileStatusBase",
                false,
                "bloom filter size is probably overestimated: sub-optimal performance.");
        }
#endif 

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
                MPA_sayMessage("GlobalFileStatusBase",
                    true,
                    "bloom_create failed bloomfilterCardinalityEst");
            }

            goto has_error;
        }

        std::string uri;
        if (!gfsObj->getUriInfo().getUri(uri)) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("GlobalFileStatusBase",
                               true,
                               "getUri failed");
            }

            goto has_error;
        }

        bloom_add(sendBloom, uri.c_str());

        if (!(mCommFabric->allReduce(true,
                                     gfsObj->getParallelInfo(),
                                     (void *) sendBloom->a,
                                     (void *) recvbuf,
                                     (int) numBytes,
                                     REDUCE_CHAR_ARRAY,
                                     REDUCE_BOR)) ) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("GlobalFileStatusBase",
                               true,
                               "Error in globalAllReduceCharBOR");
            }

            goto has_error;
        }

        t = getPopCount((uint32_t *) recvbuf, numBytes/sizeof(uint32_t));
        maxLikelihoodCardinality = (log(1.0 - (double)t/((double)m)))
                                    / ((double)k * log(1.0 - 1.0/((double)m)));
        gfsObj->setCardinalityEst((int) (maxLikelihoodCardinality + 0.5));
        bloom_destroy(sendBloom);
        free(recvbuf);
    }

    return true;

has_error:
    return false;
}


bool
GlobalFileStatusBase::samplingCardinalityEst(GlobalFileStatusAPI *gfsObj)
{
    return false;
}


bool
GlobalFileStatusBase::hier_commsplitCardinality(GlobalFileStatusAPI *gfsObj)
{
    return false;
}


bool
GlobalFileStatusBase::plain_parallelInfo(GlobalFileStatusAPI *gfsObj)
{
    bool rc;
    std::string uri;

    if (!gfsObj->getUriInfo().getUri(uri)) {
        if (ChkVerbose(0)) {
            MPA_sayMessage("GlobalFileStatusBase",
                           true,
                           "getUri failed");
        }
        rc = false;
    }
    else {
        rc = mCommFabric->grouping(true, 
                 gfsObj->getParallelInfo(), 
                 uri,
                 true /* eliminate alias */);
    }

    return rc;
}

