## $Header: $
##
## Makefile.am -- Process this file with automake to produce Makefile.in 
##
##--------------------------------------------------------------------------------
## Copyright (c) 2011, Lawrence Livermore National Security, LLC. Produced at
## the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>.
## LLNL-CODE-xxxxxx. All rights reserved.
##--------------------------------------------------------------------------------
##
##  Update Log:
##        Jul 01 2011 DHA: File created.
##



AM_CPPFLAGS                    = -I$(top_srcdir) \
                                 -I$(top_srcdir)/src \
                                 -I@MPALOC@/include

nobase_include_HEADERS         = FgfsTestGetDsoList.h \
                                 g_fs_stat_c_wrap.h \
                                 FgfsMRNetWrapper.h

testdir                        = ${pkgdatadir}/tests
test_PROGRAMS                  = sync_stat_dso_mpi \
                                 async_stat_dso_mpi \
                                 sync_stat_apath_mpi \
                                 st_classifier_constmem_per_proc \
                                 st_classifier_big_on_oneproc \
                                 st_classifier_c_lang \
                                 st_mountpoint_classifier \
                                 async_stat_dso_mrnet \
                                 gen_hostlist \
                                 mrnet_node_req \
                                 my_topo_gen

test_SCRIPTS                   = mrnet_topo_gen.sh \
                                 run.fgfs_tester.sh.in \
                                 run.fgfs_tester_mpi.sh.in \
                                 MRNetScalingSetup.sh \
                                 MPIScalingSetup.sh

CLEANFILES                     = mrnet_topo_gen.sh
EXTRA_DIST                     = mrnet_topo_gen.sh.in \
                                 run.fgfs_tester.sh.in \
                                 run.fgfs_tester_mpi.sh.in \
                                 MRNetScalingSetup.sh \
                                 MPIScalingSetup.sh


#
#  SYNC_STAT_DSO_MPI rules
#
sync_stat_dso_mpi_SOURCES      = sync_stat_dso_mpi.C \
                                 FgfsTestGetDsoList.C
sync_stat_dso_mpi_CXXFLAGS     = $(AM_CXXFLAGS) $(MPI_CFLAGS)
sync_stat_dso_mpi_LDFLAGS      = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
sync_stat_dso_mpi_LDADD        = -lelf -lssl -lcrypto @LIBMPA@ -lfgfs_mpi


#
#  ASYNC_STAT_DSO_MPI rules
#
async_stat_dso_mpi_SOURCES     = async_stat_dso_mpi.C \
                                 FgfsTestGetDsoList.C
async_stat_dso_mpi_CXXFLAGS    = $(MPI_CFLAGS) $(AM_CXXFLAGS)
async_stat_dso_mpi_LDFLAGS     = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
async_stat_dso_mpi_LDADD       = -lelf -lssl -lcrypto @LIBMPA@ -lfgfs_mpi


#
#  SYNC_STAT_APATH_MPI rules
#
sync_stat_apath_mpi_SOURCES      = sync_stat_apath_mpi.C \
                                   FgfsTestGetDsoList.C
sync_stat_apath_mpi_CXXFLAGS     = $(AM_CXXFLAGS) $(MPI_CFLAGS)
sync_stat_apath_mpi_LDFLAGS      = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
sync_stat_apath_mpi_LDADD        = -lelf -lssl -lcrypto @LIBMPA@ -lfgfs_mpi


#
#  ST_CLASSIFIER_CONSTMEM_PER_PROC  rules
#
st_classifier_constmem_per_proc_SOURCES     = st_classifier_constmem_per_proc.C
st_classifier_constmem_per_proc_CXXFLAGS    = $(MPI_CFLAGS) $(AM_CXXFLAGS)
st_classifier_constmem_per_proc_LDFLAGS     = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
st_classifier_constmem_per_proc_LDADD       = @LIBMPA@ -lfgfs_mpi


#
#  ST_CLASSIFIER_C_LANG rules
#
st_classifier_c_lang_SOURCES   = st_classifier_c_lang.C \
                                 g_fs_stat_c_wrap.C
st_classifier_c_lang_CXXFLAGS  = $(MPI_CFLAGS) $(AM_CXXFLAGS)
st_classifier_c_lang_LDFLAGS   = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
st_classifier_c_lang_LDADD     = @LIBMPA@ -lfgfs_mpi


#
#  ST_CLASSIFIER_BIG_ON_ONEPROC rules
#
st_classifier_big_on_oneproc_SOURCES  = st_classifier_big_on_oneproc.C
st_classifier_big_on_oneproc_CXXFLAGS = $(MPI_CFLAGS) $(AM_CXXFLAGS)
st_classifier_big_on_oneproc_LDFLAGS  = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
st_classifier_big_on_oneproc_LDADD    = @LIBMPA@ -lfgfs_mpi


#
#  ST_MOUNTPOINT_CLASSIFIER rules
#
st_mountpoint_classifier_SOURCES  = st_mountpoint_classifier.C
st_mountpoint_classifier_CXXFLAGS = $(MPI_CFLAGS) $(AM_CXXFLAGS)
st_mountpoint_classifier_LDFLAGS  = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)
st_mountpoint_classifier_LDADD    = @LIBMPA@ -lfgfs_mpi


#
#  GEN-HOSTLIST rules
#
gen_hostlist_SOURCES           = gen_hostlist.C
gen_hostlist_CXXFLAGS          = $(MPI_CFLAGS) $(AM_CXXFLAGS) 
gen_hostlist_LDFLAGS           = -L@MPALOC@/lib -L../../src $(MPI_CXXLDFLAGS)


#
#  mrnet_node_req rules
#
mrnet_node_req_SOURCES         = mrnet_node_req.C
mrnet_node_req_CXXFLAGS        = $(AM_CXXFLAGS) 
mrnet_node_req_LDFLAGS         = -L$../../src 


#
#  myTopoGen rules
#
my_topo_gen_SOURCES            = my_topo_gen.C
my_topo_gen_CXXFLAGS           = $(AM_CXXFLAGS) 
my_topo_gen_LDFLAGS            = -L../../src 


#
#  ASYNC_STAT_DSO_MPI rules
#
async_stat_dso_mrnet_SOURCES   = async_stat_dso_mrnet.C \
                                 FgfsTestGetDsoList.C \
                                 FgfsMRNetWrapper.C
async_stat_dso_mrnet_CXXFLAGS  = $(MRNET_CXXFLAGS) $(AM_CXXFLAGS) 
async_stat_dso_mrnet_LDFLAGS   = -Wl,-E -L@MPALOC@/lib -L../../src $(MRNET_LDFLAGS) 
async_stat_dso_mrnet_LDADD     = -lelf -lssl -lcrypto @LIBMPA@ -lfgfs_mrnet 


do_subst = sed -e 's,@MRNETTOPGEN@,@MRNETTOPGENBIN@,g'


mrnet_topo_gen.sh: mrnet_topo_gen.sh.in Makefile
	$(do_subst) < $(top_srcdir)/test/src/mrnet_topo_gen.sh.in > mrnet_topo_gen.sh
	chmod +x mrnet_topo_gen.sh

