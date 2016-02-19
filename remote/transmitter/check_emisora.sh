#!/bin/sh

AC=0
if [ ! -f /tmp/started ];then
	touch /tmp/started
	exit
fi



if [ -f /tmp/no_check_emisora ];then
	exit
fi 

if [ $(ls /dev/input/js0 2>/dev/null|wc -l) -eq 0 ];then
	echo No hay mando
	cd /etc/config/
	cp ATROPOS_wireless wireless
	wifi
	touch /tmp/no_check_emisora

	exit
fi


while [ $AC -lt 5 ];do

	UDPE=$(ps -w|grep udp_emisora_mando|grep -v grep|wc -l)
	if [ $UDPE -le 0 ];then
		echo udp_emisora no presente, lanzando
		cd /etc/emisora
		./start_emisora.sh &
		exit
	fi
	sleep 5
done



