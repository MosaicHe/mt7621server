#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/select.h>
#include <sys/times.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <assert.h> 
#include <sys/un.h> 

#define UNIX_DOMAIN "./UNIX.domain"
#define DATASIZE 512
typedef struct{
	unsigned char id;
	unsigned int dataType;
	unsigned int dataSize;
	char dataBuf[DATASIZE];
}msg;

typedef struct{
	char nvramDev[8];  // "rt2860" or "rtdev"
	char item[128];
	char value[128];
}moduleNvram;

#define NVRAM_SET			106
#define NVRAM_GET			107

void showUsge()
{
	printf("moduleset Usge:\n");
	printf("\t moduleset $moduleId [$nvramDev] $item $value\n");
	printf("\t Example: moduleset 1 rt2860 SSID1 OPENAP\n");
}

int main(int argc, char** argv )
{
    int connect_fd;
    int ret;
    char snd_buf[1024];
	int cmd;
	int len;
	int moduleId;
	moduleNvram mn;
	msg umsg;
    struct sockaddr_un srv_addr;
    	
	if( argc!=4 && argc!=5){
		showUsge();
		return 0;
	}
	
	moduleId = atoi(argv[1]);
	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	if( argc==5){
		strcpy(mn.nvramDev,	argv[2]);
		strcpy(mn.item, argv[3]);
		strcpy(mn.value, argv[4]);
	}	
	else{
		strcpy(mn.nvramDev, "rt2860");
		strcpy(mn.item, argv[2]);
		strcpy(mn.value, argv[3]);
	}
    //creat unix socket
    connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
    if(connect_fd<0)
    {
        perror("cannot create communication socket");
        return -1;
    }   
    srv_addr.sun_family=AF_UNIX;
    strcpy(srv_addr.sun_path, UNIX_DOMAIN);
    
    //connect server
    ret=connect(connect_fd,(struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if(ret<0)
    {
        perror("cannot connect to the server");
        close(connect_fd);
        return -1;
    }
 	printf("connect to server\n");
    //send info server
	umsg.id = moduleId;
	umsg.dataSize = sizeof(moduleNvram);
	umsg.dataType = NVRAM_SET;
	memcpy(umsg.dataBuf, &mn, sizeof(moduleNvram));

    write( connect_fd, &umsg, sizeof(msg));
	
	read(connect_fd, &umsg, sizeof(msg));
    printf("%s\n", umsg.dataBuf);

    return -1;
}
