/*
 * --------------------------------------------------------------------------------
 * Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
 * All rights reserved.
 *
 * Update Log:
 *
 *        Jul 01 2011 DHA: remove "using namespace" from this header
 *        Feb 07 2011 DHA: File created.
 *
 */

extern "C" {
#include <sys/mman.h>
#include <openssl/md5.h>
#include <stdlib.h>
}

#include "SyncFastGlobalFileStat.h"

class OpenSSLFileSignitureGen : public FastGlobalFileStatus::FileSignitureGen {
public:
    virtual ~OpenSSLFileSignitureGen() {}
    virtual unsigned char *
       signiture(int fd, int fileSize, int *sigSize) {

           *sigSize = MD5_DIGEST_LENGTH;
           unsigned char *fileBuf = NULL;

           unsigned char *resultBuf
              = (unsigned char *) malloc(MD5_DIGEST_LENGTH);

           if (!resultBuf || fd < 0)
               return NULL;

           fileBuf = (unsigned char *) mmap(0, fileSize,
                                            PROT_READ,
                                            MAP_SHARED,
                                            fd, 0);
           MD5(fileBuf, fileSize, resultBuf);
           munmap(fileBuf, fileSize);

           return resultBuf;
       }
};
