/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *        Apr 30 2013 DHA: Fix a memory leak in mapReduce 
 *        Jul  7 2011 DHA: File created. (Copied from the old MRNetCommFabric.C)
 *
 */

#include "MRNetCommFabric.h"
#include "MountPointAttr.h"
#include <iostream>
#include <map>

using namespace FastGlobalFileStatus;
using namespace FastGlobalFileStatus::CommLayer;
using namespace FastGlobalFileStatus::MountPointAttribute;
using namespace MRN;


///////////////////////////////////////////////////////////////////
//
//  static data
//
//
Network *MRNetCommFabric::mNetwork = NULL;
Stream *MRNetCommFabric::mStream = NULL;
MRNetCompKind MRNetCommFabric::mMrnetCompType = mck_unknown;
bool MRNetCommFabric::mGlobalMaster = false;
FgfsId_t MRNetCommFabric::mRankCache = FGFS_NOT_FILLED;
FgfsCount_t MRNetCommFabric::mSizeCache = FGFS_NOT_FILLED;



///////////////////////////////////////////////////////////////////
//
//  PUBLIC INTERFACE:   namespace FastGlobalFileStatus::CommLayer
//
//

MRNetCommFabric::MRNetCommFabric()
{

}


MRNetCommFabric::~MRNetCommFabric()
{

}


bool
MRNetCommFabric::initialize(void *net, void *channel)
{
    if (!net) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "network object must be passed.");
        }
        return false;
    }

    bool mthRc = false;
    int mrnetRc;
    mNetwork = (Network *) net;
    mStream = (Stream *) channel;

    mRankCache = (FgfsId_t) mNetwork->get_LocalRank();

    NetworkTopology::Node *myNode;
    myNode = mNetwork->get_NetworkTopology()->find_Node(mRankCache);
    mMrnetCompType = (myNode->get_NumChildren() != 0)
                     ? mck_frontEnd : mck_backEnd;

    mGlobalMaster = (mMrnetCompType == mck_frontEnd)? true : false;

    //
    // [First communcation protocol]
    // broadcast mtk_f2b_initialize into the stream with the size
    // information
    //
    //
    if (mMrnetCompType == mck_frontEnd) {

        Communicator *comm = mNetwork->get_BroadcastCommunicator();
        mSizeCache = (comm->get_EndPoints().size() + 1);

        mrnetRc = mStream->send(MMT_op_init,
                                "%d %d",
                                MMT_op_init,
                                (int) mSizeCache);

        if (mrnetRc == -1) {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "stream send returns an error code");
            }
            goto return_location;
        }

        mStream->flush();
        mthRc = !(mNetwork->has_Error());

    }
    else if (mMrnetCompType == mck_backEnd) {
        int32_t tag;
        PacketPtr recvP;

        if ( (mrnetRc = mStream->recv(&tag, recvP) == -1) ) {
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "stream recv returns an error code.");
            }
            goto return_location;
        }

        int req;
        mrnetRc = recvP->unpack("%d %d", &req, (int *) &mSizeCache);

        if ((mrnetRc == -1) || req != MMT_op_init) {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "stream recv returns an error code or unexpected msg type");
            }
            goto return_location;
        }

        mthRc = !(mNetwork->has_Error());
    }

return_location:
    return mthRc;
}


bool
MRNetCommFabric::allReduce(bool global,
                           FgfsParDesc &pd,
                           void *s,
                           void *r,
                           FgfsCount_t len,
                           ReduceDataType t,
                           ReduceOperator op) const
{
    if (!global) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "MRNet-based allReduce doesn't implement group-wise OP.");
        }
        return false;
    }

    bool mthRc = false;
    int mrnetRc;

    unsigned char *tmpRecv = NULL;
    unsigned int byteSendLen = computeByteLength(t, len);
    unsigned int byteRecvLen = byteSendLen;

    MRNetMsgType oPType = getMRNetMsgType(t, op);

    if (mMrnetCompType == mck_frontEnd) {
        mthRc = allReduceFE((unsigned char *) s,
                            &tmpRecv,
                            byteSendLen,
                            &byteRecvLen,
                            oPType);

        if (!mthRc || byteRecvLen != byteSendLen) {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "allReduceFE returned false or byteRecvLen != byteSendLen");
            }
            goto return_location;
        }
        memcpy(r, (void*) tmpRecv, byteRecvLen);
        free(tmpRecv);
    }
    else if (mMrnetCompType == mck_backEnd) {
        mthRc = allReduceBE((unsigned char *) s,
                            &tmpRecv,
                            byteSendLen,
                            &byteRecvLen,
                            oPType);

        if (!mthRc || byteRecvLen != byteSendLen) {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "allReduceBE returned false or byteRecvLen != byteSendLen");
            }
            goto return_location;
        }
        memcpy(r, (void*) tmpRecv, byteRecvLen);
        free(tmpRecv);
    }

