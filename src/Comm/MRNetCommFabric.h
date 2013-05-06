/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jul 7 2011 DHA: File created.
 *
 */

#ifndef MRNET_COMM_FABRIC_H
#define MRNET_COMM_FABRIC_H 1

extern "C" {
#include <stdint.h>
}

#include "mrnet/MRNet.h"
#include "CommFabric.h"
#include <map>
#include <vector>
#include <string>

namespace FastGlobalFileStatus {

  namespace CommLayer {


    /**
     *   FGFS_MRNET_REDUCTION_TAG
     *   Defines a value for Base tag value for MRNET sublayer
     */
    const int FGFS_MRNET_TAG_BASE = 3916;


    /**
     *   FGFS_UP_FILTER_SO_NAME
     *   Defines a upstream filter SO used for custom reduction
     */
    const char FGFS_UP_FILTER_SO_NAME[] = "libfgfs_filter.so";


    /**
     *   FGFS_DOWN_FILTER_SO_NAME
     *   Defines a upstream filter SO used for custom reduction
     */
    const char FGFS_DOWN_FILTER_SO_NAME[] = "libfgfs_filter.so";


    /**
     *   FGFS_UP_FILTER_FN_NAME
     *   Defines a downstream filter SO used for custom reduction
     */
    const char FGFS_UP_FILTER_FN_NAME[] = "FGFSFilterUp";


    /**
     *   FGFS_DOWN_FILTER_FN_NAME
     *   Defines a downstream filter SO used for custom reduction
     */
    const char FGFS_DOWN_FILTER_FN_NAME[] = "FGFSFilterDown";


    /**
     *   FGFS_DOWN_FILTER_SO_NAME
     *   Defines a upstream filter SO used for custom reduction
     */
    const unsigned char FGFS_4_BYTES[] = {'F', 'G', 'F', 'S'};


    enum MRNetMsgType {
        MMT_op_init = FGFS_MRNET_TAG_BASE,
        MMT_op_broadcast_bytes,
        MMT_op_allreduce_int_max,
        MMT_op_allreduce_int_min,
        MMT_op_allreduce_int_sum,
        MMT_op_allreduce_long_long_max,
        MMT_op_allreduce_long_long_min,
        MMT_op_allreduce_long_long_sum,
        MMT_op_allreduce_char_bor,
        MMT_op_allreduce_map,
        MMT_op_allreduce_map_elim_alias,
        MMT_debug_mpir,
        MMT_place_holder
    };


    /**
     *
     * Defines the MRNet-based communication fabric class.
     */
    class MRNetCommFabric: public CommFabric {
    public:

        /**
         *   CommFabric Ctor
         *
         */
        MRNetCommFabric();

        /**
         *   CommFabric Dtor
         *
         */
        virtual ~MRNetCommFabric();

        /**
         *   Class Static Initializer
         *
         *   @param[in] net opaque network object
         *   @param[in] channel opaque channel object
         *
         *   @return a bool value
         */
        static bool initialize(void *net=NULL, void *channel=NULL);

        /**
         *   MRNet-based allReduce
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[in] pd an FgfsStatDesc object
         *   @param[in] s source buffer
         *   @param[out] r receiver buffer
         *   @param[in] len length of the buffer
         *   @param[in] t ReduceDataType
         *   @param[in] op ReduceOperator
         *
         *   @return a bool value
         */
        virtual bool allReduce(bool global,
                               FgfsParDesc &pd,
                               void *s,
                               void *r,
                               FgfsCount_t len,
                               ReduceDataType t,
                               ReduceOperator op) const;

        /**
         *   MRNet-based broadcast
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[in] pd an FgfsStatDesc object
         *   @param[in] s source buffer
         *   @param[in] len length of the buffer
         *
         *   @return a bool value
         */
        virtual bool broadcast(bool global,
                               FgfsParDesc &pd,
                               unsigned char *s,
                               FgfsCount_t len) const;

        /**
         *   MRNet-based grouping
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[out] pd an FgfsStatDesc object
         *   @param[in] item data item to group
         *   @param[in] elimAlias flag that forces uri alias elimination
         *
         *   @return a bool value
         */
        virtual bool grouping(bool global,
                              FgfsParDesc &pd,
                              std::string &item,
                              bool elimAlias) const;


        /**
         *   MRNet-based mapReduce
         *
         *   @param[in] global bool indicating global vs. group
         *   @param[out] pd an FgfsStatDesc object
         *   @param[in] itemList a item list containing unique item (vector type)
         *   @param[in] elimAlias flag that forces uri alias elimination
         *
         *   @return a bool value
         */
        virtual bool mapReduce(bool global,
                               FgfsParDesc &pd,
                               std::vector<std::string> &itemList,
                               bool elimAlias) const;


        /**
         *   MRNet-based getRankSize
         *
         *   @param[out] rank pointer to an int
         *   @param[out] size pointer to an int
         *   @param[out] glMaster pointer to a bool (true if a global master)
         *
         *   @return a bool value
         */
        virtual bool getRankSize(int *rank, int *size, bool *glMaster) const;

        /**
         *   Virtual Interface: return the net object
         *
         *   @return an opaque object pointer
         */
        virtual void *getNet();

        /**
         *   Virtual Interface: return the channel object

         *   @return an opaque object pointer
         */
        virtual void *getChannel();


    private:

        MRNetCommFabric(const CommFabric &c);

        bool allReduceFE(unsigned char *sendBuf,
                         unsigned char **recvBuf,
                         unsigned int sndByteLen,
                         unsigned int *rcvByteLen,
                         MRNetMsgType oPType) const;

        bool allReduceBE(unsigned char *sendBuf,
                         unsigned char **recvBuf,
                         unsigned int sndByteLen,
                         unsigned int *rcvByteLen,
                         MRNetMsgType oPType) const;

        bool reduceFinal(unsigned char *finalBuf,
                         unsigned int finalBufLen,
                         unsigned char *mergedBuf,
                         unsigned int mergedBufLen,
                         unsigned char **retBuf,
                         unsigned int *retLen,
                         MRNetMsgType oPType) const;

        unsigned int computeByteLength(ReduceDataType t,
                         FgfsCount_t len) const;

        MRNetMsgType getMRNetMsgType(ReduceDataType t,
                         ReduceOperator op) const;

        static MRNetCompKind mMrnetCompType;
        static bool mGlobalMaster;
        static FgfsId_t mRankCache;
        static FgfsCount_t mSizeCache;
        static MRN::Network *mNetwork;
        static MRN::Stream *mStream;
    };

  } // CommLayer namespace

} // FastGlobalFileStatus namespace
#endif //MRNET_COMM_FABRIC_H

