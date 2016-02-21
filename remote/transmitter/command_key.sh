#!/bin/sh

TRG="root@192.168.251.113"
PORT=443
SSHK=/root/.ssh/id_rsa
KEY=/tmp/key_file
KEYR="/tmp/key_rfmon"
SHELL="ssh -i $SSHK -o ConnectTimeout=10 -p $PORT $TRG "
SCOPY="scp -i $SSHK -o ConnectTimeout=10 -P $PORT $KEY $TRG:$KEYR"
echo ""
echo $SCOPY
$SCOPY 
RET=-1
RET=$(expr  1 - $($SHELL "cd /scripts/ && ./refresh_key.sh"|grep "@OK#"|wc -l) )
echo ""
echo RESULTADO $RET
exit $RET

