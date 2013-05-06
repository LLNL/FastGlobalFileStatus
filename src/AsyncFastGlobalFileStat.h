/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jun 21 2011 DHA: File created
 *
 */

#ifndef ASYNC_FAST_GLOBAL_FILE_STAT_H
#define ASYNC_FAST_GLOBAL_FILE_STAT_H 1

#include "SyncFastGlobalFileStat.h"
#include "MountPointsClassifier.h"

namespace FastGlobalFileStatus {

    /**
     *   Provides asynchronous abstractions of Fast Global File Stat.
     */
    class AsyncGlobalFileStatus : public GlobalFileStatusAPI, public GlobalFileStatusBase {
    public:

        /**
         *   AsyncGlobalFileStat Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @return none
         */
        AsyncGlobalFileStatus(const char *pth);

        /**
         *   SyncGlobalFileStat Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @param[in] threshold a process count to staturate the
         *                         file server; when you want to overwrite
         *                         the default threshold.
         *   @return none
         */
        AsyncGlobalFileStatus(const char *pth, const int threshold);

        virtual ~AsyncGlobalFileStatus();

        /**
         *   Initializer
         *
         *   @param[in] c CommFabric object
         *   @return a bool value
         */
        static bool initialize(CommLayer::CommFabric *c);

	/**
         *   printMpClassifier
         *
         *   @return a bool value
         */
        static bool printMpClassifier();

        /**
         *   Is the path served in the fully distributed fashion?
         *   The path is served through node local storage. Yes implies
         *   yes for isWellDistributed, but the overhead of this method
         *   is higher than isWellDistributed.
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isFullyDistributed() const;

        /**
         *   Is the path served in a well distributed fashion?
         *   The avergage number of processes per file servers is
         *   higher than or equal to thresholdToSaturate.
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isWellDistributed() const;

        /**
         *   Negation of isWellDistributed.
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isPoorlyDistributed() const;

        /**
         *   Is the path served by a single file server?
         *   Yes implies yes for isPoorlyDistributed, but the overhead
         *   of this method is higher than isPoorlyDistributed.
         *
         *   @return an FGFSInfoAnswer object
         */
        virtual FGFSInfoAnswer isUnique();

    private:

        static MountPointsClassifier mMpClassifier;
    };

}

#endif // ASYNC_FAST_GLOBAL_FILE_STAT_H
