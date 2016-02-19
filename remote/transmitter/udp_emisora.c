#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <pcap.h>
#include <gcrypt.h>
#include <sched.h>
#define CONTROL_DEVICE "/dev/input/js0"
#define LOG_FILE_TARGET "log_location_"


#define MANDO_ABORDO 
#define GAS_FACTOR 1800000
#define GAS_CONFIG 39000000
#define START_WITH_YAW  2000000 //DCHA
#define STOP_WITH_YAW  -1800000  //IZDA
#define GUINNADA_FACTOR 20000
#define ALABEO_FACTOR 13000
#define CABECEO_FACTOR 13000



#ifdef MANDO_ABORDO
	#define ROUND_WINDOW 10
#else
	#define ROUND_WINDOW 350
#endif

#define BEACONS_PER_REQUEST 1

#define IP  "192.168.250.70"
#define IP_MON "192.168.251.71"


#define PORT 5555
#define PORT_MON 5556
#define MSG 8
#define OUTPUT 30

#define DEFAULT_PCAP_DEV "mon0"
#define GCRY_KEYLEN 16
#define GCRY_IVLEN 16
#define CHECKSUM_LEN 2
#define SEQ_LEN 4
#define GCRY_PAYLOAD 64
#define COMMAND_LEN GCRY_PAYLOAD-CHECKSUM_LEN-SEQ_LEN
#define MAC_LEN 6
#define KEY_FILE "/tmp/key_file"

#define MODE_SEND 'B'
char stage[250]="N";
struct timeval s_tstart;
struct timeval s_tend;
int seq_stage=0;	
int rfmon_avail=0;
char my_str_conn[30]="I192.168.250.1:5555      "; 
unsigned long int seq_num=0;
extern int errno;
char fork_input[OUTPUT]="";
pcap_t * capture;
u_char mac[6]={0x00,0xe0,0x4c,0x6c,0x05,0x80};
char gcry_key[GCRY_KEYLEN]={0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x01};
unsigned long seq=2;
gcry_cipher_hd_t  gcry_hd;

void print_gcrypt_err(gcry_error_t err){

        if ( err )
        {
            fprintf ( stderr, "Failure: %s/%s\n",
                      gcry_strsource ( err ),
                      gcry_strerror ( err ) );
            fprintf ( stdout, "Failure: %s/%s\n",
                      gcry_strsource ( err ),
                      gcry_strerror ( err ) );

        }

}
void print_vector(char * vector, int len){
        int ac=0;
        printf("\r\n");
        for(ac=0;ac<len;ac++){
                printf(" %x",vector[ac]&0x0000FF);
        }
        printf("\r\n");
}

void random_vector(char * arr, int size){
        gcry_randomize (arr, size,GCRY_STRONG_RANDOM);
}
u_int checksum(char * command, int len){

	
        len-=CHECKSUM_LEN;
	//printf("\r\n PACKET FOR CHECK");
	//print_vector(command, len);
        int ac=0;
        u_int off=8;
        u_long t=0;
        u_int c=0;
        for(ac=0;ac<len;ac++){
                t=t+((((u_int)command[ac])<<off));
                if(off==8){off=0;}else{off=8;}
        }
       /* do{
                c=(t>>16);
                t=c+(t&0x00000000FFFF);
        }while(c!=0x00);
        t=(~t);*/
        return ((u_int)t)&0x00000000FFFF;

}

void init_gcrypt( gcry_cipher_hd_t * hd){
        if (!gcry_check_version (GCRYPT_VERSION))
        {
                printf ("libgcrypt version mismatch\n");
                exit (2);
        }

        gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
        gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
        gcry_cipher_open(hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CFB, 0);
}

