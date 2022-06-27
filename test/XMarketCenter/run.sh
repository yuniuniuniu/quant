#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
export APP_LOG_PATH=$WORK_PATH/log
cd $WORK_PATH
mkdir -p $APP_LOG_PATH
sleep 2
export LD_LIBRARY_PATH=$WORK_PATH/Lib:$LD_LIBRARY_PATH
nohup $WORK_PATH/XMarketCenter_0.3.0 -a XMarketCenter -f $WORK_PATH/Config/XMarketCenter.yml > $APP_LOG_PATH/XMarketCenter_run.log 2>&1 &
echo $! > pid.txt
