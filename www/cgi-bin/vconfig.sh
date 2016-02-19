#!/bin/ash
THIS="vconfig.sh"
AJAXMODE=0

print_header(){
	echo "Status: 200 OK"
	echo "Vary: Accept"
	echo "Content-Type: text/html; charset=UTF-8"
	echo "Cache-Control: no-cache"
	echo "Expires: 0"
	echo ""
}

print_inputs(){	
	FILES=$(ls /tmp/config$1*|sed 's/\/tmp\///g')

	echo "<table>"
	for FILE in $FILES;do
		LABEL=$(echo $FILE|sed 's/config//g')
		VALUE=$(cat /tmp/$FILE)

		echo "<tr><td>"$LABEL"</td><td><input style='text-align:right' type='text' name='"$FILE"' id='"$FILE"' value='"$VALUE"'></td></tr>"
	done
	echo "</table>"

}

get_params(){
	ISINPUT=0
	Q=$(echo $QUERY_STRING|tr "?" "&")
	
	NUM=$(expr $(echo $Q|sed 's/\&/\r\n/g'|wc -l) + 1)
	AC=1
	while [ $AC -le $NUM ];do
		PAIR=$(echo $Q|cut -d "&" -f$AC)
		KEY=$(echo $PAIR|cut -d "=" -f1)
		VALUE=$(echo $PAIR|cut -d "=" -f2)
		if [ $(echo $KEY|grep config|wc -l) -gt 0 ];then
			ISINPUT=1
			VALUE=$(echo $PAIR|cut -d "=" -f2|tr "," ".")	
			VALUE=$(echo $VALUE|cut -d "." -f1)
			echo "<br>Aplicando $VALUE $KEY<br>"
			echo $VALUE > /tmp/$KEY	
		else
			if [ "$KEY" = "send_to_imu" ];then
				echo "<h5>Notificado al sistema de navegación</h5>"
				send_to_imu
			fi
			if [ "$KEY" = "save_to_disk" ];then
				echo "<h5>Guardando en disco</h5>"
				save_to_disk
			fi
			if [ "$KEY" = "recover_from_disk" ];then
				echo "<h5>Trayendo datos de disco</h5>"
				recover_from_disk
			fi
			if [ "$KEY" = "kill_imu" ];then
				echo "<h5>Parando IMU</h5>"
				killall imu >/dev/null

			fi
			if [ "$KEY" = "start_imu" ];then
				echo "<h5>Iniciando IMU</h5>"
				/start_imu.sh  &
			fi
			if [ $(echo $KEY|grep ^save_file.|wc -l) -gt 0 ];then 
			        CFILE=${KEY/save_file./}
			        echo "$VALUE"> /tmp/config$CFILE
			        echo "OK"
			        exit
			fi
			if [ $(echo $KEY|grep ^get_file.|wc -l) -gt 0 ];then 
			        CFILE=${KEY/get_file./}
			        cat /tmp/config$CFILE
			        exit
			fi			
			
		fi	
		AC=$(expr $AC + 1)
	done
	if [ $ISINPUT -eq 1 ];then
		echo "<h5>Aplicado a IMU</h5>"
		send_to_imu
	fi
}

print_menu(){
	echo "<h4>"$1"</h4>"
	for I in $2;do
		CTRL=$(echo $I|cut -d "#" -f1)
		TITULO=$(echo $I|cut -d "#" -f2)
		echo "<hr><b>"$TITULO"</b><form action='"$THIS"' id='"$CTRL"' method='get' >"
			print_inputs $CTRL
			echo "<input type='button' value='Aplicar' onclick='javascript:parseInputs(\""$CTRL"\",1);'>"
		echo "</form><script>parseInputs(\""$CTRL"\",0);</script>"	
	done		
	
}

send_to_imu(){
	kill -SIGUSR2 $(cat /tmp/pidIMU) 2>/dev/null
}
save_to_disk(){
	cp -R /tmp/config* /scripts/config/
}
recover_from_disk(){
	cp -R /scripts/config/config* /tmp/ 
}
message_imu(){
	if [ $(ps -w|grep -v kill|grep -v grep|grep -vi terminated|grep imu|wc -l) -gt 0 ];then
		echo "<span style='background:white; color:green'>IMU Funcionando</span>"
	else
		echo "<span style='background:white; color:red'>IMU Parada</span>"
	fi
}
#main
print_header
get_params




echo "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"\>"
echo "<html><head><title>Configuración TCC Interceptor</title><script src='../prototype.js' ></script>"


echo "<script>function parseInputs(frm, mode){"
echo "  var form = \$(frm);"
echo "	if (mode==1){"
echo "    	form.getInputs('text').each(function(item){item.value=item.value*1000000;if (item.value=='NaN'){item.value='';}});"
echo "	  	form.submit();"
echo "  }else{"
echo "		form.getInputs('text').each(function(item){item.value=item.value/1000000;if (item.value=='NaN'){item.value='';}});}"
echo "}</script>"
echo "<body style='font-family:Arial;background:grey'>"
echo "<h3><a href='"$THIS"'>Configuracion TCC Interceptor</a></h3>"


message_imu
echo "<input type='button' value='Parar IMU' id='kill_imu' onclick=""\"window.location='"$THIS"?kill_imu'\""" />"
echo "<input type='button' value='(Re)iniciar IMU' id='start_imu' onclick=""\"window.location='"$THIS"?start_imu'\""" /><br/>"
echo "<input type='button' value='Traer valores de disco' id='recover_from_disk' onclick=""\"window.location='"$THIS"?recover_from_disk'\""" />"
echo "<input type='button' value='Guardar valores a disco' id='save_to_disk' onclick=""\"window.location='"$THIS"?save_to_disk'\""" />"
echo "<input type='button' value='Aplicar a la IMU' id='send_to_imu' onclick=""\"window.location='"$THIS"?send_to_imu'\""" />"
echo "<hr>"
print_menu "Declinacion magnetica local" "DeclinacionMag#Declinacion_Grados_Decimales"
print_menu "Control PID" "Kalabeo#Alabeo Kcabeceo#Cabeceo Kginnada#Viraje"
print_menu "Factores Paso Bajo" "LowPass#Sensores"
exit
print_menu "Offsets Magnetometro" "OffsetMag#Centrado"
print_menu "Factores Filtro Complementario" "TAUroll#Alabeo TAUpitch#Cabeceo TAUyaw#Guiñada"
print_menu "Factores Filtro Paso Bajo del Giroscopio" "GyroAlabeo#Alabeo GyroCabeceo#Cabeceo GyroGinnada#Guiñada"
print_menu "Factores Filtro Paso Bajo del Acelerómetro" "AccelAlabeo#Alabeo AccelCabeceo#Cabeceo AccelGinnada#Eje_Z"

echo "</body></html>"