void inject_command(pcap_t *fp, gcry_cipher_hd_t * hd,char * dst_mac, char * command, int len, unsigned long * seqnum){
        char pkt[1000];
        u_char buf[1024];
        u_char iv[GCRY_IVLEN]={0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};
        len+=SEQ_LEN;
        char pkt_len[6];


        char type[] =  {    0x00,0x00,0x0d,0x00,0x04,0x80,0x02,0x00,0x02,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
			    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x64,0x00,0x11,0x00,0x00,0x05,
		             'I', 'N', 'T', 'C', 'P',0x01,0x04,0x82,0x84,0x8b,0x96,0x03,0x01,0x0c,0x04,0x06,0x01,0x02,0x00,0x00,0x00,0x00,0x05,0x04,0x00,0x01,0x00,0x00,0xdd,0x18,0x00,0x50,0xf2,0x01,0x01,
			    0x00,0x00,0x50,0xf2,0x04,0x01,0x00,0x00,0x50,0xf2,0x04,0x01,0x00,0x00,0x50,0xf2,0x02,0x00,0x00}; 



        u_int tlen=len+MAC_LEN+CHECKSUM_LEN+GCRY_IVLEN;
        random_vector(iv,GCRY_IVLEN);
        memcpy(&pkt[0],dst_mac,MAC_LEN);
        memcpy(&pkt[MAC_LEN],iv,GCRY_IVLEN);
        gcry_cipher_setiv(*hd,iv, GCRY_IVLEN);


        char cmdseq[1000];
        memcpy(cmdseq,command,len);
        u_int ac=0;
        (*seqnum)++;
        if(*seqnum>0xFFFFFFFF){*seqnum=0x01;}
        for(ac=0;ac<SEQ_LEN;ac++){
                cmdseq[len-1-ac]=(char)((*seqnum)>>(ac*8));
        }

        gcry_cipher_encrypt(*hd, &pkt[GCRY_IVLEN+MAC_LEN],len , cmdseq, len);

        u_int chk=checksum(pkt,tlen);
        u_char chkarr[CHECKSUM_LEN];
        chkarr[0]=(chk>>8);
        chkarr[1]=chk&0x00FF;

        memcpy(&pkt[GCRY_IVLEN+MAC_LEN+len], chkarr,CHECKSUM_LEN);
        memcpy(buf, type, sizeof(type));
        memcpy(buf+sizeof(type), pkt, tlen);

	#ifndef MANDO_ABORDO
        print_vector(pkt,tlen);
        #endif

        for(ac=0;ac<BEACONS_PER_REQUEST;ac++){
	    int inj=pcap_inject(fp,buf, sizeof(type)+sizeof(pkt_len)+tlen);
	    #ifndef MANDO_ABORDO
		printf("\r\n INJ %i",inj);
	    #endif	
            
        }
        
}
void query(char * q, char target_mode){
   int sock;
   
   struct hostent *hp;
   struct sockaddr_in server;
   seq_stage++;

   if (stage[0]!='K'){
	   sock= socket(AF_INET, SOCK_DGRAM, 0);
	   server.sin_family = AF_INET;

	   if((target_mode=='F')||(target_mode=='B')||(seq_stage==10)){
	
		   server.sin_port = htons(PORT);
		   hp = gethostbyname(IP);
	   }else{
		   server.sin_port = htons(PORT_MON);
	       hp = gethostbyname(IP_MON);
	   }
	   if (hp==0) printf("Unknown host");
	   bcopy((char *)hp->h_addr, (char *)&server.sin_addr,hp->h_length);

	   
	   if((target_mode=='F')||(seq_stage==10)){

	  	seq_stage=0;
#ifndef MANDO_ABORDO		
	   	fprintf(stdout,"\n %li \t -> %s",seq_num,q);
#endif
		
		sendto(sock,q,strlen(q),0,(const struct sockaddr *)&server,sizeof(struct sockaddr_in));
	   }
	   close(sock);
   }
   if(target_mode!='F'){
	#ifndef MANDO_ABORDO
		fprintf(stdout,"\n %li RFMON->",seq_num);
	#endif
	
	
	//print_vector(q,COMMAND_LEN);
	//sendto(sock,q,COMMAND_LEN,0,(const struct sockaddr *)&server,sizeof(struct sockaddr_in));
        if(rfmon_avail){
			
	            inject_command(capture,&gcry_hd,mac,q, COMMAND_LEN, &seq_num);
	}


   }
	#ifndef MANDO_ABORDO
   fflush(stdout);
#endif
   		 
   
}

int notify_key_to_rfmon(char * key){

    FILE *fp;
    int exitStat=1;
    fp = fopen(KEY_FILE, "wb");
    int writed=fwrite(key,sizeof(char),GCRY_KEYLEN,fp);
			printf("\n clave guardada\n");
		print_vector(key,GCRY_KEYLEN);
    fclose(fp);
    if(writed==GCRY_KEYLEN){
	 exitStat= WEXITSTATUS(system("./command_key.sh"));
	printf("\n KEY RESULT %i", exitStat);
    }else{
	printf("\n FAILED TO PRINT KEY");
	}


    return exitStat;
}

