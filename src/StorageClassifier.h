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
 *                         independent module.
 *
 */

#ifndef STORAGE_CLASSIFIER_H
#define STORAGE_CLASSIFIER_H 1

#include "SyncFastGlobalFileStat.h"

namespace FastGlobalFileStat {

  namespace StorageInfo {

    /**
     *   Defines a data type to specify storage selection criteria.
     *   spaceRequirement is the only mandatory requirement to set
     */
    class StorageCriteria {
    public:

        /**
         * SPEED_XXX:        Serial access speed to the storage
         * DISTRIBUTION_XXX: How many remote file servers serve the storage
         * FS_SCAL_XXX: Interent scalability of the storage
         */
        enum Requirement {
            NO_REQUIREMENT     = 0x00000000, /*!< no requirement */
            SPEED_LOW          = 0x00000001, /*!< Require low serial access performance */
            SPEED_HIGH         = 0x00000002, /*!< Require high serial access performance */
            DISTRIBUTED_NONE   = 0x00000003, /*!< Require storage that is not distributed */
            DISTRIBUTED_LOW    = 0x00000004, /*!< Require poorly distributed storage */
            DISTRIBUTED_HIGH   = 0x00000005, /*!< Require well distributed storage */
            DISTRIBUTED_FULL   = 0x00000006, /*!< Require fully distributed storage (node local) */
            FS_SCAL_SINGLE     = 0x00000007, /*!< Require non-parallel storage */
            FS_SCAL_MULTI      = 0x00000008  /*!< Require parallel storage */
        };

        StorageCriteria();

        StorageCriteria(const StorageCriteria &criteria);

        ~StorageCriteria();

        void setSpaceRequirement(int64_t bytesNeeded, int64_t bytesToFree);

        void setDistributionRequirement(int distDegree);

        void setSpeedRequirement(int speedReq);

        void setFsScalabilityRequirement(int scalReq);

        const int64_t getSpaceRequirement() const;

        const int64_t getSpaceToFree() const;

        const int getDistributionRequirement() const;

        const int getSpeedRequirement() const;

        const int getFsScalabilityRequirement() const;


    private:

        int64_t spaceRequirement;

        int64_t spaceToFree;

        int speedRequirement;

        int distributionRequirement;

        int fsScalabilityRequirement;
    };


    /**
     *   Defines a data type that provides best storage based
     *   on the criteria. The provideBestStorage method  and
     *   provideMatchingStorage are the key abstractions. Note
     *   that users of this class should instantiate an object
     *   through a factory method.
     *
     *   Only one type of constructor is allowed to be called;
     *   an object of this type cannot be copied or assigned.
     */
    class StorageClassifier {
    public:

        /**
         *   STORAGE_CLASSIFIER_MIN_SCORE -2.0
         *   Defines the minimum storage classifier score.
         */
        const double STORAGE_CLASSIFIER_MIN_SCORE = -2.0;

        /**
         *   STORAGE_CLASSIFIER_MIN_MATCH_SCORE 0.0
         *   Defines the minimum storage classifier score
         *   for a storage that matches the criteria.
         */
        const double STORAGE_CLASSIFIER_MIN_MATCH_SCORE = 0.0;

        /**
         *   STORAGE_CLASSIFIER_BASE_DEVICE_SPEED 1.0
         *   Defines the base device speed (storage's serial access).
         *   A device's speed would be a multiple (including 1x)
         *   of this value.
         */
        const double STORAGE_CLASSIFIER_BASE_DEVICE_SPEED = 1.0;

        /**
         *   STORAGE_CLASSIFIER_BASE_FS_SCALABILITY 1.0
         *   Defines the base FS scalabilty; storage's inherent
         *   scalability is defined to be a multiple (including 1x)
         *   of this value.
         */
        const double STORAGE_CLASSIFIER_BASE_FS_SCALABILITY = 1.0;

        /**
         *   StorageClassifier Ctor
         *
         *   @param[in] c CommFabric object
         *   @param[in] net opaque network object
         *   @param[out] dedicatedChannel opaque channle object
         *   @return none
         */
        StorageClassifier(CommFabric *c,
                          void *net=NULL,
                          void *dedicatedChannel=NULL);

        ~StorageClassifier();

        /**
         *   Examines all of the storage mounted across all of the
         *   processes and computes the best stroage based on the
         *   criteria specified through criteria. Only spaceRequirement
         *   should be set in critiera.
         *
         *   @param[in] criteria StorageCriteria object
         *   @param[out] matchMnt a best mount point
         *   @return MyMntEnt object that describes the best match
         *           storage's mount point info.
         */
        bool provideBestStorage(const StorageCriteria &criteria,
                                MyMntEnt &matchMnt);

        /**
         *   Examines all of the storage mounted across all of the
         *   processes and find matching stroage based on the
         *   criteria specified through criteria. Any of the optional
         *   requirement can be given with this method.
         *
         *   @param[in] criteria StorageCriteria object
         *   @param[out] match containing matching mount points
         *   @return true if no error has been encountered
         */
         bool provideMatchingStorage(const StorageCriteria &criteria,
                                     std::vector<MyMntEnt> &match);

        /**
         *   Returns true if an error has been encountered. (During such as
         *   Ctor).
         */
        bool getHasErr()  const;

    private:

        StorageClassifier() {}

        StorageClassifier(const StorageClassifier &) {}

        bool initCommFabric(CommFabric *c, void *net=NULL,
                          void *dedicatedChannel=NULL);

        bool getGloballyAvailMntPoints(std::vector<std::string> &mntPoints) const;

        bool hasErr;

        FgfsStatDesc parallelInfo;

        CommFabric *commFabric;
    };

  } // StorageInfo namespace

} // FastGlobalFileStat namespace

#endif // STORAGE_CLASSIFIER_H