return_location:
    return mthRc;
}


bool
MRNetCommFabric::broadcast(bool global, FgfsParDesc &pd,
                           unsigned char *s, FgfsCount_t len) const
{
    if (!global) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "MRNet-based breaod doesn't implement group-wise OP.");
        }

        return false;
    }

    bool mthRc = false;
    int mrnetRc;

    if (mMrnetCompType == mck_frontEnd) {
        mrnetRc = mStream->send(MMT_op_broadcast_bytes,
                               "%auc",
                               (unsigned char*) s,
                               (unsigned int) len);

        if (mrnetRc == -1) {
            mthRc = false;
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream send returned an error code.");

            goto return_location;
        }
        mStream->flush();

        mthRc = !(mNetwork->has_Error());
    }
    else if (mMrnetCompType == mck_backEnd) {
        int tag;
        unsigned char *ucharArray;
        unsigned int ucharLen;
        PacketPtr recvP;

        mrnetRc = mStream->recv(&tag, recvP);
        if ((mrnetRc == -1) || (tag != MMT_op_broadcast_bytes)) {
            mthRc = false;
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream recv returned an error code.");

            goto return_location;
        }

        mrnetRc = recvP->unpack("%auc",
                                &ucharArray,
                                &ucharLen);

        if ((mrnetRc == -1) || ( ucharLen != (unsigned int) len)) {
            mthRc = false;
            MPA_sayMessage("MRNetCommFabric",
                true,
                "unpack returned an error code or size mismatch.");

            goto return_location;
        }

        memcpy((void*)s, ucharArray, (size_t) len);
        free(ucharArray);

        mthRc = !(mNetwork->has_Error());
    }

return_location:
    return mthRc;
}


bool
MRNetCommFabric::grouping(bool global, 
                          FgfsParDesc &pd,
                          std::string &item,
                          bool elimAlias) const
{
    if (!global) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Only global grouping is supported");
        }
        return false;
    }

    if (IS_NO(pd.mapEmpty())) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "pd.groupingMap isn't empty");
        }
        return false;
    }

    pd.setUriString(item);
    std::vector<std::string> itemList;
    itemList.push_back(item);

    //
    // mapReduce may reset uriString of the pd object.
    //
    mapReduce(global, pd, itemList, elimAlias);

    pd.setGroupInfo();

    return true;
}


