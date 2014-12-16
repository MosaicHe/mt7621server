#ifndef __CLIENT_H
#define __CLIENT_H

#include <sys/types.h>     
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

#include "list.h"

#define DEBUG

#ifdef DEBUG 
#define F_OUT	printf("%s(): ",__FUNCTION__);fflush(stdout);
#define deb_print(fmt, arg...)  F_OUT printf(fmt, ##arg);fflush(stdout)
#else
#define F_OUT	
#define deb_print(fmt, arg...)
#endif	//DEBUG

#define MAXMODULE 4
#define TIMEOUT	 5
#define DATASIZE    512
#define PORT 8000
#define PORT2 8002
#define SRV_IP "192.168.111.1"
#define INITIP	"192.168.111.1"
#define N 5

#define CLIENT_IP "192.168.111.2"

#define TMPFILE "modules.tmp"

//unix domain socket
#define UNIX_DOMAIN  "/tmp/UNIX.domain"


// send2server flag
#define RETURN 1

typedef struct{
	unsigned char id;
	unsigned int dataType;
	unsigned int dataSize;
	char dataBuf[DATASIZE];
}msg;

typedef struct{
	unsigned int dataType;
	unsigned int dataSize;
	char dataBuf[DATASIZE];
}UDPMessage;

typedef struct{
	char SN[12];
	char fwVersion[16];

	char state_24g;
	char ssid_24g[36];
	char mac_24g[18];
	int channel_24g;
	
	char state_5g;
	char ssid_5g[36];
	char mac_5g[18];
	int channel_5g;
}moduleInfo;

typedef struct{
	int  moduleID;
	int sock_fd;
	long activeTime;
	int timeoutCounter;
	int state;
	int checkState;
	struct sockaddr_in addr;
	moduleInfo mdInfo;
}client;


typedef struct{
	char nvramDev[6];  // "2860" or "rtdev"
	char item[128];
	char value[128];
}moduleNvramCmd;

 
struct Event{
	char eventType;
	void* pdata;
	int sockfd;
	struct sockaddr_in addr;	
};

#define REQ_HELLO   		1
#define REQ_FIRTWARE_UPDATE 2
#define REQ_CONFIG		    3
#define REQ_RUN 			4

#define ERROR			   -1			


#define STATE_IDLE 				0
#define STATE_HELLO 			1
#define STATE_FIRMWARE_CHECKED 	2
#define STATE_CONFIG 			3
#define STATE_RUN 				4
#define STATE_DISCONNECT		5

#define BROADCAST			100	
#define HEARTBEAT 			101
#define HEARTBEAT_ACK		102
#define COMMAND				103
#define COMMAND_SUCCEED		104
#define COMMAND_FAIL		105

#define RESP_IDLE			0
#define RESP_HEAERBEAT		1

#define CLIENT_RECV 1
#define UDP_RECV 	2

#endif  // __CLIENT_H

