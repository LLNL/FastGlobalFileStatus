#!/bin/sh

ncore_per_node=12
mach_name="sierra"

if [ $# -eq 1 ]
then
ncore_per_node=$1
mach_name=$2
fi

#app_names="kull"
app_names="ale3d ares kull"
testers="async_stat_dso_mrnet"
fgfs_ops="0 1 2 3"
#be_count="144"
be_count="144 256 512 1331 2197 4096 10000 14641"
dir_ptr="FGFS.Scaling"
cP2FoRatio=3
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
                 height=`mrnet_node_req 1 $scale $ncore_per_node $cP2FoRatio`
                 fanout=`mrnet_node_req 2 $scale $ncore_per_node $cP2FoRatio`
                 totalBENum=`mrnet_node_req 3 $scale $ncore_per_node $cP2FoRatio`
                 totalCPNum=`mrnet_node_req 4 $scale $ncore_per_node $cP2FoRatio`
                 totalBENodeNum=`mrnet_node_req 5 $scale $ncore_per_node $cP2FoRatio`
                 totalCPNodeNum=`mrnet_node_req 6 $scale $ncore_per_node $cP2FoRatio`
                 totalNodeNum=`mrnet_node_req 7 $scale $ncore_per_node $cP2FoRatio`
                 usingCorePerNodeCP=`mrnet_node_req 8 $scale $ncore_per_node $cP2FoRatio`

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
	             -e "s,@MACH@,$mach_name,g" \
	             -e "s,@JOBNAME@,$app.$totalNodeNum.$scale,g" \
	             -e "s,@TEST_PATH@,$testpath,g" \
	             -e "s,@TESTER_NAME@,$etype,g" \
	             -e "s,@TESTER_NUM@,$tOp,g" \
	             -e "s,@APP_PATH@,$apppath,g" \
	             -e "s,@FAN_OUT@,$fanout,g" \
	             -e "s,@HEI@,$height,g" \
	             -e "s,@CORE_COUNT@,$ncore_per_node,g" \
	             -e "s,@CP_NCOUNT@,$totalCPNodeNum,g" \
	             -e "s,@NCPS@,$totalCPNum,g" \
	             -e "s,@NBES@,$totalBENum,g" \
	             -e "s,@APP@,$app,g" \
                     -e "s,@RTPATH@,$rootpath,g" \
                     -e "s,@TOT_NC@,$totalNodeNum,g" < run.fgfs_tester.sh.in > run.fgfs_tester.sh

                mv run.fgfs_tester.sh $testpath
                chmod u+x $testpath/run.fgfs_tester.sh

              done
            done
      done
done