int load_key_from_file(char *gkey){
	FILE *fp;
	fp = fopen(KEY_FILE, "rb");
	int read=fread(gkey,sizeof(char),GCRY_KEYLEN,fp);
	fclose(fp);
	if(read==GCRY_KEYLEN){
		printf("\n clave cargada\n");
		print_vector(gkey,GCRY_KEYLEN);
		return 0;
	}
	return -1;
}

/*
int mygetch( ) {
	struct termios oldt, newt;
	int ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}
*/
int abs(int val){
	if(val<0){
		return -val;
	}
	return val;
}

int main(int argc, char *argv[]){
      char output[COMMAND_LEN]="";
      char last_output[COMMAND_LEN]="";		 
      char msg[MSG];
      int pad=0;
      int reads=0;
      int sock_atropos=0;
      int encendido=0;
      int gas=0;
      int alabeo=0;
      int cabeceo=0;
      int guinnada=0;
      int pipe_des[2];	
      char com[20]="";
      int analog=0;
      char conectado=0;
      int rounds=0;
      int alive=0;
      int ispresent=0;
      char * ptok;
      int out_gas;
      int last_gas;
      int send_gas=0;
      int send_guinnada=0;
      int ignore_round=0;
      int transient_present=0;
      int config=0;
      char device[100]="wlan0";
      int result=0;
      char errorBuffer[PCAP_ERRBUF_SIZE];

      char mode_send=MODE_SEND;

      const u_char *pkt_data;
      struct pcap_pkthdr *header;

	printf("\r\nEMISORA INTERCEPTOR\n\n");
      

      if(argc>1){
        if (argv[1][0]=='#'){
	        strcpy(my_str_conn,&argv[1][1]);
            if (argc>2){strcpy(device,argv[1]);}

        }else{
            strcpy(device,argv[1]);
	    if(argc>=2){
		strcpy(stage, argv[2]);
		if(argc>=3){
			mode_send=argv[3][0];
		}
	    }
        }
	

      }

	struct sched_param schedule;

	schedule.sched_priority=sched_get_priority_min(SCHED_OTHER);
	sched_setscheduler(pthread_self(), SCHED_OTHER,&schedule);	

	schedule.sched_priority=sched_get_priority_min(SCHED_FIFO);
	sched_setscheduler(pthread_self(), SCHED_FIFO,&schedule);

    
    printf("\nSTAGE %s", stage);
    printf("\nMODE  %c", mode_send);
     

    char log_file_path[128]="";
    int hastolog=0;
    FILE *fl;
    fl = fopen(LOG_FILE_TARGET, "r");

    if (fl!=NULL){

	int readp=fread(log_file_path,sizeof(char),128,fl);
	log_file_path[readp-1]='\0';
	fclose(fl);
        fl=fopen(log_file_path,"w+");
        if(fl!=NULL){hastolog=1; printf("\nLOG DE QUERY ACTIVADO");}
	
    }

    printf("\nLOG STATUS %i  %s ", hastolog, log_file_path);


    if(mode_send!='F'){

	    pcap_if_t * allAdapters;
	    pcap_if_t * adapter;  
	    printf("\nABRIENDO PCAP");
	    fflush(stdout);
	    result= pcap_findalldevs(&allAdapters, errorBuffer );
	    printf("\nRespuesta PCAP: %i\n", result);
	    fflush(stdout);
	    if(result==0){
		
		for( adapter = allAdapters; adapter != NULL; adapter = adapter->next)
		{
		        //printf("\r\nDetectando %s", adapter->name);
		        if (strcmp(adapter->name, device)==0){
	   		    rfmon_avail=1;                 
		            break;
		        }
		        
		}
	    }


	    if(rfmon_avail){
		init_gcrypt(&gcry_hd);

	    	seq=1;

	    	printf("\nInterfaz en RFMON cargada: %s",adapter->name );
	    	capture = pcap_open_live(device, BUFSIZ, 1, 1000, errorBuffer);
	       

	    	printf("\nResultado: %s\n",pcap_geterr(capture));      
	    	if(capture==NULL){printf("\n NULL HANDLER");}
	    
	   }else{
		printf("\nADVERTENCIA: Sin soporte con RFMON");
	
	   }
   } 
    printf("\nAbriendo entrenador");

    pad=open(CONTROL_DEVICE,O_RDONLY|O_NONBLOCK);
    char input='n';
     if(pad<=0){
        printf("\nNo se encuentran mandos. Continuar?[s/N]:");
	scanf ("%c",&input);
        if(input!='s'){
                exit(-1);
        }
    }else{
    conectado=1;
    }
    conectado=1;



    printf("\nEntrenador en #%i\n",pad);
    encendido=0;
  
    ispresent=1;
    last_gas=20000;
       if(hastolog==1){	
		sprintf(output, "QQZ%iZ0Z0Z0Z", last_gas);	
		printf("\n ESCRITO PRIMER LOG %i",fprintf(fl,"%s\n", output ));
		fflush(fl);
       } 
	if (mode_send!='F'){
		if ((stage[0]=='S')||(stage[0]=='N')){	
			printf("\nGENERANDO CLAVE");	
			random_vector(gcry_key,GCRY_KEYLEN);		
		}
		if(stage[0]=='K'){
			
			int loadkey=load_key_from_file(gcry_key);
			printf("\nCARGADA CLAVE: %i", loadkey);
			if(loadkey!=0){exit(-1);}
		}else{
			while(notify_key_to_rfmon(gcry_key)!=0){
				printf("\nKEY EXANGE FAILED, RETRY");
				sleep(1);
			}
		}
		 printf("\r\n KEY EXANGE OK");
		if (stage[0]=='S'){
			printf("\nETAPA GENERAR CLAVE HECHO");
			exit(0);
		}      
	        if(rfmon_avail){
			gcry_cipher_setkey (gcry_hd,gcry_key,GCRY_KEYLEN);
	        } 
      }


    while(1==1){
	
    reads=read(pad,msg,MSG);

    if(rfmon_avail==0){mode_send='F';}

    if (reads==MSG){
        alive=0;
        encendido=1; 
        analog=msg[5];	
        //printf("\r\n%i ANALOG %i",msg[7], analog);	
        if((msg[7]==2)&&(encendido==1)){//gas
            gas=((analog+127))*GAS_FACTOR;
          if((abs(gas-send_gas)<100000000)||((gas<228600000)&&(gas>20000))||(transient_present==1)){
		    //printf("\r\n!GAS_SPIKE! %i",abs(gas-last_gas));
            //send_gas=gas;  
            transient_present=0;
           send_gas=gas;

	    }
#ifndef MANDO_ABORDO
		else{
	
            printf("!");

            }
#endif          
          
        }
        if((msg[7]==4)&&(encendido==1)){//guiñada
          guinnada=-analog*GUINNADA_FACTOR;
        }
        if((msg[7]==1)&&(encendido==1)){//cabeceo
          cabeceo=-analog*CABECEO_FACTOR;
        }
        if((msg[7]==0)&&(encendido==1)){//alabeo
          alabeo=-analog*ALABEO_FACTOR;
        }  
        conectado=1;   
    }else{
        alive++;
    }
    if(alive>=2000){
        alive=0;
        close(pad);
        if(ispresent==0){
            transient_present=1;
        }
        ispresent=1;
        pad=open(CONTROL_DEVICE,O_RDONLY|O_NONBLOCK);
        if( access( CONTROL_DEVICE, R_OK ) < 0 ) {
            ispresent=0;
        }
    }
    rounds++;


    if((encendido==1)&&(conectado==1)&&(ispresent==1)&&(rounds>=ROUND_WINDOW)){
       
	    if((send_gas<=GAS_CONFIG)&&(guinnada==0)){
		    config=1;

	    }
	    if(send_gas>GAS_CONFIG){config=0;}
	    if (config==1){
		    send_guinnada=0;
                      // printf("\r\n %i %i ", guinnada,STOP_WITH_YAW);
                        if(guinnada<=STOP_WITH_YAW){
                                query("Y0                         ",mode_send);
				
                        }
                        if(guinnada>=START_WITH_YAW){
				if((rfmon_avail)&&(1==2)){
					random_vector(gcry_key,GCRY_KEYLEN);
					gcry_cipher_setkey (gcry_hd,gcry_key,GCRY_KEYLEN);
    					seq=2;				
					notify_key_to_rfmon(gcry_key);
				}
	                        query("Y1000000                   ",mode_send);
				

			
                        }
			if(hastolog==1){fflush(fl);}
	    }else{
            send_guinnada=guinnada;
        }
	
	
	    sprintf(output, "QQZ%iZ%iZ%iZ%iZ", (int)send_gas/1000,(int)alabeo/1000, (int)cabeceo/1000, (int)send_guinnada/1000);

	    if(hastolog==1){
		fprintf(fl,"%s\n", output );
             }
		
		printf(".");fflush(stdout);
	    query(output, mode_send);
        rounds=0;

    }
    
    //#ifndef MANDO_ABORDO
	usleep(500);
    //#endif

    

    }   
}


