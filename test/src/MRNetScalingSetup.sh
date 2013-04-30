#!/bin/sh
##
## MRNetScalingSetup.sh -- this is a script that sets up FGFS scaling tests for MRNet.
##    This creates experiment directories each of which contains a batch script
##    to run at various scales. The tester is async_stat_dso_mpi (AsyncGlobalFileStatus),
##    which runs FGFS' global file status queries:
##       - isFullyDistributed (0)
##       - isWellDistributed (1)
##       - isPoorlyDistributed (2)
##       - isUnique (3)
##
## To use MRNet, you might have to set the following environment variables to your
## shell start-up script (e.g., ~/.cshrc)
## setenv LD_LIBRARY_PATH <MRNet Install Root>/lib:<FGFS Install Root>/lib
## setenv MRNET_COMM_PATH <MRNet Install Root>/bin/mrnet_commnode
## In case, rsh is the remote access protocol you must use to set up an
## MRNet overlay network. 
## setenv XPLAT_RSH rsh
##
##
## -------------------------------------------------------------------------------- 
## Copyright (c) 2011 - 2013, Lawrence Livermore National Security, LLC. Produced at
## the Lawrence Livermore National Laboratory. Written by Dong H. Ahn <ahn1@llnl.gov>. 
## LLNL-CODE-490173. All rights reserved.
## 
## This file is part of MountPointAttributes. 
## For details, see https://computing.llnl.gov/?set=resources&page=os_projects
## 
## Please also read LICENSE - Our Notice and GNU Lesser General Public License.
## 
## This program is free software; you can redistribute it and/or modify it under 
## the terms of the GNU General Public License (as published by the Free Software
## Foundation) version 2.1 dated February 1999.
## 
## This program is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU
## General Public License for more details.
## 
## You should have received a copy of the GNU Lesser General Public License along
## with this program; if not, write to the Free Software Foundation, Inc., 59 Temple
## Place, Suite 330, Boston, MA 02111-1307 USA
## -------------------------------------------------------------------------------- 
##
##  Update Log:
##        Apr 30 2013 DHA: File created
##


ncore_per_node=1
mach_name=""

if [ $# -ne 3 ]
then
    echo ""
    echo "Usage: MRNetScalingSetup.sh <number of cores per testing node> <testing machine name> <target application executable path>"
    echo ""
    echo "  This is a script that sets up FGFS scaling tests for MPI."
    echo "  This creates experimental directories each of which contains a batch script (MOAB/Slurm)"
    echo "  to run test programs at various scales. The tester is async_stat_dso_mpi "
    echo "  (AsyncGlobalFileStatus) which runs FGFS' global file status queries:"
    echo "     - isFullyDistributed (0)"
    echo "     - isWellDistributed (1)"
    echo "     - isPoorlyDistributed (2)"
    echo "     - isUnique (3)"
    exit 1
fi

ncore_per_node=$1
mach_name=$2
apppath=$3
app_names="TestApp"
testers="async_stat_dso_mrnet"
fgfs_ops="0 1 2 3"
be_count="256 512 1000 4096 10000 14641 20736 2851"
dir_ptr="FGFS.Scaling.MRNet"
branchPerCore=3
rootpath=`pwd`


mkdir -p $dir_ptr
dir_ptr="$dir_ptr/MRNet"
mkdir $dir_ptr

for etype in $testers ; do
      mkdir -p $dir_ptr/$etype

      for app in $app_names ; do
            mkdir -p $dir_ptr/$etype/$app

            for scale in $be_count ; do
              mkdir -p $dir_ptr/$etype/$app/$scale

              for tOp in $fgfs_ops; do 
                 testpath=$rootpath/$dir_ptr/$etype/$app/$scale/$tOp
                 mkdir $testpath
                 ln -s ../../../../../../$etype $testpath/
                 height=`mrnet_node_req 1 $scale $ncore_per_node $branchPerCore`
                 fanout=`mrnet_node_req 2 $scale $ncore_per_node $branchPerCore`
                 totalBENum=`mrnet_node_req 3 $scale $ncore_per_node $branchPerCore`
                 totalCPNum=`mrnet_node_req 4 $scale $ncore_per_node $branchPerCore`
                 totalBENodeNum=`mrnet_node_req 5 $scale $ncore_per_node $branchPerCore`
                 coreToUsePerBENode=`mrnet_node_req 6 $scale $ncore_per_node $branchPerCore`
                 totalCPOnlyNodeNum=`mrnet_node_req 7 $scale $ncore_per_node $branchPerCore`
                 totalNodeNum=`mrnet_node_req 8 $scale $ncore_per_node $branchPerCore`
                 corePerCp=`mrnet_node_req 9 $scale $ncore_per_node $branchPerCore`

                 sed -e "s,@NNODES@,$totalNodeNum,g" \
	             -e "s,@MACH@,$mach_name,g" \
	             -e "s,@JOBNAME@,$app.$totalNodeNum.$scale,g" \
	             -e "s,@TEST_PATH@,$testpath,g" \
	             -e "s,@TESTER_NAME@,$etype,g" \
	             -e "s,@TESTER_NUM@,$tOp,g" \
	             -e "s,@APP_PATH@,$apppath,g" \
	             -e "s,@FAN_OUT@,$fanout,g" \
	             -e "s,@HEI@,$height,g" \
	             -e "s,@CORE_COUNT@,$ncore_per_node,g" \
	             -e "s,@CPONLY_NCOUNT@,$totalCPOnlyNodeNum,g" \
	             -e "s,@COREPERCP@,$corePerCp,g" \
	             -e "s,@NBES@,$totalBENum,g" \
                     -e "s,@CORE_TO_USE_PER_BENODE@,$coreToUsePerBENode,g" \
	             -e "s,@APP@,$app,g" \
                     -e "s,@RTPATH@,$rootpath,g" \
                     -e "s,@TOT_NC@,$totalNodeNum,g" < run.fgfs_tester.sh.in > run.fgfs_tester.sh

                mv run.fgfs_tester.sh $testpath
                chmod u+x $testpath/run.fgfs_tester.sh

              done
            done
      done
done

