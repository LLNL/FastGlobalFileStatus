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

#ifndef SYNC_FAST_GLOBAL_FILE_STAT_H
#define SYNC_FAST_GLOBAL_FILE_STAT_H 1

#include "FastGlobalFileStat.h"

namespace FastGlobalFileStatus {

    /**
     *   Defines a data type that a tool should derive from
     *   and implement. The hash method should be capable of
     *   generating a unique file signiture in a similar fashion
     *   as md5sum of the file. An example implementation is
     *   provided in a seperate source file: OpenSSLFileSigGen.[h|C]
     */
    class FileSignitureGen {
    public:

        virtual ~FileSignitureGen() { }


        /**
         *   Computes a signiture of the file and resurn a signiture buffer
         *   and its size. A derived class must implement this method.
         *
         *   @param[in] fd file descriptor of an open file
         *   @param[in] fileSize the size of the input file
         *   @param[out] sigSize the size of the returning signiture buffer
         *   @return a pointer to the signiture buffer; an overriden signiture
         *           method should use malloc to allocate the buffer; and the
         *           the caller is responsible for freeing the buffer.
         */
        virtual unsigned char *
                signiture(int fd, int fileSize, int *sigSize) = 0;
    };


    /**
     *   Provides synchronous abstractions of Fast Global File Stat.
     */
    class SyncGlobalFileStatus : public GlobalFileStatusAPI, public GlobalFileStatusBase {
    public:

        /**
         *   SyncGlobalFileStatus Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @return none
         */
        SyncGlobalFileStatus(const char *pth);

        /**
         *   SyncGlobalFileStatus Ctor
         *
         *   @param[in] pth an absolute path with no links to a file
         *   @param[in] threshold a process count to staturate the
         *                         file server; when you want to overwrite
         *                         the default threshold.
         *   @return none
         */
        SyncGlobalFileStatus(const char *pth, const int threshold);

        virtual ~SyncGlobalFileStatus();

        /**
         *   Class Static Initializer. (Call once for the entire class hierarchy)
         *   This is a global collective: all distributed component must
         *   call synchronously.
         *
         *   @param[in] fsg pointer to a FileSignitureGen object
         *   @param[in] c pointer to a CommFabric object
         *   @return a bool value
         */
        static bool initialize(FileSignitureGen *fsg,
                               CommLayer::CommFabric *c);

        /**
         *   This is a global collective: all distributed component must 
         *   call synchronously. It performs a bunch of traging operations:
         *   it computes a cardinality estimate that approximate how many
         *   distinct groups exists; by doing that, it helps FGFS to avoid 
         *   unscalable reduction when lots of groups exist. Overally,
         *   the triage operation satisfies many global properties of a file
         *   with a cheap and scalable operation. This method MUST be called
         *   before using other abstractions of synchronous FGFS.
         *
         *   @param[in] algo CommAlorithms (default: bloom filter based)
         *   @return a bool value
         */
        bool triage(CommAlgorithms algo=bloomfilter);

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

        /**
         *   Is the file consistent across nodes?
         *   Yes for isUnique implies yes for isConsistent().
         *   But the converse does not hold. This method would incur
         *   the same overhead as isUnique when the path is served
         *   by a single server and a higher overheads when the path
         *   is served by a distributed file servers.
         *
         *   @param[in] serial the method performs file signiture
         *                      computation serially if true. Only set
         *                      this flag for debugging
         *   @return an FGFSInfoAnswer object
         */
        FGFSInfoAnswer isConsistent(bool serial=false);

        /**
         *   Computes a signiture of the file using the registered
         *   FileSignitureGen object. This function uses a scalable approach
         *   to generate the signiture buffer.
         *
         *   @param[out] buf a pointer to the allocated struct stat. This method
         *                    fills this info using a scalable algorithm.
         *   @param[out] sigSize the size of the returning signiture buffer
         *   @return a pointer to the signiture buffer; the
         *           the caller is responsible for freeing the buffer.
         */
        unsigned char * signiture(struct stat *buf, int *sigSize);

        /**
         *   Computes a signiture of the file using the registered
         *   FileSignitureGen object. This function uses a non-scalable approach
         *   to generate the signiture buffer. This method should only be used for
         *   debugging and testing.
         *
         *   @param[out] buf a pointer to the allocated struct stat. This method
         *                    fills this info using a scalable algorithm.
         *   @param[out] sigSize the size of the returning signiture buffer
         *   @return a pointer to the signiture buffer; the
         *           the caller is responsible for freeing the buffer.
         */
        unsigned char * signitureSerial(struct stat *buf, int *sigSize);

        bool forceComputeParallelInfo( );


    private:

        /**
         *   FileSignitureGen
         */
        static FileSignitureGen *fileSignitureGen;
    };

}

#endif // SYNC_FAST_GLOBAL_FILE_STAT_H

