#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <gcrypt.h>


#define DEFAULT_PCAP_DEV "wlan0"
#define GCRY_KEYLEN 16
#define GCRY_IVLEN 16
#define CHECKSUM_LEN 2
#define SEQ_LEN 4
#define GCRY_PAYLOAD 64
#define COMMAND_LEN GCRY_PAYLOAD-CHECKSUM_LEN-SEQ_LEN
#define MAC_LEN 6
#define IP_IMU  "192.168.251.70"
#define PORT_IMU 5555

#define KEY_FILE "/tmp/key_rfmon"

unsigned long seq=1;
unsigned long lastseq=1;
gcry_cipher_hd_t  gcry_hd;
int sock;
struct sockaddr_in server;
char gcry_key[GCRY_KEYLEN]={0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x01};

void print_vector(char * vector, int len){
        int ac=0;
        printf("\r\n");
        for(ac=0;ac<len;ac++){
                vector[ac]=vector[ac]&0x0000FF;
 //               if(vector[ac]>21){
 //                   printf(" %c", (char)vector[ac]);
 //               }else{
                    printf(" %x",vector[ac]&0x0000FF);
 //               }
        }
        printf("\r\n");
}

static void signal_key(int signo){
	if (signo== SIGUSR2){
        FILE *fp;
        char tkey[GCRY_KEYLEN];   
        fp = fopen(KEY_FILE, "rb");
        int readed=fread(tkey,1,GCRY_KEYLEN,fp);
        if(readed==GCRY_KEYLEN){
            memcpy(gcry_key,tkey,GCRY_KEYLEN);
            system("echo @OK# > /tmp/key_changed");
            printf("\r\n OK RECEIVING KEY");
            print_vector(gcry_key,GCRY_KEYLEN);
            gcry_cipher_setkey (gcry_hd,gcry_key,GCRY_KEYLEN);
            seq=1;
            lastseq=1;
        }else{

	        system("echo @ERROR# > /tmp/key_changed");
	        printf("\r\n ERROR LEN RECEIVED KEY");
        }
	}else{
		system("echo @ERROR# > /tmp/key_changed");
		printf("\r\n ERROR SIGNAL NOT ALLOWED");
	}
}

int openSocket( const char device[IFNAMSIZ] )
{
	struct ifreq ifr;
	struct sockaddr_ll ll;
	const int protocol = ETH_P_ALL;
	int sock = -1;
	
	//assert( sizeof( ifr.ifr_name ) == IFNAMSIZ );

	sock = socket( PF_PACKET, SOCK_RAW, htons(protocol) );
	if ( sock < 0 )
	{
		perror( "socket failed (do you have root priviledges?)" );
		return -1;
	}
	
	memset( &ifr, 0, sizeof( ifr ) );
	strncpy( ifr.ifr_name, device, sizeof(ifr.ifr_name) );
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
	{
		perror("ioctl[SIOCGIFINDEX]");
		close(sock);
		return -1;
	}

	memset( &ll, 0, sizeof(ll) );
	ll.sll_family = AF_PACKET;
	ll.sll_ifindex = ifr.ifr_ifindex;
	ll.sll_protocol = htons(protocol);
	if ( bind( sock, (struct sockaddr *) &ll, sizeof(ll) ) < 0 ) {
		perror( "bind[AF_PACKET]" );
		close( sock );
		return -1;
	}
		
	// Enable promiscuous mode
/*	struct packet_mreq mr;
	memset( &mr, 0, sizeof( mr ) );
	
	mr.mr_ifindex = ll.sll_ifindex;
	mr.mr_type    = PACKET_MR_PROMISC;

	if( setsockopt( sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof( mr ) ) < 0 )
	{
		perror( "setsockopt[PACKET_MR_PROMISC]" );
		close( sock );
		return -1;
	}
	*/
	return sock;
}


