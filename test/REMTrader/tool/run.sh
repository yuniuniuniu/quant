#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
cd $WORK_PATH
pkill coll_rptr
rm -rf *.log
sleep 2
export LD_LIBRARY_PATH=$WORK_PATH/Lib:$LD_LIBRARY_PATH
nohup $WORK_PATH/coll_rptr $WORK_PATH/coll_rptr_conf.xml > $WORK_PATH/run.log 2>&1 &
echo $! > pid.txt
