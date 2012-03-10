/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Aug 26 2011 DHA: File created
 *
 */

#ifndef MOUNT_POINTS_CLASSIFIER_H
#define MOUNT_POINTS_CLASSIFIER_H 1

#include "SyncFastGlobalFileStat.h"

namespace FastGlobalFileStatus {


    /**
     *   Allows abstractions like MountPointsClassifier to annote
     *   each and all of mount points
     */
    class GlobalProperties {
    public:
        GlobalProperties();
        GlobalProperties(const GlobalProperties &lhs);
        ~GlobalProperties();

        const FGFSInfoAnswer getUnique() const;
        const FGFSInfoAnswer getPoorlyDist() const;
        const FGFSInfoAnswer getWellDist() const;
        const FGFSInfoAnswer getFullyDist() const;
        const FGFSInfoAnswer getConsistent() const;
        const FGFSInfoAnswer isUnique() const;
        const FGFSInfoAnswer isPoorlyDistributed() const;
        const FGFSInfoAnswer isWellDistributed() const;
        const FGFSInfoAnswer isFullyDistributed() const;
        const MountPointAttribute::FileSystemType getFsType() const;
        const int getFsSpeed() const;
        const int getFsScalability() const;
        const int getDistributionDegree() const;
        const std::string & getFsName() const;
        CommLayer::FgfsParDesc & getParDesc();
        const CommLayer::FgfsParDesc & getParallelDescriptor() const;

        void setUnique(FGFSInfoAnswer v);
        void setPoorlyDist(FGFSInfoAnswer v);
        void setWellDist(FGFSInfoAnswer v);
        void setFullyDist(FGFSInfoAnswer v);
        void setConsistent(FGFSInfoAnswer v);
        void setFsType(MountPointAttribute::FileSystemType t);
        void setFsName(const char *str);
        void setFsSpeed(int speed);
        void setFsScalability(int scal);
        void setDistributionDegree(int dist);
        void setParDesc(const CommLayer::FgfsParDesc pd);

    private:
        FGFSInfoAnswer mUnique;
        FGFSInfoAnswer mPoorlyDist;
        FGFSInfoAnswer mWellDist;
        FGFSInfoAnswer mFullyDist;
        FGFSInfoAnswer mConsistent;
        MountPointAttribute::FileSystemType mFsType;
        int mFsSpeed;
        int mFsScalability;
        int mDistributionDegree;
        std::string mFsName;
        CommLayer::FgfsParDesc mParDesc;
    };


    /**
     *   Data type that classifies all of the globally available 
     *   mount points and store in its datebase mAnnoteMountPoints
     */
    class MountPointsClassifier : public GlobalFileStatusBase {
    public:
        MountPointsClassifier();

        ~MountPointsClassifier();

        static bool runClassification(CommLayer::CommFabric *c);

        static const std::map<std::string, GlobalProperties> & 
                     getGlobalMountpointsMap();

        static std::map<std::string, GlobalProperties> mAnnoteMountPoints;
    };
}

#endif // MOUNT_POINTS_CLASSIFIER_H
