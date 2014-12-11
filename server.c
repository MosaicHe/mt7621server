#include "head.h"
#include "table.h"
#include "command.h"
#include <sys/un.h> 
#include <pthread.h>
#include <signal.h>
#include <sys/sendfile.h>
//#include "nvram.h"
#include "tool.h"
#include "list.h"

static LISTNODE *listHead;
static client *p_client;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;

#define MAX(a, b) (a>b?a:b)

void doConfigModule(client* pclient)
{
	sendData(pclient->moduleID, REQ_CONFIG, NULL, 0);
}

void handleEvent(struct Event* p_Event)
{
	int ret = -1;
	client *pclient = p_Event->pclient;
	msg *pmsg = (msg*)p_Event->pdata;
	int fwUpdate;
	int responseState;
	moduleInfo md;
	deb_print("handEvent\n");
	switch(p_Event->eventType){
		case CLIENT_RECV:
			switch(pclient->state){
				case STATE_IDLE:
					deb_print("STATE_IDLE\n");
					if(pmsg->dataType==REQ_HELLO){
						pclient->moduleID = pmsg->id;
						pclient->state = STATE_HELLO;
						memcpy(&md, pmsg->dataBuf, pmsg->dataSize);	
						pclient->mdInfo=md;
						sendData(pclient->moduleID, STATE_HELLO, NULL, 0);
						if(ret<0){
							/*FIXME*/		
						}
					}else{
						responseState = STATE_IDLE;
						sendData(pclient->moduleID, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_HELLO:
					deb_print("STATE_HELLO\n");
					if(pmsg->dataType==REQ_FIRTWARE_UPDATE){
						if( !strcmp( pclient->mdInfo.fwVersion, getModulefwVersion() ) ){
							fwUpdate = 1;
							sendData(pclient->moduleID, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int));
				        	ret = sendFirmware(pclient->moduleID);
							if(ret<0){
								/* FIXME */							
							}
							// Module needs reboot to update firmware, so delete client information
							// from client_table;  wait for Module registering again.
							table_delete(client_table, pclient);								
						}else{
							fwUpdate = 0;
							sendData(pclient->moduleID, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int) );
						  	pclient->state = STATE_FIRMWARE_CHECKED;
						}				
					}else{
						responseState = STATE_HELLO;
						sendData(pclient->moduleID, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_FIRMWARE_CHECKED:
					if( pmsg->dataType==REQ_CONFIG ){
						doConfigModule(pclient);
						pclient->state = STATE_CONFIG;
					}else{
						responseState = STATE_FIRMWARE_CHECKED;
						sendData(pclient->moduleID, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_CONFIG:
					if( pmsg->dataType==REQ_RUN ){
						sendData(pclient->moduleID, REQ_RUN, NULL, sizeof(int));
						pclient->state = STATE_RUN;
					}else{
						responseState = STATE_CONFIG;
						sendData(pclient->moduleID, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_RUN:
					/* FIXME */
				break;

				default:
				break;
			}
		break;

		default:
		break;	
	}
}


void* workthread(void* arg)
{
	struct Event *p_ev;

	sendBroadCast();
	while(1){
		deb_print("************\n");
		if( list_size(listHead)>0){
			pthread_mutex_lock(&mutex);
			list_get(listHead, p_ev);
			pthread_mutex_unlock(&mutex); 
			handleEvent(p_ev);
			free(p_ev);	
		}else{
			pthread_mutex_lock(&mutex);
			pthread_cond_wait(&empty_cond, &mutex);
			pthread_mutex_unlock(&mutex);
		}
	}
}


int doWebWork(int fd)
{
	int dataType;
	char buf[128];
	int buflen;
//	int finished = 0;
	int responseType;
	int id;
	
	while(1){
		recvData(fd, &id, &dataType, buf, &buflen, 0);
		if( dataType == CLOSE )
			return 0;
		else{
			responseType = doCommand(dataType, buf, &buflen);
			sendData(fd, responseType, buf, buflen);
		}
	}
}


/*
 * function: open a unix domain socket for webserver
 */
int openUnixdomainSocket()
{
//	socklen_t clt_addr_len;
    int listen_fd;
    int ret;
    struct sockaddr_un srv_addr;
    listen_fd=socket(PF_UNIX, SOCK_STREAM, 0);
    if(listen_fd<0)
    {
        perror("cannot create communication socket");
        return;
    }
    //set server addr_param
    srv_addr.sun_family=AF_UNIX;
    strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);
    unlink(UNIX_DOMAIN);
    //bind sockfd & addr
    ret=bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if(ret==-1)
    {
        perror("cannot bind server socket");
        close(listen_fd);
        unlink(UNIX_DOMAIN);
        return;
    }
    //listen sockfd 
	deb_print(" before listen");
    ret=listen(listen_fd,1);
    if(ret==-1)
    {
        perror("cannot listen the client connect request");
        close(listen_fd);
        unlink(UNIX_DOMAIN);
        return;
    }
	return listen_fd;
}

int main(int argc, char**argv)
{
	int ret = -1;
	int i;
	char cmd[256];
	int server_fd,cli_fd, unix_fd, temp_fd;
	pthread_t ptd;
	msg *p_msg;
	struct Event ev;

	fd_set rdfds;
	int maxfd;

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	struct sockaddr_in peer_addr;
	struct sockaddr_in local_addr;
	socklen_t addrlen;
	addrlen = sizeof(struct sockaddr);
	
	//init mutex
	pthread_mutex_init (&mutex, NULL);	
	table_init(client_table);
	listHead = list_create();

	unix_fd = openUnixdomainSocket();

	// listen socket
	server_fd = socket(PF_INET,SOCK_STREAM,0);
	if(server_fd < 0)
	{
		perror("socket");
		exit(1);
	}
	bzero(&local_addr ,sizeof(local_addr));
	local_addr.sin_family = PF_INET;
	local_addr.sin_port = htons(PORT); 
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if( bind(server_fd, (struct sockaddr*)&local_addr, addrlen) < 0 )
	{
		perror("bind");
		exit(1);
	}

	if( listen(server_fd,N) < 0)
	{
		perror("listen");
		exit(1);
	}
		
	ret = pthread_create(&ptd, NULL, workthread, NULL);
	if(ret < 0){
		perror("pthread_create error!\n");
		exit(-1);
	}
	
	while(1){
		// reset rdfds
		FD_ZERO(&rdfds);
		FD_SET(server_fd, &rdfds);
		FD_SET(unix_fd, &rdfds);
		maxfd = MAX(server_fd, unix_fd);
		for(i=1;i<4;i++){
			table_get(client_table, p_client, i);
			if(p_client != NULL){
				FD_SET(p_client->sock_fd, &rdfds);
				maxfd = MAX(maxfd, p_client->sock_fd);				
			}
		}

		deb_print("******select*******\n");
		ret = select( maxfd+1, &rdfds, NULL, NULL, &tv);
		switch(ret){
		case -1:
			perror("select error");
			exit(1);
		case 0:
			/*FIXME*/		
			break;

		default:
			if( FD_ISSET(server_fd, &rdfds)){
				cli_fd = accept( server_fd, (struct sockaddr*)&peer_addr, &addrlen);
				deb_print("a client is connecting: ip:%s, port:%d\n",
							inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
				p_client = (client*)malloc(sizeof(client));
				p_client->sock_fd = cli_fd;
				p_client->state = STATE_IDLE;
				p_client->addr = peer_addr;
				table_insert(client_table, p_client);
				break;

			}else if( FD_ISSET(unix_fd, &rdfds)){
				temp_fd=accept(unix_fd, NULL, NULL);
				if(temp_fd<0){
					perror("cannot accept client connect request");
					close( temp_fd);
					unlink(UNIX_DOMAIN);
					break;
				}
				doWebWork(temp_fd);
				close(temp_fd);
				break;
			}else{
				for(i=1;i<4;i++){
					table_get(client_table, p_client, i);
					if( p_client!=NULL && FD_ISSET(p_client->sock_fd, &rdfds) ){
						deb_print("fd:%d have recieve data\n",p_client->sock_fd);
						p_msg = (msg*)malloc(sizeof(msg));
						bzero(p_msg, sizeof(msg));
						
						ret = read( p_client->sock_fd, p_msg, sizeof(msg));
						if(ret<0){
							perror("Socket ead error\n");
							break;
						}
						deb_print("receive data:%d\n",p_msg->dataSize);

						ev.eventType = CLIENT_RECV;
						ev.pdata = p_msg;
						ev.pclient= p_client;
			
						pthread_mutex_lock(&mutex);
						list_insert( listHead, ev );
						if( list_size(listHead)==1 )
							pthread_cond_signal(&empty_cond);
						pthread_mutex_unlock(&mutex);
					}
				}
				break;		
			}		
		}
	}

	close(unix_fd);
	close(server_fd);
	unlink(UNIX_DOMAIN);
	return 0;
}
