#!/bin/bash
WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
rm -rf $WORK_PATH/log/*
rm -rf $WORK_PATH/*.con
cd $WORK_PATH
kill -9 $(cat pid.txt)