void packet_handler(const u_char *pkt_data, int caplen){
	char command_o[100];
	int commandlen=0;
    
    int identified = 0;
    int ac=0;

    for ( ac =0; ac< caplen; ac++){
        if (pkt_data[ac] == 'I' && pkt_data[ac+1] == 'N' && pkt_data[ac+2] == 'T' && pkt_data[ac+3] == 'C' && pkt_data[ac+4] == 'P'){
            identified = 1;	
            break;        
        }
    }

    if(identified){
	    if(decode_packet(&gcry_hd,&pkt_data[54+ac],caplen-(54+ac)-10,command_o, &commandlen,&seq)==0){
		//printf("\r\n SENT %s (%i) ->%i \t",command_o, strlen(command_o),seq);
            //fflush(stdout);
            sendto(sock,command_o,strlen(command_o),0,(const struct sockaddr *)&server,sizeof(struct sockaddr_in));
     		lastseq=seq;
        }
        
    }

}
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

void random_vector(char * arr, int size){
	gcry_randomize (arr, size,GCRY_STRONG_RANDOM);
}



u_int checksum(char * command, int len){


        len-=CHECKSUM_LEN;

        int ac=0;
        u_int off=8;
        u_long t=0;
        u_int c=0;
        for(ac=0;ac<len;ac++){
                t=t+((((u_int)command[ac])<<off));
                if(off==8){off=0;}else{off=8;}
        }
        /*do{
                c=(t>>16);
                t=c+(t&0x00000000FFFF);
        }while(c!=0x00);
	*/
        //t=(~t);
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


int decode_packet(gcry_cipher_hd_t * hd,  char * packet, int len, char *command, int *commandlen, unsigned long * seqnum){
	
	u_char chk_carried[CHECKSUM_LEN];
	u_int chk_calculated;

	memcpy(chk_carried,&packet[len-CHECKSUM_LEN],CHECKSUM_LEN);

    //printf("\r\n CHK_CALC %x %x %x",checksum(packet,len),(((chk_carried[0]<<8)&0x00FF00)|(chk_carried[1]&0x0000FF)),checksum(packet,len)-(((chk_carried[0]<<8)&0x00FF00)|(chk_carried[1]&0x0000FF)));
	
	if(checksum(packet,len)!=(((chk_carried[0]<<8)&0x00FF00)|(chk_carried[1]&0x0000FF))){
		printf("\r\n BAD CHECKSUM");
    	return -1;
	}


    gcry_cipher_setiv(*hd,&packet[MAC_LEN], GCRY_IVLEN);

	*commandlen=len-MAC_LEN-GCRY_IVLEN-CHECKSUM_LEN;
    gcry_cipher_decrypt (*hd,command,*commandlen ,&packet[MAC_LEN+GCRY_IVLEN], *commandlen);

	int ac=0;
	unsigned long cseqnum=0;
	for(ac=0;ac<SEQ_LEN;ac++){
		cseqnum|=((command[(*commandlen-1-ac)]&0x000000FF)<<(ac*8));
	}
	
	if(*seqnum>=cseqnum){
		return -2;
	}
	*seqnum=cseqnum;
	*commandlen-=SEQ_LEN;

	return 0;
}

int main(int argc, char *argv[])
{
	int result;

	int pcap_available=0;
	char dev_name[20];

	signal(SIGUSR2, signal_key);

	if(argc>1){
		strcpy(dev_name,argv[1]);
	}else{
		strcpy(dev_name,DEFAULT_PCAP_DEV);
	}
	init_gcrypt(&gcry_hd);


    struct hostent *hp;

    sock= socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;

    server.sin_port = htons(PORT_IMU);
    hp = gethostbyname(IP_IMU);

    if (hp==0) printf("IMU IP no alcanzable");
    bcopy((char *)hp->h_addr, (char *)&server.sin_addr,hp->h_length);



    const u_char pkt_data[4096];
	u_char *param;

	int rawSocket = openSocket( dev_name );
	if ( rawSocket < 0 )
	{
		fprintf( stderr, "error opening socket\n" );
		return 1;
	}
    gcry_cipher_setkey (gcry_hd,gcry_key,GCRY_KEYLEN);
    printf("\r\nINIT INTERCEPTOR");
    fflush(stdout);
   
    
	while(1){
		fd_set readfds;
		FD_ZERO( &readfds );
		FD_SET( rawSocket, &readfds );        
        int numFds = select( rawSocket+1, &readfds, NULL, NULL, NULL );

		if ( numFds == 1 )
		{
			int caplen = read( rawSocket,  pkt_data, sizeof( pkt_data) );    
		    if(pkt_data!=NULL){
                    packet_handler(pkt_data,caplen);
		    }
        }
	}	
    printf("\r\n EXITED");
	
	return(0);
}
