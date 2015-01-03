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

#define MODULE_PORT 8002


#define UNIX_DOMAIN "/UNIX.domain"
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

typedef struct{
	int moduleId;
	int requst;
	char data[128];
}Request;

#define REQ_MODULE_EXIST	1
#define REQ_IF_EXIST		2
#define REQ_MAC				3
#define REQ_IP				4

#define NVRAM_SET			106
#define NVRAM_GET			107
#define NVRAM_SET_COMMIT	111
#define INIT_INTERNET		108
#define SET_STALIMIT		109
#define GET_MACLIST			110
#define GET_MACNUM			112
#define SYSTEM_CMD			113

int connect2Server( )
{
    int connect_fd;
    int ret;
    struct sockaddr_un srv_addr;

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

   	return connect_fd;
}


int connect2Module(int moduleId)
{
	int moduleFd;
	struct sockaddr_in moduleAddr;

	char ipaddr[28];
	int ret;	
	
	ret = getModuleIp(moduleId, ipaddr);	
	if(ret<0)
		return -1;

	moduleFd = socket(PF_INET,SOCK_STREAM,0);
	if(moduleFd < 0){
		perror("can't open socket\n");
		exit(1);
	}
	moduleAddr.sin_family=AF_INET;
	moduleAddr.sin_addr.s_addr = inet_addr(ipaddr);
	moduleAddr.sin_port = htons(MODULE_PORT);

	ret = connect(moduleFd, (struct sockaddr*)&moduleAddr, sizeof(struct sockaddr));
	if(ret < 0){
		printf("can not connect to module:\n");
		close(moduleFd);
		return -1;
	}
	printf("connect to module\n");
	return moduleFd;
}


int module_get(int moduleId, char *nvramDev, char* item, char* value)
{
	int connect_fd;
	moduleNvram mn;
	msg umsg;
	struct timeval tv;
	int ret =-1;
	fd_set rdfds;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Module(moduleId);
	if(connect_fd<0){
		return -1;
	}
	//send info server
	if( strcmp(nvramDev, "rtdev") && strcmp(nvramDev, "rt2860") )
		return -1;
	
	memcpy(mn.nvramDev, nvramDev, strlen(nvramDev)+1);
	memcpy(mn.item, item, strlen(item)+1);
	umsg.id = moduleId;
	umsg.dataSize = sizeof(moduleNvram);
	umsg.dataType = NVRAM_GET;
	memcpy(umsg.dataBuf, &mn, sizeof(moduleNvram));
    ret = write( connect_fd, &umsg, sizeof(msg));
	if(ret!= sizeof(msg)){
		close(connect_fd);
		return -1;
	}
	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		close(connect_fd);
		return -1;
	}else{
		read(connect_fd, &umsg, sizeof(msg));
		memcpy(value, umsg.dataBuf, strlen(umsg.dataBuf));
	}
	close(connect_fd);
    return 0;
}

int module_set(int moduleId, char *nvramDev, char* item, char* value)
{
	int connect_fd;
	moduleNvram mn;
	msg umsg;
	struct timeval tv;
	int ret =-1;
	fd_set rdfds;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Module(moduleId);
	if(connect_fd<0){
		return -1;
	}
	if( strcmp(nvramDev, "rtdev") && strcmp(nvramDev, "rt2860") )
		return -1;

	memcpy(mn.nvramDev, nvramDev, strlen(nvramDev)+1);
	memcpy(mn.item, item, strlen(item)+1);
	memcpy(mn.value, value, strlen(value)+1);
	umsg.id = moduleId;
	umsg.dataSize = sizeof(moduleNvram);
	umsg.dataType = NVRAM_SET;
	memcpy(umsg.dataBuf, &mn, sizeof(moduleNvram));
    ret = write( connect_fd, &umsg, sizeof(msg));
	if(ret!= sizeof(msg)){
		return -1;
	}

	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		close(connect_fd);
		return -1;
	}else{
		read(connect_fd, &umsg, sizeof(msg));
		memcpy(value, umsg.dataBuf, strlen(umsg.dataBuf)+1);
	}
	close(connect_fd);
    return 0;
}

int moduleSystem(int moduleId, char* s)
{
	int connect_fd;
	msg umsg;
	int ret =-1;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Module(moduleId);
	if(connect_fd<0){
		return -1;
	}
	printf("%s\n", s);
	bzero(&umsg, sizeof(msg));
	umsg.id = moduleId;
	umsg.dataType = SYSTEM_CMD;
	memcpy(umsg.dataBuf, s, strlen(s)+1);
	umsg.dataSize=strlen(umsg.dataBuf);

	ret = write( connect_fd, &umsg, sizeof(msg));
	if(ret!= sizeof(msg)){
		close(connect_fd);
		return -1;
	}
	return 0;
}