bool
MRNetCommFabric::mapReduce(bool global, 
                           FgfsParDesc &pd,
                           std::vector<std::string> &itemList,
                           bool elimAlias) const
{
    if (!global) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "MRNet-based mapReduce doesn't implement group-wise OP.");
        }
        return false;
    }

    std::vector<std::string>::iterator i;

    for (i=itemList.begin(); i != itemList.end(); ++i) {
        ReduceDesc redDescObj;
        redDescObj.setFirstRank(pd.getRank());
        redDescObj.countIncr();
        pd.insert(*i, redDescObj);
    }

    bool mthRc = false;
    int mrnetRc;

    unsigned char *tmpRecv = NULL;
    unsigned int byteSendLen = (unsigned int) pd.packedSize();
    unsigned int byteRecvLen = byteSendLen;

    MRNetMsgType oPType = (elimAlias)? MMT_op_allreduce_map_elim_alias
                                       : MMT_op_allreduce_map;

    unsigned char *bbuf = (unsigned char *) malloc(byteSendLen);
    if (!bbuf) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "malloc returned null");
        }
        goto return_location;
    }
    pd.pack((char *)bbuf, byteSendLen);

    if (mMrnetCompType == mck_frontEnd) {

        mthRc = allReduceFE((unsigned char *) bbuf,
                            &tmpRecv,
                            byteSendLen,
                            &byteRecvLen,
                            oPType);

        if (!mthRc)  {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "allReduceFE returned false or length mismatch");
            }
            goto return_location;
        }

        pd.clearMap();
        pd.unpack((char *)tmpRecv, byteRecvLen);
        if (elimAlias) {
            if (pd.adjustUri()) {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("MRNetCommFabric",
                        false,
                        "URI adjusted");
                }
            }
        }
        free(tmpRecv);
    }
    else if (mMrnetCompType == mck_backEnd) {

        mthRc = allReduceBE((unsigned char *) bbuf,
                            &tmpRecv,
                            byteSendLen,
                            &byteRecvLen,
                            oPType);

        if (!mthRc) {
            mthRc = false;
            if (ChkVerbose(1)) {
                MPA_sayMessage("MRNetCommFabric",
                    true,
                    "allReduceBE returned false or length mismatch");
            }
            goto return_location;
        }

        pd.clearMap();
        pd.unpack((char*)tmpRecv, byteRecvLen);
        if (elimAlias) {
            if (pd.adjustUri()) {
                if (ChkVerbose(1)) {
                    MPA_sayMessage("MRNetCommFabric",
                        false,
                        "URI adjusted");
                }
            }
        }
        free(tmpRecv);
    }
    
    //
    // 4/30/2013 DHA: totalview MemScape found a leak
    //
    free (bbuf);

return_location:
    return mthRc;
}


bool
MRNetCommFabric::getRankSize(int *rank, int *size, bool *glMaster) const
{
    *rank = mRankCache;
    *size = mSizeCache;
    *glMaster = mGlobalMaster;

    return true;
}


void *
MRNetCommFabric::getNet()
{
    return (void *) mNetwork;
}


void *
MRNetCommFabric::getChannel()
{
    return (void *) mStream;
}


///////////////////////////////////////////////////////////////////
//
//  PRIVATE INTERFACE:   namespace FastGlobalFileStatus::CommLayer
//
//

bool
MRNetCommFabric::allReduceFE(unsigned char *sendBuf,
                             unsigned char **recvBuf,
                             unsigned int sendByteLen,
                             unsigned int *recvByteLen,
                             MRNetMsgType oPType) const

{
    bool mthRc = false;
    int mrnetRc;
    int tag;
    unsigned char *ucharArray;
    unsigned int ucharLen;
    PacketPtr recvP;
    unsigned char *retBuf = NULL;
    unsigned int retBufLen = 0;
    unsigned char *firstBytes 
        = (unsigned char *) malloc(sizeof(FGFS_4_BYTES));

    memcpy((void*)firstBytes,
           (void*)FGFS_4_BYTES,
           sizeof(FGFS_4_BYTES));

    mrnetRc = mStream->send(oPType,
                            "%auc",
                            firstBytes,
                            sizeof(FGFS_4_BYTES)); 
    if (mrnetRc == -1) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream send returns an error code!");
        }
        goto return_location;
    }
    mStream->flush();

    mrnetRc = mStream->recv(&tag, recvP);
    if ((mrnetRc == -1) || (tag != oPType)) {
        mthRc = false;
        goto return_location;
    }

    mrnetRc = recvP->unpack("%auc", &ucharArray, &ucharLen);

    if (mrnetRc == -1) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "PacketPtr unpack returns an error code!");
        }
        goto return_location;
    }

    if (!(reduceFinal(sendBuf, sendByteLen,
                      ucharArray, (unsigned int) ucharLen,
                      &retBuf, &retBufLen, oPType)) ) {
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "reduceFinal failed");
        }
    }

    free(ucharArray);

    (*recvBuf) = retBuf;
    *recvByteLen = retBufLen;
    mrnetRc = mStream->send(oPType,
                            "%auc",
                            (*recvBuf),
                            (unsigned int) *recvByteLen);

    if (mrnetRc == -1) {
        mthRc = false;
        goto return_location;
    }
    mStream->flush();

    mthRc = !(mNetwork->has_Error());

return_location:
    return mthRc;
}


