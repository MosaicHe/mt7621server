#include "head.h"
#include "table.h"
#include "command.h"
#include <sys/un.h> 
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/sendfile.h>
//#include "nvram.h"
#include "tool.h"
#include "list.h"

#define HB_TIMEOUT 5

static LISTNODE *listHead;
static client *p_client;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
int modulefds[10];

#define MAX(a, b) (a>b?a:b)
#define MIN(a, b) (a>b?b:a)

void doConfigModule(int fd, client* pclient)
{
	sendData(fd, REQ_CONFIG, NULL, 0);
}

void handleEvent(struct Event* p_Event)
{
	int ret = -1;
	int i;
	client *pclient;
	msg *pmsg = (msg*)p_Event->pdata;
	int fwUpdate;
	int responseState;
	moduleInfo md;
	deb_print("handEvent\n");
	int sock_fd = p_Event->sockfd;

	switch(p_Event->eventType){
		case CLIENT_RECV:
			deb_print("CLIENT_RECV\n");

			//if the client struct is not ceated yet, create one
			if( table_get_by_id(client_table, &pclient, pmsg->id) ){
				pclient = (client*)malloc(sizeof(client));
				pclient->state = STATE_IDLE;
				pclient->addr = p_Event->addr; 
				pclient->activeTime = time(NULL)+TIMEOUT;
				pclient->moduleID = pmsg->id;
			
				table_insert( client_table, pclient);
				//set pclient point to where client_table[i] point to,
				//so, we can change client node by pclient
				table_get_by_id(client_table, &pclient, pmsg->id);
			}
			switch(pclient->state){
				case STATE_IDLE:
					deb_print("STATE_IDLE\n");
					if(pmsg->dataType==REQ_HELLO){
						memcpy(&md, pmsg->dataBuf, pmsg->dataSize);	
						pclient->mdInfo=md;
						ret= sendData( sock_fd, REQ_HELLO, NULL, 0);
						if(ret<0){
							break;		
						}
						pclient->state = STATE_HELLO;
					}else{
						responseState = STATE_IDLE;
						sendData(sock_fd, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_HELLO:
					deb_print("STATE_HELLO,moduleID:%d\n", pclient->moduleID);
					if(pmsg->dataType==REQ_FIRTWARE_UPDATE){
#if 0
						if( !strcmp( pclient->mdInfo.fwVersion, getModulefwVersion() ) ){
#else
						if(0){
#endif
							fwUpdate = 1;
							sendData(sock_fd, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int));
				        	ret = sendFirmware(sock_fd);
							if(ret<0){
								/* FIXME */							
							}
							// Module needs reboot to update firmware, so delete client information
							// from client_table;  wait for Module registering again.
							table_delete(client_table, pclient);								
						}else{
							deb_print(" REQ_FIRTWARE_UPDATE\n");
							fwUpdate = 0;
							sendData(sock_fd, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int) );
						  	pclient->state = STATE_FIRMWARE_CHECKED;
						}				
					}else{
						responseState = STATE_HELLO;
						sendData(sock_fd, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_FIRMWARE_CHECKED:
					if( pmsg->dataType==REQ_CONFIG ){
						doConfigModule(p_Event->sockfd, pclient);
						pclient->state = STATE_CONFIG;
					}else{
						responseState = STATE_FIRMWARE_CHECKED;
						sendData(sock_fd, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_CONFIG:
					if( pmsg->dataType==REQ_RUN ){
						sendData(sock_fd, REQ_RUN, NULL, sizeof(int));
						pclient->state = STATE_RUN;
					}else{
						responseState = STATE_CONFIG;
						sendData(sock_fd, ERROR, &responseState, sizeof(int));
					}
				break;
				case STATE_RUN:
					/* FIXME */
					pthread_mutex_lock(&mutex);
					close(sock_fd);
					for(i=0;i<10;i++)
						if(modulefds[i]==sock_fd)
							modulefds[i]=-1;
					pthread_mutex_unlock(&mutex);
				break;

				default:
				break;
				}
		break;

		case UDP_RECV:
				switch(pmsg->dataType){
				case HEARTBEAT_ACK:
					if( ! table_get_by_id(client_table, &pclient, pmsg->id) && 
										pclient->checkState==RESP_HEAERBEAT ){
						deb_print("receive heartbeatack from module:%d\n",pclient->moduleID);
						pthread_mutex_lock(&mutex);
						pclient->checkState = RESP_IDLE;
						pclient->activeTime = time(NULL);
						pthread_mutex_unlock(&mutex);					
					}
						
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
		if( list_size(listHead)>0){
			pthread_mutex_lock(&mutex);
			list_pop( listHead, &p_ev);
			pthread_mutex_unlock(&mutex); 
			handleEvent(p_ev);
			free(p_ev);	
		}else{
			pthread_mutex_lock(&mutex);
			deb_print(" pthread wait for signal\n");
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


int openListenTcpSocket()
{
	struct sockaddr_in local_addr;
	socklen_t addrlen;
	int server_fd;

	addrlen = sizeof(struct sockaddr);

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

	return server_fd;

}

int openUdpListenSocket()
{
	int sock;
	int ret;
	struct sockaddr_in local_addr;
	socklen_t addrlen;
	addrlen = sizeof(struct sockaddr);
	
	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	if (sock < 0){
		perror("sock error");
		return -1;
	}

	memset((void*) &local_addr, 0, sizeof(struct sockaddr_in));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htons(INADDR_ANY );
	local_addr.sin_port = htons(PORT2);

	ret = bind(sock, (struct sockaddr*)&local_addr, addrlen);
	if (ret < 0){
		perror("bind error");
		return -1;
	}
	return sock;
}


int main(int argc, char**argv)
{
	int ret = -1;
	int i;
	char cmd[256];
	int server_fd,cli_fd, unix_fd, temp_fd;
	int udp_fd;
	pthread_t ptd;
	msg *p_msg;
	struct Event ev;
	long now;
	long timeRemain = HB_TIMEOUT;

	fd_set rdfds;
	int maxfd;

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	struct sockaddr_in peer_addr;
	socklen_t addrlen;
	addrlen = sizeof(struct sockaddr);

	table_init(client_table);
	list_create(&listHead);
	pthread_mutex_init(&mutex, NULL);		
	for(i=0;i<10;i++)
		modulefds[i]=-1;

	//open Unix domain socket for webserver
	unix_fd = openUnixdomainSocket();

	//open TCP listen socket
	server_fd = openListenTcpSocket();
	
	//open UDP listen socket
	udp_fd = openUdpListenSocket();

	ret = pthread_create(&ptd, NULL, workthread, NULL);
	if(ret < 0){
		perror("pthread_create error!\n");
		exit(-1);
	}
	
	while(1){
		// reset rdfds
		deb_print("hello---------->\n");
		FD_ZERO(&rdfds);
		FD_SET(server_fd, &rdfds);
		FD_SET(unix_fd, &rdfds);
		FD_SET(udp_fd, &rdfds);
		maxfd = MAX(server_fd, unix_fd);
		maxfd = MAX(udp_fd, maxfd);
		for(i=0;i<10;i++){
			if( modulefds[i] != -1 ){
				FD_SET( modulefds[i], &rdfds);
				maxfd = MAX(maxfd, modulefds[i]);
			}
		}
		timeRemain = HB_TIMEOUT;
		for(i=1;i<4;i++){
			table_get(client_table, &p_client, i);
			if(p_client != NULL ){
				timeRemain = MIN( now - p_client->activeTime, timeRemain);
			}
		}
		deb_print("timeRemain:%ld\n", timeRemain);	
		tv.tv_sec = timeRemain;
		ret = select( maxfd+1, &rdfds, NULL, NULL, &tv);
		switch(ret){
		case -1:
			perror("select error\n");
			break;
		case 0:
			/*FIXME*/
			now = time(NULL);

			for(i=1;i<4;i++){
				table_get(client_table, &p_client, i);
				if(p_client != NULL && ( now - p_client->activeTime > HB_TIMEOUT) ){
					//send heartbeat data
					sendHeartbeat( p_client );
					p_client->checkState = RESP_HEAERBEAT;
					p_client->activeTime = now;
				}
			}
			break;

		default:
			if( FD_ISSET(server_fd, &rdfds)){
				cli_fd = accept( server_fd, (struct sockaddr*)&peer_addr, &addrlen);
				printf("a client is connecting: ip:%s, port:%d, fd:%d\n",
						  inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port), cli_fd);
				for(i=0;i<10;i++){
					if(modulefds[i] == -1){
						modulefds[i] = cli_fd;
						break;
					}
				}
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
			}else if( FD_ISSET(udp_fd, &rdfds)){
				p_msg = (msg*)malloc(sizeof(msg));
				ret = recvfrom( udp_fd, &p_msg, sizeof(msg), 0,
						           (struct sockaddr*)&peer_addr, &addrlen); 
				if(ret != sizeof(msg)){
					free(p_msg);
					break;
				}
				ev.eventType = UDP_RECV;
				ev.pdata = p_msg;
				ev.sockfd= 0;
				ev.addr  = peer_addr;
				pthread_mutex_lock(&mutex);
				list_append( listHead, ev );
				deb_print("list size:%d\n",list_size(listHead));
				if( list_size(listHead)==1 ){
					deb_print("send singal to pthread wait\n");
					pthread_cond_signal(&empty_cond);
				}
				pthread_mutex_unlock(&mutex);
				break;

			}else{
				for(i=0;i<10;i++){
					if( modulefds[i]!=-1 && FD_ISSET(modulefds[i], &rdfds) ){
						deb_print("fd:%d have recieve data\n",modulefds[i]);
						p_msg = (msg*)malloc(sizeof(msg));
						bzero(p_msg, sizeof(msg));
						
						ret = read( modulefds[i], p_msg, sizeof(msg));
						if(ret<0){
							perror("Socket read error\n");
							break;
						}
						deb_print("read data length:%d\n", ret);
						if(ret==0){
							close(modulefds[i]);
							modulefds[i] = -1;
							break;
						}						
						
						deb_print("module id:%d, dataType:%d, receive data:%d\n",p_msg->id, 
															p_msg->dataType, p_msg->dataSize);
						ev.eventType = CLIENT_RECV;
						ev.pdata = p_msg;
						ev.sockfd= modulefds[i];
						ev.addr  = peer_addr;
			
						pthread_mutex_lock(&mutex);
						list_append( listHead, ev );
						deb_print("list size:%d\n",list_size(listHead));
						if( list_size(listHead)==1 ){
							deb_print("send singal to pthread wait\n");
							pthread_cond_signal(&empty_cond);
						}
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
