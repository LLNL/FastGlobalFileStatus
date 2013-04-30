#!/bin/sh
## $Header: $
##
## MPIScalingSetup.sh -- this is a script that sets up FGFS scaling tests for MPI.
##    This creates experiment directories each of which contains a batch script
##    to run at various scales. The testers are async_stat_dso_mpi (AsyncGlobalFileStatus)
##    and sync_stat_dso_mpi (SyncGlobalFileStatus), which run FGFS' global 
##    file status queries:
##       - isFullyDistributed (0)
##       - isWellDistributed (1)
##       - isPoorlyDistributed (2)
##       - isUnique (3)
##       - isConsistent (4).
##    The last query is only available with SyncGlobalFileStatus.
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
##        Apr 29 2013 DHA: File created
##

ncore_per_node=1
mach_name=""
apppath=""

if [ $# -ne 3 ]
then
    echo ""
    echo "Usage: MPIScalingSetup.sh <number of cores per testing node> <testing machine name> <target application executable path>"
    echo ""
    echo "  This is a script that sets up FGFS scaling tests for MPI."
    echo "  This creates experimental directories each of which contains a batch script (MOAB/Slurm)"
    echo "  to run test programs at various scales. The testers are async_stat_dso_mpi "
    echo "  (AsyncGlobalFileStatus) and sync_stat_dso_mpi (SyncGlobalFileStatus), "
    echo "  which run FGFS' global file status queries:"
    echo "     - isFullyDistributed (0)"
    echo "     - isWellDistributed (1)"
    echo "     - isPoorlyDistributed (2)"
    echo "     - isUnique (3)"
    echo "     - isConsistent (4)."
    echo "   The last query is only available with SyncGlobalFileStatus."
    exit 1
fi
  
ncore_per_node=$1
mach_name=$2
apppath=$3
app_names="TestApp"
testers="async_stat_dso_mpi sync_stat_dso_mpi"
fgfs_ops="0 1 2 3 4"
taskscount="512 1024 2048 4096 8192 16384 32768 65536"
dir_ptr="FGFS.Scaling.mpi"
rootpath=`pwd`


mkdir -p $dir_ptr
dir_ptr="$dir_ptr/MPI"
mkdir $dir_ptr

for etype in $testers ; do
      mkdir -p $dir_ptr/$etype

      for app in $app_names ; do
            mkdir -p $dir_ptr/$etype/$app

            for scale in $taskscount ; do
              mkdir -p $dir_ptr/$etype/$app/$scale

              for tOp in $fgfs_ops; do 
                 testpath=$rootpath/$dir_ptr/$etype/$app/$scale/$tOp
                 mkdir $testpath
                 ln -s ../../../../../../$etype $testpath/
                 totalNodeNum=`expr $scale / $ncore_per_node`
                 mod=`expr $scale % $ncore_per_node`

                 if [ $mod -ne 0 ]
                 then
                     totalNodeNum=`expr $totalNodeNum + 1`
                 fi

                 sed -e "s,@NNODES@,$totalNodeNum,g" \
	             -e "s,@NTASKS@,$scale,g" \
	             -e "s,@MACH@,$mach_name,g" \
	             -e "s,@JOBNAME@,$app.$totalNodeNum.$scale,g" \
	             -e "s,@TEST_PATH@,$testpath,g" \
	             -e "s,@TESTER_NAME@,$etype,g" \
	             -e "s,@TESTER_NUM@,$tOp,g" \
	             -e "s,@APP_PATH@,$apppath,g" \
	             -e "s,@APP@,$app,g" \
                     -e "s,@RTPATH@,$rootpath,g" < run.fgfs_tester_mpi.sh.in > run.fgfs_tester_mpi.sh

                mv run.fgfs_tester_mpi.sh $testpath
                chmod u+x $testpath/run.fgfs_tester_mpi.sh

              done
            done
      done
done

