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
     @mainpage Fast Global File Status (FGFS)
     @author Dong H. Ahn <ahn1@llnl.gov>, Development Environment Group, Livermore Computing (LC) Division, LLNL

     @section intro Introduction

     Large-scale systems typically mount many different file systems with
     distinct performance characteristics and capacity. High performance computing 
     applications must efficiently use this storage in order to realize their full performance
     potential. Users must take into account potential file replication throughout
     the storage hierarchy as well as contention in lower levels of the 
     I/O system, and must consider communicating the results of file I/O 
     between application processes to reduce file system accesses.
     Addressing these issues and optimizing file accesses requires detailed 
     run-time knowledge of file system performance characteristics and 
     the location(s) of files on them.
     
     We developed Fast Global File Status (FGFS) to provide a scalable 
     mechanism to retrieve such information of a file, including its degree of 
     distribution or replication and consistency. FGFS uses a novel node-local 
     technique that turns expensive, non-scalable file system calls into 
     simple string comparison operations. FGFS raises the namespace of a 
     locally-defined file path to a global namespace with little or no file 
     system calls to obtain global file properties efficiently. Our evaluation 
     on a large multi-physics application showed that most FGFS file status queries on 
     its executable and 848 shared library files complete in 272 milliseconds
     or faster at 32,768 MPI processes. Even the most expensive operation, 
     which checks global file consistency, completes in under 7 seconds at this 
     scale, an improvement of several orders of magnitude 
     over the traditional checksum technique.

     @section fnresol File Name Resolution Engine 
     The main abstractions that enable raising the namespace of local file names
     are packaged up into the \c MountPointAttributes module.
     The core technique of \c MountPointAttributes is a scalable mechanism to 
     raise the local namespace of a file to a global namespace. 
     The global namespace enables fast comparisons of local 
     file properties across distributed machines with little or
     no access requirement on the underlying file systems. 
     More specifically, the file name resolution engine of \c MountPointAttributes 
     turns a local file path into a Uniform Resource Identifier (URI), 
     a globally unique identifier of the file. This resolution 
     process is merely a memory operation, as our technique 
     builds an URI through the file system mount point table, 
     which is available in system memory. Thus,
     this core logic requires no communication and can scale well.

     The \c MountPointInfo class is the main data type 
     associated with the file name resolution process. For example,
     the following code snippet stores \c filepath's URI into 
     \c uriString like "nfs://dip-nfs.llnl.gov:/vol/g0/joe/readme"
     through a FileUriInfo object.

     @verbatim
         #include "MountPointAttr.h"
         using namespace FastGlobalFileStatus;
         ...

         MountPointInfo mpInfo(true);
         if (!IS_YES(mpInfo.isParsed())) 
            return;
            
         FileUriInfo uriInfo;
         std::string uriString;
         const char *filepath = "/g/g0/joe/readme";
         mpInfo.getFileUriInfo(filepath, uriInfo);

         // getUri resolves filepath into a URI string like
         // nfs://dip-nfs.llnl.gov:/vol/g0/joe/readme
         uriInfo.getUri(uriString);

         ...
     @endverbatim

     In addition, \c MountPointInfo allows developers to retrieve mount-points
     information directly through a \c MyMntEnt object and offers 
     higher-level abstractions to query relevant properties: 
     e.g., is the source of a file remote or local.

     @verbatim
         #include "MountPointAttr.h"
         using namespace FastGlobalFileStatus;
         ...

         MyMntEnt anEntry;
        
         //
         // Returns the mount point info on filepath
         //
         mpInfo.getMntPntInfo(filepath, anEntry)   

         //
         // Determines whether the filepath is remotely served or not
         // 
         if (IS_YES(mpInfo.isRemoteFileSystem(filepath, anEntry)) {
             // filepath is remotely served
         }
     @endverbatim

     @section filestatus Global File Status Queries
     The global namespace provided by the \c MountPointAttributes module
     forms a reference space where local 
     parallel name comparisons can compute common global
     properties. The global information must capture
     properties like the number of different sources
     that serve the file to all participating
     processes as well as the process count and the representative
     process of each source.
     FGFS provides low-level primitives on these
     properties. For example, the \c FgfsParDesc parallel
     descriptor returns various grouping information
     such as the number of unique groups and the size
     and the representative process of the group to which
     the caller belongs.

     FGFS also composes these primitives in a way
     to capture the main issues that emerge in HPC
     and exposes this information through a high-level query API.
     Specifically, the API class, \c GlobalFileStatusAPI,
     defines five virtual query methods: 

     - \c isFullyDistributed()
     - \c isWellDistributed()
     - \c isPoorlyDistributed()
     - \c isUnique()
     - \c isConsistent()

     Taking the local path of a file as the input,
     the \c isWellDistributed and \c isPoorlyDistributed
     queries test whether the file is served by a number of remote
     file servers that is higher or lower than a configurable threshold, respectively.
     Further, the \c isFullyDistributed query determines
     whether the file is served locally, a special case of being
     well-distributed, while the \c isUnique
     query tests whether it is served by a single remote server,
     a special case of being poorly-distributed.
     Finally, \c isConsistent, which is implied by \c isUnique evaluates
     whether the file's content is consistent across all application processes.

     @section fsstatus Global File Systems Status Queries
     While file status queries are geared towards
     the needs of read operations, its inverse function
     can generally benefit write operations:
     FGFS' file systems status queries.
     A file systems status query takes as an input
     a set of required global properties of a file
     system such as
     its available aggregate space, distribution,
     performance and scalability.
     The query then searches through all of the file systems mounted
     on the machine and selects the best matching locations.

     \c GlobalFileSystemsStatus is the main data type
     that captures this concept. A distributed program
     passes to an object of this type the required
     global properties of a file system in the form
     of a \c FileSystemCriteria object.
     The status object then iterates over all mounted
     file systems and collects the global
     information of each mount point as a file path
     in order to test and to score how well
     the global properties meet the specified criteria.


     The only criterion that
     the program must specify is the space requirement.
     As the status object iterates through the mounted file systems,
     it calculates the aggregate
     number of bytes needed by all equivalent processes and tests
     whether the target file system has enough free space.
     In particular, when a mount point resides across multiple
     remote file servers for the target program, distinct groups
     of processes write into their own associated file servers.
     Besides the space requirement, \c FileSystemCriteria provides
     the following options to allow a program to refine the requirement of a file system further:
     - \c SpeedRequirement: the sequential processing performance of a file system with the choices of \c LOW and \c HIGH;
     - \c DistributionRequirement: the distribution of a file system with the choices of \c UNIQUE, \c LOW, \c HIGH, and \c FULL;
     - \c ScalabilityRequirement: the scalability of a file system with the choices of \c SINGLE---i.e., a plain NFS, and \c MULTI---i.e., a parallel file system.

     If multiple file systems match the selected criteria, FGFS orders them using a scoring function.

     @section interoperability Interoperation with HPC Communication Fabrics
     FGFS is designed as a general purpose layer.
     Thus, it is critical for FGFS to interoperate well with a wide range of HPC
     communication fabrics. For this purpose, FGFS builds on a virtual network
     abstraction that defines a rudimentary collective
     communication interface containing only the basic operations: a simple broadcast, multicast, and reductions.
     This layer can be implemented on top of many native
     communication layers through plug-ins
     that translates the native communication protocols to this rudimentary
     collective calls.

     Thus far, we have developed and tested virtual network plug-ins
     for MPI, MRNet (http://www.paradyn.org/mrnet) and LaunchMON (http://sourceforge.net/projects/launchmon), but this technique
     is equally applicable to other overlay networks and bootstrappers such as
     PMGR Collective, COBO, and LIBI.
     The virtual network abstraction allows an HPC
     program written to a specific communication fabric to instantiate
     FGFS on that fabric. 

     The following example shows the initialization of FGFS with MPI. All MPI processes 
     must collectively instantiate CommFabric objects with the \c MPICommFabric::initialize
     method. And pass these objects to the \c initalize() method of a 
     specific query type (in this case \c AsyncGlobalFileStatus query.)

     @verbatim
     #include "mpi.h"
     #include "Comm/MPICommFabric.h"
     #include "AsyncFastGlobalFileStat.h"
     
     ...

     bool rc;
     if (!MPICommFabric::initialize(&argc, &argv)) {
        MPA_sayMessage("TEST",
	               true,
                       "MPICommFabric::initialize returned false");
        exit(1);
     }
     CommFabric *cfab = new MPICommFabric();

     rc = AsyncGlobalFileStatus::initialize(cfab);

     AsyncGlobalFileStatus myQueryObj("/abs_path/to/test")
  
     if (IS_YES(myQueryObj.isUnique())) {
       // /abs_path/to/test is consistent by definition
       ...
     }
     
     @endverbatim

    */


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
         *   @param[in] d new  CardinalityEst of integer type
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
         *   @param[in] sg new threshold
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
         *   @param[in] th new high low point of integer type
         *   @return the old high low point of integer type
         */
        int setHiLoCutoff(const int th);

        /**
         *   Performs necessary communication to determine global
         *   properties including a number of different equivalent
         *   groups
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool computeParallelInfo(GlobalFileStatusAPI *gfsObj,
                                 CommAlgorithms algo=bloomfilter);

        /**
         *   Performs cardinality estimate: how many different equivalent 
         *   groups are there? The client can get this estimate with
         *   the getCardinalityEst method
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
         *   @param[in] algo an algorithm type of CommAlgorithms
         *   @return success or failure of bool type
         */
        bool computeCardinalityEst(GlobalFileStatusAPI *gfsObj,
                                   CommAlgorithms algo=bloomfilter);

        /**
         *   Performs cardinality estimate based on the bloomfilter
         *   algorithm.
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool bloomfilterCardinalityEst(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality estimate based on the sampling
         *   algorithm: No/
         *   tImplemented
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool samplingCardinalityEst(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality estimate based on the generalized
         *   comm-split: NotImplemented
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
         *   @return success or failure of bool type
         */
        bool hier_commsplitCardinality(GlobalFileStatusAPI *gfsObj);

        /**
         *   Performs cardinality check (accurate) based on the
         *   actual list reduction: NotImplemented
         *   @param[in,out] gfsObj a GlobalFileStatAPI object
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