bool
MRNetCommFabric::allReduceBE(unsigned char *sendBuf,
                             unsigned char **recvBuf,
                             unsigned int sendByteLen,
                             unsigned int *recvByteLen,
                             MRNetMsgType oPType) const
{
    bool mthRc = false;
    int mrnetRc;
    int tag;
    unsigned char *ucharArray;
    int ucharLen;
    PacketPtr recvP;

    mrnetRc =  mStream->recv(&tag, recvP);
    if ((mrnetRc == -1) || (tag != oPType)) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream recv returns an error code!");
        }
        goto return_location;
    }

    mrnetRc = recvP->unpack("%auc", &ucharArray, &ucharLen);
    if ((mrnetRc == -1) || ucharLen != sizeof(FGFS_4_BYTES)) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "PacketPtr unpack returns an error code!");
        }
        goto return_location;
    }
    free(ucharArray);

    mrnetRc = mStream->send(oPType, "%auc", sendBuf, sendByteLen);
    if (mrnetRc == -1) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream send returns an error code!");
        }
        goto return_location;
    }
    mStream->flush();

    mrnetRc = mStream->recv(&tag, recvP);
    if ((mrnetRc == -1) || (tag != oPType)) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "Stream recv returns an error code!");
        }
        goto return_location;
    }

    mrnetRc = recvP->unpack("%auc", &ucharArray, &ucharLen);
    if (mrnetRc == -1) {
        mthRc = false;
        if (ChkVerbose(1)) {
            MPA_sayMessage("MRNetCommFabric",
                true,
                "PacketPtr unpack returns an error code!");
        }
        goto return_location;
    }

    (*recvBuf) = ucharArray;
    *recvByteLen = ucharLen;

    mthRc = !(mNetwork->has_Error());

return_location:
    return mthRc;
}


unsigned int
MRNetCommFabric::computeByteLength(ReduceDataType t, FgfsCount_t len) const
{
    unsigned int retByteLen = -1;
    switch (t) {
    case REDUCE_INT:
        retByteLen = len * sizeof(int);
        break;

    case REDUCE_LONG_LONG_INT:
        retByteLen = len * sizeof(long long int);
        break;

    case REDUCE_CHAR_ARRAY:
        retByteLen = len;
        break;

    case REDUCE_UNKNOWN_TYPE:
    default:
        break;
    }

    return retByteLen;
}


MRNetMsgType
MRNetCommFabric::getMRNetMsgType(ReduceDataType t, ReduceOperator op) const
{
    // MMT_op_place_holder is the error type in this function
    MRNetMsgType rOp = MMT_place_holder;

    switch (t) {
    case REDUCE_INT: {
        switch (op) {
        case REDUCE_MAX:
            rOp = MMT_op_allreduce_int_max;
            break;

        case REDUCE_MIN:
            rOp = MMT_op_allreduce_int_min;
            break;

        case REDUCE_SUM:
            rOp = MMT_op_allreduce_int_sum;
            break;

        default:
            break;
        }
        break;
    }

    case REDUCE_LONG_LONG_INT: {
        switch (op) {
        case REDUCE_MAX:
            rOp = MMT_op_allreduce_long_long_max;
            break;

        case REDUCE_MIN:
            rOp = MMT_op_allreduce_long_long_min;
            break;

        case REDUCE_SUM:
            rOp = MMT_op_allreduce_long_long_sum;
            break;

        default:
            break;
        }
        break;
    }

    case REDUCE_CHAR_ARRAY: {
        switch (op) {
        case REDUCE_BOR:
            rOp = MMT_op_allreduce_char_bor;
            break;

        default:
            break;
        }
        break;
    }

    case REDUCE_UNKNOWN_TYPE:
    default:
        break;
    }

    return rOp;

}


