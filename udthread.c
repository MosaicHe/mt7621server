#include "head.h"
#include "tool.h"
#include "table.h"

void* udThread( void* arg)
{
	deb_print("udThread\n");
	int udFd=*(int*)arg;
	int ret=-1;
	msg umsg;
	client* pclient;
	int moduleFd;
	struct sockaddr_in moduleAddr;
	struct timeval tv;

	ret=read(udFd, &umsg, sizeof(umsg));
	if(ret!=sizeof(msg))
		return;
	table_get_by_id(client_table, &pclient, umsg.id);
	if(pclient==NULL){
		strcpy(umsg.dataBuf, "Error:module ID doesn't exist!\n");
		umsg.dataSize= strlen(umsg.dataBuf)+1;
		ret=write(udFd, &umsg, sizeof(umsg));
	}else{
		moduleFd = socket(PF_INET,SOCK_STREAM,0);
		if(moduleFd < 0){
			perror("can't open socket\n");
			exit(1);
		}
		moduleAddr =pclient->addr;
		moduleAddr.sin_port = htons(MODULE_PORT);
		ret = connect(moduleFd, (struct sockaddr*)&moduleAddr, sizeof(struct sockaddr));
		if(ret < 0){
			printf("can not connect to module\n");
			strcpy(umsg.dataBuf, "Error:can not connect to module!\n");
			umsg.dataSize= strlen(umsg.dataBuf)+1;
			ret=write(udFd, &umsg, sizeof(umsg));
		}
		ret=write(moduleFd, &umsg, sizeof(msg));
		tv.tv_sec=1;
		tv.tv_usec=0;		
		ret = recvData(moduleFd, &umsg, &tv);
		write(udFd, &umsg, sizeof(umsg));
	}
}
