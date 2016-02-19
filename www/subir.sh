#!/bin/bash

SSHC="root@192.168.250.70"
subir(){

	scp $1 $SSHC:$2

}
comando(){
	ssh $SSHC $1
}

comando "mkdir /imu; mkdir /scripts; mkdir /imu/config"

subir "../WEB/*" "/www/"
subir "../WEB/cgi-bin/*" "/www/cgi-bin/"

