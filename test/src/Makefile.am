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



INCLUDES                       = -I$(top_srcdir) \
                                 -I$(top_srcdir)/src \
                                 -I@MPALOC@/include

testdir                        = ${pkgdatadir}/tests
test_PROGRAMS                  = sync_stat_dso_mpi \
                                 async_stat_dso_mpi \
                                 async_stat_dso_mrnet \
                                 gen_hostlist \
                                 mrnet_node_req

test_SCRIPTS                   = mrnet_topo_gen.sh 

CLEANFILES                     = $(test_SCRIPTS)
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
sync_stat_dso_mpi_LDFLAGS      = -L@MPALOC@/lib -L$(top_srcdir)/src $(MPI_CLDFLAGS)
sync_stat_dso_mpi_LDADD        = -lelf -lssl @LIBMPA@ -lfgfs_mpi


#
#  ASYNC_STAT_DSO_MPI rules
#
async_stat_dso_mpi_SOURCES     = async_stat_dso_mpi.C \
                                 FgfsTestGetDsoList.C
async_stat_dso_mpi_CXXFLAGS    = $(MPI_CFLAGS) $(AM_CXXFLAGS) 
async_stat_dso_mpi_LDFLAGS     = -L@MPALOC@/lib -L$(top_srcdir)/src $(MPI_CLDFLAGS)
async_stat_dso_mpi_LDADD       = -lelf -lssl @LIBMPA@ -lfgfs_mpi


#
#  GEN-HOSTLIST rules
#
gen_hostlist_SOURCES           = gen_hostlist.C
gen_hostlist_CXXFLAGS          = $(MPI_CFLAGS) $(AM_CXXFLAGS) 
gen_hostlist_LDFLAGS           = -L@MPALOC@/lib -L$(top_srcdir)/src $(MPI_CLDFLAGS)


#
#  mrnet_node_req rules
#
mrnet_node_req_SOURCES         = mrnet_node_req.C
mrnet_node_req_CXXFLAGS        = $(MPI_CFLAGS) $(AM_CXXFLAGS) 
mrnet_node_req_LDFLAGS         = -L@MPALOC@/lib -L$(top_srcdir)/src $(MPI_CLDFLAGS)


#
#  ASYNC_STAT_DSO_MPI rules
#
async_stat_dso_mrnet_SOURCES   = async_stat_dso_mrnet.C \
                                 FgfsTestGetDsoList.C \
				 FgfsMRNetWrapper.C
async_stat_dso_mrnet_CXXFLAGS  = $(MRNET_CXXFLAGS) $(AM_CXXFLAGS) 
async_stat_dso_mrnet_LDFLAGS   = -Wl,-E -L@MPALOC@/lib -L$(top_srcdir)/src $(MRNET_LDFLAGS)
async_stat_dso_mrnet_LDADD     = -lelf -lssl @LIBMPA@ -lfgfs_mrnet


do_subst = sed -e 's,@MRNETTOPGEN@,@MRNETTOPGENBIN@,g'


mrnet_topo_gen.sh: mrnet_topo_gen.sh.in Makefile
	$(do_subst) < mrnet_topo_gen.sh.in > mrnet_topo_gen.sh
	chmod +x mrnet_topo_gen.sh

