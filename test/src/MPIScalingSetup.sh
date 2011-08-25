#!/bin/sh

ncore_per_node=12
mach_name="sierra"

if [ $# -eq 1 ]
then
ncore_per_node=$1
mach_name=$2
fi

app_names="ale3d ares kull"
testers="async_stat_dso_mpi sync_stat_dso_mpi"
fgfs_ops="0 1 2 3 4"
taskscount="128 256 512 1024 2048 4096 8192 16384"
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

                 apppath=""
                 if [ $app = "kull" ]
                 then
                     apppath="/usr/gapps/kull/chaos_4_x86_64_ib/bin/.v2.4/debug/bin/kull"
                 elif [ $app = "ale3d" ]
                 then
                     apppath="/usr/mic/bdiv/ALE3D/chaos_4_x86_64_ib2/BUILD/v4.12.9/dbg/src/ale3d"
                 elif [ $app = "ares" ]
                 then
                     apppath="/usr/gapps/ARES/public/chaos_4_x86_64_ib/exec/ares.1.24.23"
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

