#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
export APP_LOG_PATH=$WORK_PATH/log
mkdir -p $APP_LOG_PATH
cd $WORK_PATH
sleep 2
#export APP_LOG_PATH=/home/Deploy/Log/`date +%Y%m%d`
export LD_LIBRARY_PATH=$WORK_PATH/Lib:$LD_LIBRARY_PATH
nohup $WORK_PATH/XWatcher_1.0.0 -d -a XWatcher -f $WORK_PATH/XWatcher.yml > $APP_LOG_PATH/XWatcher_1.0.0_run.log 2>&1 &
echo $! > pid.txt
