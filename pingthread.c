#include "head.h"
#include "table.h"
#include "tool.h"

#define PING_
#define PING_PORT 8003
#define TIMEOUTLIMIT 5

void *pingThread(void* arg)
{
	int i=0;
	int ret;
	client *pclient;
	msg umsg;
	int udpFd;
	fd_set rdfds;
	struct timeval tv;
	int timeout=0;
	udpFd = socket(AF_INET, SOCK_DGRAM, 0); 
	if (udpFd < 0){
		perror("udpFd error");
		return;
	}

	while(1){
		if( table_size(client_table)==0 )
			sendBroadCast();
		
		for(i=1;i<4;i++){
			table_get(client_table, &pclient, i);
			if(pclient!=NULL){
				pclient->addr.sin_port=htons(PING_PORT);
				umsg.dataType = HEARTBEAT;
				sendto(udpFd, &umsg, sizeof(msg), 0,
						(struct sockaddr*)&(pclient->addr), sizeof(struct sockaddr));
				deb_print("sendto heartbeat to module:%d\n", pclient->moduleID );
				FD_ZERO(&rdfds);
				FD_SET(udpFd, &rdfds);
				tv.tv_sec = 1;
				tv.tv_usec= 0;
				ret = select(udpFd+1, &rdfds, NULL, NULL, &tv);
				if(ret<0){
					return;		
				}else if(ret==0){
					pclient->timeoutCounter++;
					deb_print("module:%d heartbeat lost %d\n", pclient->moduleID, pclient->timeoutCounter);
					if(pclient->timeoutCounter > TIMEOUTLIMIT){
						LOCK_CLIENT_TABLE;
						table_delete(client_table, pclient);
						UNLOCK_CLIENT_TABLE;
					}
				}else{
					ret = recvfrom(udpFd, &umsg, sizeof(msg), 0,
								NULL, NULL);
					if(umsg.dataType == HEARTBEAT_ACK){
						deb_print("recvfrom heartbeat_ack from module:%d\n", pclient->moduleID );
						pclient->timeoutCounter=0;			
					}	
				}		
			}
		}
		sleep(9);
	}
}
