/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 24 2011 DHA: File created (Copied from old CommFabric.h)
 *
 */

#ifndef DIST_DESC_H
#define DIST_DESC_H 1

extern "C" {
#include <stdint.h>
}

#include <string>
#include <map>
#include "FgfsCommon.h"

namespace FastGlobalFileStatus {

  namespace CommLayer {

    /**
     *   FGFS_NOT_FILLED
     *   Defines a value to indicate uninitialized integer
     */
    const int FGFS_NOT_FILLED = -1;


    /**
     *   typedef for FgfsId_t
     */
    typedef uint32_t FgfsId_t;


    /**
     *   typedef for FgfsCount_t 
     */
    typedef FgfsId_t FgfsCount_t;


    /**
     *   typedef for BloomFilterAlign_t
     */
    typedef uint32_t BloomFilterAlign_t;


    /**
     *   Enumerates underlying communication fabric types
     */
    enum CommFabricType {
        cft_mpi,
        cft_mrnet,
        cft_cobo,
        cft_unknown
    };


    /**
     *   Enumerates different components for MRNet sublayer
     */
    enum MRNetCompKind {
        mck_frontEnd,
        mck_backEnd,
        mck_commNode,
        mck_unknown
    };


    /**
     *   Provides a data type needed for basic list reduction
     */
    class ReduceDesc {
    public:
        friend class FgfsParDesc;
        ReduceDesc();
        ~ReduceDesc();
        ReduceDesc(const ReduceDesc &r);


        /**
         *   Set the first-seen rank for the tracing item
         *   @param[in] rnk FgfsId_t
         *   @return none
         */
        void setFirstRank(FgfsId_t rnk);


        /**
         *   Increment the count of the ranks that contributed this item
         *   @return none
         */
        void countIncr();


        /**
         *   Increase the count by incr
         *   @param[in] incr FgfsCount_t
         *   @return none
         */
        void incrCountBy(FgfsCount_t incr);

        /**
         *   Get the first-seen rank process
         *   @return FgfsId_t
         */
        FgfsId_t getFirstRank() const;


        /**
         *   Get the count of the rank processes
         *   @return FgfsCount_t
         */
        FgfsCount_t getCount() const;


    private:
        // rd[0]: the first rank seen w.r.t. a unique item
        // rd[1]: count of rank processes
        FgfsId_t rd[2];
    };


     /**
     *   Provides descriptor and reduction data model for this process
     */
    class FgfsParDesc {
    public:
        FgfsParDesc();
        FgfsParDesc(const FgfsParDesc &o);
        ~FgfsParDesc();

        /**
         *   assignment operator
         */
        FgfsParDesc & operator=(const FgfsParDesc &rhs);

        /**
         *   Answers if this process is a representative
         */
        FGFSInfoAnswer isRep() const;


        /**
         *   Answers if the grouping has been performed
         */
        FGFSInfoAnswer isGroupingDone() const;


        /**
         *   Answers if there is only group
         */
        FGFSInfoAnswer isSingleGroup() const;


        /**
         *   Answers if this process is the global master
         */
        FGFSInfoAnswer isGlobalMaster() const;


        /**
         *   Accessors
         */
        void setRank(FgfsId_t rnk);
        FgfsId_t getRank() const;

        void setGlobalMaster();
        void unsetGlobalMaster();

        void setSize(FgfsCount_t sz);
        FgfsCount_t getSize() const;

        void setNumOfGroups(FgfsCount_t sz);
        FgfsCount_t getNumOfGroups() const;

        void setGroupId(FgfsId_t gid);
        FgfsId_t getGroupId() const;

        void setRankInGroup(FgfsId_t grnk);
        FgfsId_t getRankInGroup() const;

        void setGroupSize(FgfsCount_t sz);
        FgfsCount_t getGroupSize() const;

        void setRepInGroup(FgfsId_t rnk);
        FgfsId_t getRepInGroup() const;

        void setUriString(const std::string &uri);
        const std::string & getUriString() const;


        /**
         *   (de)serialers for the reducer map
         */
        size_t pack(char *buf, size_t s) const;
        size_t unpack(char *buf, size_t s);
        size_t packedSize() const;


        /**
         *   operators on the reducer map
         */
        FGFSInfoAnswer insert(std::string &item, ReduceDesc &robj);
        FGFSInfoAnswer mapEmpty();
        FGFSInfoAnswer clearMap();
        std::map<std::string, ReduceDesc>& getGroupingMap();

	
        /**
         *   Eliminates the aliases of file sources in groupingMap
	 *   and condence. Only the global master should call this
	 *   method.
         */
	bool eliminateUriAlias();
        bool adjustUri();


        /**
         *   set group information based on the current state of
         *   this object.
         */
        FGFSInfoAnswer setGroupInfo();


    private:

        /**
         *   Global Info
         */
        FgfsId_t mGlobalRank;
        bool mGlobalMaster;
        FgfsCount_t mSize;
        FgfsCount_t numOfGroups;


        /**
         *   Group Info
         */
        FgfsId_t mGroupId;
        FgfsId_t mRankInGroup;
        FgfsCount_t mGroupSize;
        FgfsId_t mRepInGroup;

        /**
         *    Reduction object for grouping
         */
        std::string mUriString;
        std::map<std::string, ReduceDesc> groupingMap;
    };

  } // CommLayer namespace

} // FastGlobalFileStatus namespace

#endif // DIST_DESC_H

