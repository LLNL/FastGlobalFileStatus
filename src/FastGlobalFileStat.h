/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 21 2011 DHA: Copied from the old FastGlobalFileStat.h
 *                         to organize the classes to support sync and async 
 *                         abstractions.
 *        Feb 17 2011 DHA: Added Doxygen style documentation.
 *        Feb 15 2011 DHA: Added StorageClassifier support.
 *        Feb 15 2011 DHA: Changed main higher level abstrations for
 *                         GlobalFileStat to be isUnique, isPoorlyDistributed
 *                         isWellDistributed, and isFullyDistributed.
 *        Jan 13 2011 DHA: File created.
 *
 */

#ifndef FAST_GLOBAL_FILE_STAT_H
#define FAST_GLOBAL_FILE_STAT_H 1

extern "C" {
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include <string>
#include "MountPointAttr.h"
#include "Comm/CommFabric.h"


namespace FastGlobalFileStatus {

    /**
     *   Enumerates algorithm types. These represent internal
     *   alorithms that the fast global file stat uses to
     *   embody its abstractions
     */
    enum CommAlgorithms {
        bloomfilter,
        sampling,
        hier_commsplit,
        bloomfilter_hier_commsplit,
        sampling_hier_commsplit,
        algo_unknown
    };


    /**
     *   Defines per-file API through an abstract class. Any class that
     *   derives this API must implement the pure virtual methods.
     */
    class GlobalFileStatusAPI {
    public:

        /**
         *   GlobalFileStatusAPI Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @return none
         */
        GlobalFileStatusAPI(const char *pth);

        /**
         *   Is the path served in the fully distributed fashion?
         *   The path is served through node local storage. Yes implies
         *   yes for isWellDistributed, but the overhead of this method
         *   is higher than isWellDistributed.
         *   (virtual interface)
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isFullyDistributed() const = 0;

        /**
         *   Is the path served in a well distributed fashion?
         *   The avergage number of processes per file servers is
         *   higher than or equal to thresholdToSaturate.
         *   (virtual interface)
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isWellDistributed() const = 0;

        /**
         *   Negation of isWellDistributed.
         *   (virtual interface)
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isPoorlyDistributed() const = 0;

        /**
         *   Is the path served by a single file server?
         *   Yes implies yes for isPoorlyDistributed, but the overhead
         *   of this method is higher than isPoorlyDistributed.
         *   (virtual interface)
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isUnique() = 0;

        /**
         *   Return the target path
         *   @return a target path string
         */
        const char * getPath() const;

        /**
         *   Return grouping information
         *   @return a FgfsParDesc object
         */
        CommLayer::FgfsParDesc & getParallelInfo();

        /**
         *   Return grouping information
         *   @return a FgfsParDesc object as const
         */
        const CommLayer::FgfsParDesc & getParallelInfo() const;

        /**
         *   Return Uri of FileUriInfo type
         *   @return an Uri
         */
        MountPointAttribute::FileUriInfo & getUriInfo();

        /**
         *   Return Uri of FileUriInfo type
         *   @return a FileUriInfo object as const
         */
        const MountPointAttribute::FileUriInfo & getUriInfo() const;

        /**
         *   Return MyMntEnt
         *   @return a MyMntEnt object
         */
        MountPointAttribute::MyMntEnt & getMyEntry();

        /**
         *   Return MyMntEnt
         *   @return a MyMntEnt object of const
         */
        const MountPointAttribute::MyMntEnt & getMyEntry() const;

        /**
         *   Check if the path globally node-local
         *   @return yes or no of bool type
         */
        bool isNodeLocal() const;

        /**
         *   Check if the path globally node-local
         *   @return yes or no of bool type
         */
        bool setNodeLocal(bool b);

        /**
         *   Return cardinality estimate
         *   @return an integer cardinality
         */
        int getCardinalityEst() const;

        /**
         *   Set CardinalityEst
         *   @param[in] new  CardinalityEst of integer type
         *   @return the old CardinalityEst of integer type
         */
        int setCardinalityEst(const int d);


    private:

        /**
         *   target path
         */
        std::string mPath;

        /**
         *   parallel information such as your repr task and ranks
         */
        CommLayer::FgfsParDesc mParallelInfo;

        /**
         *   globally unique file ID for path on this node
         */
        MountPointAttribute::FileUriInfo mUriInfo;

        /**
         *   Local MyMntEnt entry for m_path
         */
        MountPointAttribute::MyMntEnt mEntry;

        /**
         *   is m_path local
         */
        bool mNodeLocal;

        /**
         *   max likelihood of the set cardinality
         */
        int mCardinalityEst;
    };


    /**
     *   Base class that serves as a high-level Global File Stat 
     *   interface in a underlying communication fabric independent manner.
     */
    class GlobalFileStatusBase {
    public:

        /**
         *   GlobalFileStatBase::FGFS_NPROC_TO_SATURATE (64)
         *   Defines the process count that serves as a
         *   threshold above which to saturate a shared file server.
         */
        static const int FGFS_NPROC_TO_SATURATE = 64;

        /**
         *   GlobalFileStatBase::FGFS_UNIQUE_RANGE_MAX (3)
         *   For small scale cases, even wellDistributed cases can be unique
         *   so upper layer should do an additional check of
         *   if getCardinalityEst is leq FGFS_UNIQUE_RANGE_MAX
         */
        static const int FGFS_UNIQUE_RANGE_MAX = 3;

        GlobalFileStatusBase();

        GlobalFileStatusBase(int threshold);

        virtual ~GlobalFileStatusBase();

        /**
         *   Return the MountPointInfo object, class static object holding
         *   all the information about the per-node mount points
         *
         *   @return an MountPointInfo object
         */
        static const MountPointAttribute::MountPointInfo & getMpInfo();

        /**
         *   Return the CommFabric object, class static object holding
         *   communcation fabric layer
         *
         *   @return a CommFabric object
         */
        static const CommLayer::CommFabric *getCommFabric();

        /**
         *   Initialize including communication fabric bootstrapping
         *
         *   @param[in] c CommFabric object
         *
         *   @return a bool value
         */
        static bool initialize(CommLayer::CommFabric *c);

        /**
         *   Check if there has been an error encountered
         *   @return true if an error
         */
        bool hasError();


    protected:

        /**
         *   Return ThresholdToSaturate
         *   @return the current threshold of int
         */
        int getThresholdToSaturate() const;

        /**
         *   Set ThresholdToSaturate
         *   @param[in] new threshold
         *   @return the old int threshold
         */
        int setThresholdToSaturate(const int sg);

        /**
         *   Get high low cutoff point
         *   @return high low cutoff point of integer type
         */
        int getHiLoCutoff() const;

        /**
         *   Set high low cutoff point
         *   @param[in] new high low point of integer type
         *   @return the old high low point of integer type
         */
        int setHiLoCutoff(const int th);

        /**
         *   Performs necessary communication to determine global
         *   properties including a number of different equivalent
         *   groups
         *   @param[in|out] a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool computeParallelInfo(GlobalFileStatusAPI *gfsObj,
                                 CommAlgorithms algo=bloomfilter);

        /**
         *   Performs cardinality estimate: how many different equivalent 
         *   groups are there? The client can get this estimate with
         *   the getCardinalityEst method
         *   @param[in|out] a GlobalFileStatAPI object
         *   @param[in] an algorithm type of CommAlgorithms
         *   @return success or failure of bool type
         */
        bool computeCardinalityEst(GlobalFileStatusAPI *gfsObj,
                                   CommAlgorithms algo=bloomfilter);

        /**
         *   Performs cardinality estimate based on the bloomfilter
         *   algorithm.
         *   @param[in|out] a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool bloomfilterCardinalityEst(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality estimate based on the sampling
         *   algorithm: NotImplemented
         *   @param[in|out] a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool samplingCardinalityEst(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality estimate based on the generalized
         *   comm-split: NotImplemented
         *   @param[in|out] a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool hier_commsplitCardinality(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality check (accurate) based on the
         *   actual list reduction: NotImplemented
         *   @param[in|out] a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool plain_parallelInfo(GlobalFileStatusAPI *gfsObj);


    private:

        GlobalFileStatusBase(const GlobalFileStatusBase &s);

        uint32_t getPopCount(uint32_t *filter, uint32_t s);

        /**
         *   error indicator
         */
        bool mHasErr;

        /**
         *   algorithm selector
         */
        CommAlgorithms mAlgorithm;

        /**
         *   a process count that would saturate a single file server
         */
        int mThresholdToSaturate;

        /**
         *   cutoff in terms of p/maxNPPerGroup
         */
        int mHiLoCutoff;

        /**
         *   communication fabric (does this have to be static?)
         */
        static CommLayer::CommFabric *mCommFabric;

        /**
         *   per-node mount point information
         */
        static MountPointAttribute::MountPointInfo mpInfo;
    };
}

#endif // FAST_GLOBAL_FILE_STAT_H

