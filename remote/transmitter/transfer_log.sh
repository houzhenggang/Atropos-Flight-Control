#!/bin/ash
cp /etc/config/ATROPOS_wireless /etc/config/wireless
wifi

TRG="root@192.168.251.113"
PORT=443
SSHK=/root/.ssh/id_rsa
KEY=$(cat log_location)
KEYR="/scripts/pilot.txt"
SCOPY="scp -i $SSHK -o ConnectTimeout=10 -P $PORT $KEY $TRG:$KEYR"
$SCOPY
