## $Header: $
##
## Makefile.am -- Process this file with automake to produce Makefile.in 
##
##
##--------------------------------------------------------------------------------
## Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
## the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
## LLNL-CODE-xxxxxx. All rights reserved.
##--------------------------------------------------------------------------------
##
##  Update Log:
##        Jul 08 2011 DHA: Added mrnet-based library build rules
##        Jun 30 2011 DHA: Added Todd's MPI m4 support
##        Jun 29 2011 DHA: File created.
##
##

lib_LTLIBRARIES           = libfgfs_mpi.la libfgfs_mrnet.la libfgfs_filter.la

AM_CPPFLAGS               = -I$(top_srcdir) -I@MPALOC@/include

nobase_include_HEADERS    = Comm/DistDesc.h \
                            Comm/CommFabric.h \
                            Comm/MPICommFabric.h \
                            Comm/MPIReduction.h \
                            Comm/MRNetCommFabric.h \
                            bloom.h \
                            OpenSSLFileSigGen.h

include_HEADERS           = FastGlobalFileStat.h \
                            SyncFastGlobalFileStat.h \
                            MountPointsClassifier.h \
                            AsyncFastGlobalFileStat.h \
                            StorageClassifier.h

libfgfs_mpi_la_SOURCES    = bloom.c \
                            Comm/CommFabric.C \
                            Comm/DistDesc.C \
                            Comm/MPICommFabric.C \
                            FastGlobalFileStat.C \
                            SyncFastGlobalFileStat.C \
                            MountPointsClassifier.C \
                            AsyncFastGlobalFileStat.C \
                            StorageClassifier.C

libfgfs_mpi_la_CFLAGS     = $(AM_CFLAGS)
libfgfs_mpi_la_CXXFLAGS   = $(MPI_CFLAGS) $(AM_CXXFLAGS)
libfgfs_mpi_la_LDFLAGS    = $(AM_LDFLAGS) -L@MPALOC@/lib $(LIBMPA) $(MPI_CXXLDFLAGS) \
                            -version-info @FGFS_CURRENT@:@FGFS_REVISION@:@FGFS_AGE@

#
# StorageClassification is yet unavailable
#
libfgfs_mrnet_la_SOURCES  = bloom.c \
                            Comm/CommFabric.C \
                            Comm/DistDesc.C \
                            Comm/MRNetCommFabric.C \
                            FastGlobalFileStat.C \
                            SyncFastGlobalFileStat.C \
                            MountPointsClassifier.C \
                            AsyncFastGlobalFileStat.C

libfgfs_mrnet_la_CFLAGS   = $(AM_CFLAGS)
libfgfs_mrnet_la_CXXFLAGS = $(MRNET_CXXFLAGS) $(AM_CXXFLAGS) -fpic
libfgfs_mrnet_la_LDFLAGS  = $(AM_LDFLAGS) -L@MPALOC@/lib $(LIBMPA) $(MRNET_LDFLAGS) \
                            -version-info @FGFS_CURRENT@:@FGFS_REVISION@:@FGFS_AGE@

libfgfs_filter_la_SOURCES = Comm/MRNetFilterUp.C \
                            Comm/MRNetFilterDown.C \
                            Comm/DistDesc.C
libfgfs_filter_la_CFLAGS  = $(AM_CFLAGS)
libfgfs_filter_la_CXXFLAGS= $(MRNET_CXXFLAGS) $(AM_CXXFLAGS)
libfgfs_filter_la_LDFLAGS = $(AM_LDFLAGS) -L@MPALOC@/lib $(LIBMPA) \
                            -version-info @FGFS_CURRENT@:@FGFS_REVISION@:@FGFS_AGE@


