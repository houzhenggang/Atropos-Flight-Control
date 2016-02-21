#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <signal.h>
#include <errno.h>
#define FTELEMETRY "/tmp/teleIMU"

#define BUFFER 750
#define BIGBUFFER 10000
char  result[BIGBUFFER];
char survey[BIGBUFFER];
unsigned char *read_mem;
char hayComm=0;
int  shmidw;


int buffer_to_file(char * source_buffer,int buffer_size ,char *target_file){
        int fd;
        int result=-1;
        fd = open(target_file, O_WRONLY|O_CREAT|O_TRUNC);
        if (fd>0){
                result=write(fd,source_buffer,buffer_size);
                close(fd);
        }
        return(result);

}
char * get_data(char * file ){
	int fd;
	int reads=0;
	
	result[0]='\0';
	fd=open(file,O_RDONLY);
	if (fd>0){
		reads=read(fd,result,BIGBUFFER);
		close(fd);
	}
	
	return result;
}

float fromBin(unsigned char * stream, unsigned int * offsetStream,unsigned int bytes, unsigned int floatPrecision){
	float out=0;
	unsigned long uout=0;
	char issigned =0;
	unsigned int ac=0;
	issigned=(stream[(*offsetStream)]>>7)&0x01;

	stream[(*offsetStream)]&=0x7F;
	unsigned int endOffset;
	unsigned int bitOffset;
	endOffset=bytes+(*offsetStream);
	unsigned int count=0;

	for(ac=(*offsetStream);ac<endOffset;ac++){
		bitOffset=((bytes-1-count)*8);
		uout|=(stream[ac])<<bitOffset;
		count++;

	}
	out=uout;

	if(floatPrecision>0){
	    out=out/floatPrecision;
	}
	if(issigned>0){
	  out=-out;
	}
	(*offsetStream)+=count;	

	return out;
}

int shared_out_init()
{
	if((shmidw = shmget(9997, 256, IPC_CREAT | 0666)) < 0) {
		printf("Error in shmget. errno is: %d\n", errno);
		return -1;
	}	
	if((read_mem = (unsigned char *) shmat(shmidw, NULL, 0)) < 0) {
		
		printf("Error in shm attach. errno is: %d\n", errno);
		return -1;
	}
	return 0;
}

void send_jsonoutput(){
	hayComm=1;
	unsigned char jsalida[BIGBUFFER];
	unsigned int offset=2;
	int ac=0;
	
	  sprintf(jsalida,"\n\n\n<script>window.parent.telecomet.input({  \
			    \"encendido\":%i,\"cpu_overflow\":%i, \"yaw_control\":%i, \"failsafe\": %i, \"snd\":%i, \
			    \"sensor\":{ \"alabeo\":%.3f,\"v_alabeo\":%.3f,\"cabeceo\":%.3f,\"v_cabeceo\":%.3f,\"guinada\":%.3f,\"v_ginada\":%.3f}, \
			    \"target\":{ \"alabeo\":%.3f,\"v_alabeo\":%.3f,\"cabeceo\":%.3f,\"v_cabeceo\":%.3f,\"guinada\":%.3f,\"v_ginada\":%.0f, \"altitud\":%.0f, \"v_altitud\": %.0f}, \
			    \"motor\" :{ \"value\":[%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f], \"max\": %0.f}, \
			    \"rawGyro\":{ \"x\":%0.f,\"y\":%0.f,\"z\":%0.f}, \
			    \"rawAccel\":{ \"x\":%0.f,\"y\":%0.f,\"z\":%0.f}, \
			    \"rawMag\":{ \"x\":%0.f,\"y\":%0.f,\"z\":%0.f}, \
			    \"analog\" :[%.3f,%.3f], \
                \"dcm\" :[[%.3f,%.3f,%.3f],[%.3f,%.3f,%.3f],[%.3f,%.3f,%.3f]], \
                \"texas\":{\"turret\":{\"rumbo\":%0.f, \"elev\": %0.f,\"comp\": %0.f},\"motors\":{\"l\":%0.f, \"r\":%0.f}, \"switches\":%0.f},\
                \"msg\":\"%s\" \
			  });</script>",
			  (unsigned int)(read_mem[0]&0x01),(unsigned int)((read_mem[0]>>1)&0x07),(unsigned int)((read_mem[0]>>4)&0x03),(unsigned int)((read_mem[0]>>6)&0x01),(unsigned int)read_mem[1],
			  fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
			  fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
		  
			  fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
			  fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
		  
			  fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),
		  
			  fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),//motores
              fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),fromBin(read_mem,&offset,3,0),
              fromBin(read_mem,&offset,3,0),
	  
	   
			  fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),//raws
			  fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),
			  fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),
			  fromBin(read_mem,&offset,2,100),fromBin(read_mem,&offset,2,100),
              
              fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),//dcm
              fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
              fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),fromBin(read_mem,&offset,3,1000),
		  
		  fromBin(read_mem,&offset,2,0),fromBin(read_mem,&offset,2,0),fromBin(read_mem,&offset,2,0),fromBin(read_mem,&offset,1,0),fromBin(read_mem,&offset,1,0),fromBin(read_mem,&offset,1,0),//texas
              
			  &read_mem[150]
			  );

	  write(1,jsalida,strlen(jsalida));
}


int main(void){
  
	char salida[BUFFER];
	char last_salida[BUFFER];
	char input[BUFFER];
	char generic_buffer[BUFFER];
	int imupid=-1;
	shared_out_init();
	
	sprintf(generic_buffer,"%i",getpid());

	buffer_to_file(generic_buffer,5,"/tmp/pidOUTPUT");	
	get_data("/tmp/pidIMU");
	
	if(result[0]!='\0'){
	    imupid=atoi(result);
	}else{
	    exit(-1);
	}
	
	kill(imupid,SIGUSR1);
	
	
	sprintf(salida,"Status: 200 OK\r\nVary: Accept\r\nContent-Type: text/html; charset=UTF-8\r\nCache-Control: no-cache\r\nExpires: 0");
	write(1,salida,strlen(salida));
	//sprintf(salida,"\r\n\r\n<html><body><script type='text/javascript'>var comet=window.parent.telecomet;</script>\r\n");
	//write(1,salida,strlen(salida));	
	//signal(SIGUSR1,send_jsonoutput);
	signal(SIGUSR2,send_jsonoutput);

	hayComm=1;
	while(hayComm==1){	
		hayComm=0;
		sleep(7);		
	}
	sprintf(salida,"<script>comet.input('EXIT_INTERPROCESS');</script>");
	write(1,salida,strlen(salida));	
}
