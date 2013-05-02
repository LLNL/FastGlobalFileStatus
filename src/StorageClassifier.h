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
 *                         independent class.
 *
 */

#ifndef STORAGE_CLASSIFIER_H
#define STORAGE_CLASSIFIER_H 1

#include "SyncFastGlobalFileStat.h"
#include "MountPointsClassifier.h"


namespace FastGlobalFileStatus {

  namespace StorageInfo {

    typedef uint64_t nbytes_t;


    /**
     *   Defines a data type that inherits SyncGlobalFileStatus
     *   to help check space requirement for each mount-point-path.
     */
    class GlobalStorageChecker : public SyncGlobalFileStatus {
    public:

        /**
         *   GlobalStorageChecker Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @return none
         */
       GlobalStorageChecker(const char *pth);

        /**
         *   GlobalStorageChecker Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @param[in] threshold a process count to staturate the
         *                         file server; when you want to overwrite
         *                         the default threshold.
         *   @return none
         */
        GlobalStorageChecker(const char *pth, const int threshold);

        virtual ~GlobalStorageChecker();

        /**
         *   Class Static Initializer. (Call once for the entire class hierarchy)
         *   This is a global collective: all distributed component must
         *   call synchronously.
         *
         *   @param[in] c pointer to a CommFabric object
         *   @return a bool value
         */
        static bool initialize(CommLayer::CommFabric *c);

        FGFSInfoAnswer meetSpaceRequirement(nbytes_t bytesNeededlocally,
                               nbytes_t *bytesNeededGlobally,
                               nbytes_t *bytesNeededWithinGroup,
                               nbytes_t *bytesAvailableWithinGroup,
                               int *distEst);

    };


    /**
     *   Defines a data type to specify storage selection criteria.
     *   spaceRequirement is the only mandatory requirement to set
     */
    class FileSystemsCriteria {
    public:

        typedef nbytes_t SpaceRequirement;

        /**
         * SPEED_XXX:        Serial access speed to the storage
         * DISTRIBUTION_XXX: How many remote file servers serve the storage
         * FS_SCAL_XXX: Interent scalability of the storage
         */
        enum SpeedRequirement {
            SPEED_REQUIRE_NONE = 0x00000000, /*!< no requirement */
            SPEED_LOW          = 0x00000001, /*!< Requires low serial access performance */
            SPEED_HIGH         = 0x00000002  /*!< Requires high serial access performance */
        };

        enum DistributionRequirement {
            DISTRIBUTED_REQUIRE_NONE = 0x00000000, /*!< no requirement */
            DISTRIBUTED_UNIQUE = 0x00000003, /*!< Requires globally shared storage (Unique) */
            DISTRIBUTED_LOW    = 0x00000004, /*!< Requires lowly distributed storage (Poorly including Unique) */
            DISTRIBUTED_HIGH   = 0x00000005, /*!< Requires highly distributed storage (Well including Fully) */
            DISTRIBUTED_FULL   = 0x00000006  /*!< Requires fully distributed storage (Fully) */
        };

        enum ScalabilityRequirement {
            FS_SCAL_REQUIRE_NONE  = 0x00000000, /*!< no requirement */
            FS_SCAL_SINGLE     = 0x00000007, /*!< Requires serial storage */
            FS_SCAL_MULTI      = 0x00000008  /*!< Requires parallel storage */
        };

        FileSystemsCriteria();

        FileSystemsCriteria(nbytes_t bytesNeeded,
                        nbytes_t bytesFree=0,
                        SpeedRequirement speed=SPEED_REQUIRE_NONE,
                        DistributionRequirement dist=DISTRIBUTED_REQUIRE_NONE,
                        ScalabilityRequirement scal=FS_SCAL_REQUIRE_NONE);

        FileSystemsCriteria(const FileSystemsCriteria &criteria);

        ~FileSystemsCriteria();

        void setSpaceRequirement(nbytes_t bytesNeeded,
                                 nbytes_t bytesToFree);

        void setDistributionRequirement(DistributionRequirement dist);

        void setSpeedRequirement(SpeedRequirement speed);

        void setScalabilityRequirement(ScalabilityRequirement scale);

        const SpaceRequirement getSpaceRequirement() const;

        const nbytes_t getSpaceToFree() const;

        const DistributionRequirement getDistributionRequirement() const;

        const SpeedRequirement getSpeedRequirement() const;

        const ScalabilityRequirement getScalabilityRequirement() const;

        bool isRequireNone() const;


    private:

        nbytes_t spaceRequirement;

        nbytes_t spaceToFree;

        SpeedRequirement speedRequirement;

        DistributionRequirement distributionRequirement;

        ScalabilityRequirement scalabilityRequirement;
    };


    struct MyMntEntWScore {
        MountPointAttribute::MyMntEnt mpEntry;
        int score;
    };


