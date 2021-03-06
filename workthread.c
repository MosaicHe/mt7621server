#include "head.h"
#include "tool.h"
#include "table.h"

#define READ_TIMEOUT 1

static msg *pmsg;


void doConfigModule(int fd, client* pclient)
{
	sendData(fd, REQ_CONFIG, NULL, 0);
}

int handleModuleRegister(int moduleFd, struct sockaddr_in addr)
{
	client *pclient;
	struct timeval tv;
	int ret;
	int fwUpdate;

	LOCK_CLIENT_TABLE;
	//delete pclient when a new register comming;
	if( table_get_by_id(client_table, &pclient, pmsg->id)!=NULL ){
		table_delete(client_table, &pclient);
	}

	pclient = (client*)malloc(sizeof(client));
	pclient->state = STATE_IDLE;
	pclient->addr = addr; 
	pclient->activeTime = time(NULL);
	pclient->moduleID = pmsg->id;
	pclient->timeoutCounter=0;
	memcpy(&pclient->mdInfo, pmsg->dataBuf, sizeof(moduleInfo));
	table_insert( client_table, pclient);
	deb_print("24G state:%d,5G state:%d\n",pclient->mdInfo.state_24g, pclient->mdInfo.state_5g);
	printModuleInfo( &(pclient->mdInfo));

	//set pclient point to where client_table[i] point to,
	//so we can change client node by pclient
	table_get_by_id(client_table, &pclient, pmsg->id);
	
	memcpy(&pclient->mdInfo, pmsg->dataBuf, sizeof(moduleInfo));
	pclient->state = STATE_HELLO;
	UNLOCK_CLIENT_TABLE;
	
	writeModuleInfo2Nvram(pclient->moduleID, &pclient->mdInfo );

	ret= sendData( moduleFd, RESP_SUCCESS, NULL, 0);
	if(ret<0){
		LOCK_CLIENT_TABLE;
		pclient->state = STATE_IDLE;
		UNLOCK_CLIENT_TABLE;
		return -1;	
	}
	while(1){
		tv.tv_sec = 1;
		ret = recvData(moduleFd, pmsg, &tv);
		if(ret!=sizeof(msg)){
			printf("recvData error\n");
			LOCK_CLIENT_TABLE;
			pclient->state = STATE_IDLE;
			UNLOCK_CLIENT_TABLE;
			return -1;
		}

		switch(pclient->state){
			case STATE_HELLO:
				deb_print("STATE_HELLO,moduleID:%d\n", pclient->moduleID);
				if(pmsg->dataType==REQ_FIRTWARE_UPDATE){
	#if 0
					if( !strcmp( pclient->mdInfo.fwVersion, getModulefwVersion() ) ){
	#else
					if(0){
	#endif
						fwUpdate = 1;
						sendData(moduleFd, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int));

				    	ret = sendFirmware(moduleFd);
						if(ret<0){
							/* FIXME */
							pclient->state = STATE_IDLE;
							return -1;							
						}

						// Module needs reboot to update firmware, so delete client information
						// from client_table;  wait for Module registering again.
						LOCK_CLIENT_TABLE;
						table_delete(client_table, pclient);
						UNLOCK_CLIENT_TABLE;								
					}else{
						deb_print(" REQ_FIRTWARE_UPDATE\n");
						fwUpdate = 0;
						sendData(moduleFd, REQ_FIRTWARE_UPDATE, &fwUpdate, sizeof(int) );
					  	pclient->state = STATE_FIRMWARE_CHECKED;
						break;
					}				
				}else{
					pclient->state = STATE_IDLE;
					sendData(moduleFd, RESP_ERROR, NULL, 0);
					return -1;
				}
				break;
			case STATE_FIRMWARE_CHECKED:
				if( pmsg->dataType==REQ_CONFIG ){
					doConfigModule(moduleFd, pclient);
					pclient->state = STATE_CONFIG;
				}else{
					sendData(moduleFd, RESP_ERROR, NULL, 0);
					pclient->state = STATE_IDLE;
					return -1;
				}
				break;
			case STATE_CONFIG:
				if( pmsg->dataType==REQ_RUN ){
					sendData(moduleFd, RESP_SUCCESS, NULL, 0);
					pclient->state = STATE_RUN;
					return 0;
				}else{
					sendData(moduleFd, RESP_ERROR, NULL, 0);
					pclient->state = STATE_IDLE;
					return -1;
				}
				break;
			default:
				break;
		}
	}
	return 0;
}



void handleModuleReport()
{

}

#if 0
void* workThread(void* arg)
{
	fd_set rdfds;
	struct timeval tv;
	int count;
	int ret;
	printf("workThread\n");
	moduleFd = ((struct moduleSocketInfo*)arg)->fd;
	addr = ((struct moduleSocketInfo*)arg)->addr;
	free(arg);

	pmsg = (msg*)malloc(sizeof(msg));
	tv.tv_sec = READ_TIMEOUT;
	tv.tv_usec = 0;
	ret = recvData(moduleFd, pmsg, &tv);
	if(ret<0||ret==0){
		printf("read socket error\n");
	}
	printf("%d**********\n",pmsg->dataType);
	switch(pmsg->dataType){
		case REQ_REGISTER:
			handleModuleRegister();
			break;
		case REQ_REPORT:
			handleModuleReport();
			break;
		default:
			break;
	}
	free(pmsg);
	close(moduleFd);
}
#endif

void tcpwork(void* arg)
{
	fd_set rdfds;
	struct timeval tv;
	int count;
	int ret;

	int moduleFd;
	static struct sockaddr_in addr;
	printf("workThread\n");
	moduleFd = ((struct moduleSocketInfo*)arg)->fd;
	addr = ((struct moduleSocketInfo*)arg)->addr;
	free(arg);
	deb_print("moduleFd:%d\n", moduleFd);
	pmsg = (msg*)malloc(sizeof(msg));
	tv.tv_sec = READ_TIMEOUT;
	tv.tv_usec = 0;
	ret = recvData(moduleFd, pmsg, &tv);
	if(ret<0||ret==0){
		printf("read socket error\n");
	}
	printf("%d**********\n",pmsg->dataType);
	switch(pmsg->dataType){
		case REQ_REGISTER:
			handleModuleRegister(moduleFd, addr);
			break;
		case REQ_REPORT:
			handleModuleReport();
			break;
		default:
			break;
	}
	free(pmsg);
	close(moduleFd);
}



