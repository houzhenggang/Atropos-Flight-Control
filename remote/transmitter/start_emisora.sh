#!/bin/sh
HOME=/root
killall -9 udp_emisora_mando
killall -9 command_key.sh
sleep 1
cp /etc/config/ATROPOS_wireless /etc/config/wireless
wifi
sleep 1
#exit
echo Encuestando mando

./udp_emisora_mando B S F &
sleep 1
killall -9 udp_emisora_mando
F1=$(cat $(cat log_location)|grep QQZ0Z0|wc -l)
L=$(cat $(cat log_location)|wc -l)


if ([ $F1 -gt 0 ]||[ $L -le 1 ]);then
	echo Manteniendo conexion WPA
	./udp_emisora_mando F F F &
	exit
fi



echo Intercambio de Clave
./udp_emisora_mando B S B
sleep 1
cp /etc/config/MONITOR_wireless /etc/config/wireless
wifi
sleep 1
echo Modo Monitor
iw dev wlan0 set channel 11 HT40-
./channel_issue.sh &
sleep 1
echo LAnzando mando monitor
nice -20 ./udp_emisora_mando wlan0 K B