int module_set_commit(int moduleId, char *nvramDev)
{
	int connect_fd;
	moduleNvram mn;
	msg umsg;
	struct timeval tv;
	int ret =-1;
	fd_set rdfds;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	if( strcmp(nvramDev, "rtdev") && strcmp(nvramDev, "rt2860") ){
		return -1;
	}

	connect_fd = connect2Module(moduleId);
	if(connect_fd<0){
		return -1;
	}
	strcpy(mn.nvramDev, nvramDev);
	umsg.id = moduleId;
	umsg.dataSize = sizeof(moduleNvram);
	umsg.dataType = NVRAM_SET_COMMIT;
	memcpy(umsg.dataBuf, &mn, sizeof(moduleNvram));
    ret = write( connect_fd, &umsg, sizeof(msg));
	if(ret!= sizeof(msg)){
		return -1;
	}
	close(connect_fd);
    return 0;
}


int reloadInernet(int moduleId)
{
	int connect_fd;
	msg umsg;
	int ret;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Module(moduleId);
	if(connect_fd<0){
		return -1;
	}
	
	umsg.dataType = INIT_INTERNET;
    ret = write( connect_fd, &umsg, sizeof(msg));
	if(ret!= sizeof(msg)){
		return -1;
	}
	close(connect_fd);
    return 0;
}

int getModuleExist(int moduleId)
{
	int ret;
	int connect_fd;
	fd_set rdfds;
	int val;
	struct timeval tv;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Server();
	if(connect_fd<0){
		return -1;
	}
	Request r;
	r.moduleId =moduleId;
	r.requst = REQ_MODULE_EXIST;
	ret = write( connect_fd, &r, sizeof(Request));
	if(ret!= sizeof(Request))
		return -1;

	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		return -1;
	}else{
		bzero(&r, sizeof(Request));
		read(connect_fd, &r, sizeof(r));
	}
	close(connect_fd);
	printf("state:%s\n", r.data);
	if( !strcmp(r.data, "1") )
		return 1;
	else
		return 0;
}

int getModuleIfExist(int moduleId, char* ifname)
{
	int ret;
	int connect_fd;
	fd_set rdfds;
	int val;
	struct timeval tv;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Server();
	if(connect_fd<0){
		return -1;
	}
	Request r;
	r.moduleId =moduleId;
	r.requst = REQ_IF_EXIST;
	strcpy(r.data, ifname);
	printf("ifname:%s\n", ifname);
	ret = write( connect_fd, &r, sizeof(Request));
	if(ret!= sizeof(Request))
		return -1;

	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		return -1;
	}else{
		read(connect_fd, &r, sizeof(r));
	}
	close(connect_fd);
	printf("state:%s\n",r.data);
	if(!strcmp(r.data, "1"))
		return 1;
	else
		return 0;
}

int getModuleMac(int moduleId, char* ifname, char* mac)
{
	int ret;
	int connect_fd;
	fd_set rdfds;
	struct timeval tv;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Server();
	if(connect_fd<0){
		return -1;
	}
	Request r;
	r.moduleId =moduleId;
	r.requst = REQ_MAC;
	strcpy(r.data, ifname);
	
	ret = write( connect_fd, &r, sizeof(Request));
	if(ret!= sizeof(Request))
		return -1;

	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		return -1;
	}else{
		bzero(&r, sizeof(Request));
		read(connect_fd, &r, sizeof(r));
		memcpy( mac, r.data, strlen(r.data)+1);
		
	}
	close(connect_fd);
	return 0;
}

int getModuleIp(int moduleId, char* ip)
{
	int ret;
	int connect_fd;
	fd_set rdfds;
	struct timeval tv;
	char* value;

	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
	connect_fd = connect2Server();
	if(connect_fd<0){
		return -1;
	}
	Request r;
	r.moduleId =moduleId;
	r.requst = REQ_IP;
	
	ret = write( connect_fd, &r, sizeof(Request));
	if(ret!= sizeof(Request))
		return -1;

	FD_ZERO(&rdfds);
	FD_SET(connect_fd, &rdfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
	ret = select(connect_fd+1,&rdfds, NULL, NULL, &tv);
	if(ret<0||ret==0){
		printf("timeout\n");
		return -1;
	}else{
		bzero(&r, sizeof(Request));
		read(connect_fd, &r, sizeof(r));
		memcpy( ip, r.data, strlen(r.data)+1);
	}
	close(connect_fd);
	return 0;
}