    /**
     *   Defines a data type that provides best file systems based
     *   on the criteria. The provideBestFileSystem method  and
     *   provideMatchingFileSystem are the key abstractions. 
     *
     *   Only one type of constructor is allowed to be called;
     *   an object of this type cannot be copied or assigned.
     */
    class GlobalFileSystemsStatus : public GlobalFileStatusBase {
    public:

        /**
         *   STORAGE_CLASSIFIER_MIN_SCORE -1
         *   Defines the minimum storage classifier score.
         */
        static const int STORAGE_CLASSIFIER_REQ_UNMET_SCORE = -1;

        /**
         *   STORAGE_CLASSIFIER_MIN_MATCH_SCORE 0.0
         *   Defines the minimum storage classifier score
         *   for a storage that matches the criteria.
         */
        static const int STORAGE_CLASSIFIER_REQ_MET_SCORE = 0;

        /**
         *   STORAGE_CLASSIFIER_MIN_MATCH_SCORE 0.0
         *   Defines the minimum storage classifier score
         *   for a storage that matches the criteria.
         */
        static const int STORAGE_CLASSIFIER_MAX_SCORE = 10000;

        /**
         *   STORAGE_CLASSIFIER_BASE_DEVICE_SPEED 1
         *   Defines the base device speed (storage's serial access).
         *   A device's speed would be a multiple (including 1x)
         *   of this value.
         */
        static const int STORAGE_CLASSIFIER_BASE_DEVICE_SPEED = 1;

        /**
         *   STORAGE_CLASSIFIER_MAX_DEVICE_SPEED 1.0
         *   Defines the base device speed (storage's serial access).
         *   10x of STORAGE_CLASSIFIER_BASE_DEVICE_SPEED
         */
        static const int STORAGE_CLASSIFIER_MAX_DEVICE_SPEED = 10;

        /**
         *   STORAGE_CLASSIFIER_BASE_FS_SCALABILITY 1.0
         *   Defines the base FS scalabilty; storage's inherent
         *   scalability is defined to be a multiple (including 1x)
         *   of this value.
         */
        static const int STORAGE_CLASSIFIER_BASE_FS_SCALABILITY = 1;

        /**
         *   StorageClassifier Ctor
         *   @return none
         */
        GlobalFileSystemsStatus();

        /**
         *   StorageClassifier Dtor
         *
         *   @return none
         */
        virtual ~GlobalFileSystemsStatus();

        /**
         *   Initializer
         *
         *   @param[in] c CommFabric object
         *   @return a bool value
         */
        static bool initialize(CommLayer::CommFabric *c);

        /**
         *  Returns the static mount point classifier object which has
         *  grouping information on all of the mount points
         * 
         */
        static const MountPointsClassifier & getMountPointsClassifier();

        /**
         *   Examines all of the file systems mounted across all of the
         *   processes and find matching file systems based on the
         *   criteria specified through the criteria object. Any of the optional
         *   requirement can be given with this method. Returning mount
         *   point vectors are in descending score order. Moint point with
         *   a higher score is at the front of the vector.
         *
         *   Note that this method does not take into account quota and permission
         *   in free disk space calculation. Users must make sure the quota and
         *   permission don't keep them from writing data into the file system.
         *   Also, the disk space calculation is based on the snapshot this
         *   method takes. After the snapshot is taken, nothing prevents other processes
         *   from writing data into the target file system significantly
         *   reducing the free space significantly,
         *
         *   @param[in] criteria FileSystemsCriteria object
         *   @param[out] match containing matching mount points in descending 
         *                     score order
         *   @return true if no error has been encountered
         */
        bool provideBestFileSystems(const FileSystemsCriteria &criteria,
                     std::vector<MyMntEntWScore> &match);


        /**
         *   Returns true if an error has been encountered. (During such as
         *   Ctor).
         */
        bool getHasErr()  const;


    private:

        GlobalFileSystemsStatus(const GlobalFileSystemsStatus &) {}

        int scoreStorage(const std::string &pth,
                     const GlobalProperties &gps,
                     const FileSystemsCriteria &criteria);

        int checkSpaceReq(const std::string &pth,
                     const GlobalProperties &gps,
                     const nbytes_t space,
                     int *distEst);

        int scoreWSpeedReq(const std::string &pth,
                     const GlobalProperties &gps,
                     const FileSystemsCriteria::SpeedRequirement speed);

        int scoreWDistributionReq(const std::string &pth,
                     const GlobalProperties &gps,
                     const FileSystemsCriteria::DistributionRequirement dist);

        int scoreWScalabilityReq(const std::string &pth,
                     const GlobalProperties &gps,
                     const FileSystemsCriteria::ScalabilityRequirement scale);

        static MountPointsClassifier mMpClassifier;

        bool mHasErr;
    };

  } // StorageInfo namespace

} // FastGlobalFileStatus namespace

#endif // STORAGE_CLASSIFIER_H
