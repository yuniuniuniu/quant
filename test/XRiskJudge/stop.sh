WORK_PATH=$(cd $(dirname $0); pwd)
LOG_PATH=$WORK_PATH/log
cd $WORK_PATH
echo "delete from CancelledCountTable;" | sqlite3 $WORK_PATH/XRiskJudge.db
kill -9 $(cat pid.txt)
rm -rf $WORK_PATH/log/*