bool
MRNetCommFabric::reduceFinal(unsigned char *finalBuf,
                             unsigned int finalBufLen,
                             unsigned char *mergedBuf,
                             unsigned int mergedBufLen,
                             unsigned char **retBuf,
                             unsigned int *retLen,
                             MRNetMsgType oPType) const
{
    bool rc = false;

    switch(oPType) {
        case MMT_op_allreduce_int_max: {
            int *feValue = (int *)finalBuf;
            int *meValue = (int *)mergedBuf;
            int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(int)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(int);
            if (!(retIntBuf = (int *) malloc(sizeof(int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = ((*feValue) > (*meValue))
                           ? (*feValue)
                           : (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_int_min: {
            int *feValue = (int *)finalBuf;
            int *meValue = (int *)mergedBuf;
            int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(int)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(int);
            if (!(retIntBuf = (int *) malloc(sizeof(int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = ((*feValue) < (*meValue))
                           ? (*feValue)
                           : (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_int_sum: {
            int *feValue = (int *)finalBuf;
            int *meValue = (int *)mergedBuf;
            int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(int)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(int);
            if (!(retIntBuf = (int *) malloc(sizeof(int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = (*feValue) + (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_long_long_max: {
            long long int *feValue = (long long int *)finalBuf;
            long long int *meValue = (long long int *)mergedBuf;
            long long int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(long long)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(long long int);
            if (!(retIntBuf = (long long int *) malloc(sizeof(long long int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = ((*feValue) > (*meValue))
                           ? (*feValue)
                           : (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_long_long_min: {
            long long int *feValue = (long long int *)finalBuf;
            long long int *meValue = (long long int *)mergedBuf;
            long long int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(long long)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(long long int);
            if (!(retIntBuf = (long long int *) malloc(sizeof(long long int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = ((*feValue) < (*meValue))
                           ? (*feValue)
                           : (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_long_long_sum: {
            long long int *feValue = (long long int *)finalBuf;
            long long int *meValue = (long long int *)mergedBuf;
            long long int *retIntBuf = NULL;

            if (finalBufLen != sizeof(*feValue) ||
                mergedBufLen != sizeof(*meValue) ) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "length mismatch (%d or %d != sizeof(long long)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retLen) = sizeof(long long int);
            if (!(retIntBuf = (long long int *) malloc(sizeof(long long int)))) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retIntBuf) = (*feValue) + (*meValue);
            (*retBuf) = (unsigned char *) retIntBuf;
            rc = true;

            break;
        }

        case MMT_op_allreduce_char_bor: {
            int i;
            if (finalBufLen != mergedBufLen) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "FE buf length (%d) is not equal to merged Buf length (%d)",
                    finalBufLen, mergedBufLen);
                break;
            }

            (*retBuf) = (unsigned char *)
                        malloc (sizeof(unsigned char*) * finalBufLen);
            if (!(*retBuf)) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            (*retLen) = finalBufLen;
            BloomFilterAlign_t *btrav1 = (BloomFilterAlign_t *) finalBuf;
            BloomFilterAlign_t *btrav2 = (BloomFilterAlign_t *) mergedBuf;
            BloomFilterAlign_t *newbtrav = (BloomFilterAlign_t *) (*retBuf);

            for (i=0; i < finalBufLen/sizeof(BloomFilterAlign_t); ++i) {
                *newbtrav = (*btrav1) | (*btrav2);
                btrav1++;
                btrav2++;
                newbtrav++;
            }
            rc = true;

            break;
        }

        case MMT_op_allreduce_map: 
        case MMT_op_allreduce_map_elim_alias: {

            FgfsParDesc pd;
	    pd.setGlobalMaster();

            pd.unpack((char*) finalBuf, (size_t) finalBufLen);
            pd.unpack((char*) mergedBuf, (size_t) mergedBufLen);

            if (oPType == MMT_op_allreduce_map_elim_alias) {
	        if (IS_YES(pd.isGlobalMaster())) {
		    if (pd.eliminateUriAlias()) {
		        if (ChkVerbose(1)) {
		            MPA_sayMessage("MRNetCommFabric",
			  	       false,
				       "Uri Alias eliminated");
		        }
		    }
		}
            }

            (*retLen) = (int) pd.packedSize();
            (*retBuf) = (unsigned char *) malloc(*retLen);
            if (!(*retBuf)) {
                MPA_sayMessage("reduceFinal",
                    true,
                    "malloc returned NULL");
                break;
            }

            pd.pack((char*)(*retBuf), (*retLen));
            rc = true;

            break;
        }

        default: {
            MPA_sayMessage("reduceFinal",
                           true,
                           "Unknown message types: %d",
                           oPType);

            break;
        }
    }

    return rc;
}
