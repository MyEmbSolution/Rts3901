#!/bin/sh

if [ $# -ge 1 ]; then
	ACTION=$1
else
	ACTION="start"
fi

if [ $# -ge 2 ]; then
	CFG_DIR=$2
else
	CFG_DIR=/usr/conf/peacock.json
fi

if [ "$ACTION" == "start" ] || [ "$ACTION" == "restart" ]; then
	killall peacock
	killall rtspd

	peacock -c $CFG_DIR -v 8 &
	rtspd -c $CFG_DIR -v 8 &
elif [ "$ACTION" == "stop" ]; then
	killall peacock
	killall rtspd
else
	echo "Invalid Action $1"
fi


