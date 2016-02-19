#!/bin/ash
KFILE="/tmp/key_rfmon"
RFPROC=rfmon_recv
RECV=/tmp/key_changed
rm -f $RECV
if [ -f $KFILE ];then
	if [ "$(pidof $RFPROC)" = "" ];then
		echo @ERROR#NOPROC
	else
	
		kill -SIGUSR2 $(pidof $RFPROC)
		AC=0
		while (! [ -f $RECV ]&&[ $AC -lt 10 ]);do
			sleep 1
			AC=$(expr $AC + 1)
		done
		if [ -f $RECV ];then
			cat $RECV
		else
			echo @ERROR#RECV
		fi	
	fi	
else
	echo @ERROR#NOFILE
fi

