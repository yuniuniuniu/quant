#!/bin/bash
ulimit -c unlimited
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
mkdir -p $LOG_PATH
cd $WORK_PATH
nohup $WORK_PATH/consumer Quanter1 > $LOG_PATH/consumer1.log 2>&1 &
nohup $WORK_PATH/consumer Quanter2 > $LOG_PATH/consumer2.log 2>&1 &
nohup $WORK_PATH/consumer Quanter3 > $LOG_PATH/consumer3.log 2>&1 &
nohup $WORK_PATH/consumer Quanter4 > $LOG_PATH/consumer4.log 2>&1 &
nohup $WORK_PATH/producer XMarketCenter > $LOG_PATH/producer.log 2>&1 &
