#include <stdio.h>             
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <errno.h>

//#include "ralink_gpio.h"

#include "head.h"

#define FWNAME "/sda1/firmware/root_uImage"


#define IP_FOUND "server_broadcast"
#define IP_FOUND_ACK "server_broadcast_ack"
#define IFNAME "eth7"
#define MCAST_PORT 9999


#define GPIO_DEV	"/dev/bgpio"
#define GET_MODULES	  1
#define GET_IDS		  2
#define CTL_ID1		  3
#define CTL_ID2		  4
#define CTL_ID3		  5
#define CTL_ID4		  6

enum {
	gpio_in,
	gpio_out,
};

extern int testModuleOn()
{
	int fd, req;
	int status = -1;
	fd = open(GPIO_DEV, O_RDONLY);
	if (fd < 0) {
		perror(GPIO_DEV);
		return -1;
	}
	
	ioctl(fd, GET_MODULES, &status);
	printf("modues status: 0x%x\n", status);
	
	close(fd);
	return status;

}

extern int getModuleState( int num )
{
	int fd, req;
	int mask = 1 << (num-1);
	int status = -1;
	fd = open(GPIO_DEV, O_RDONLY);
	if (fd < 0) {
		perror(GPIO_DEV);
		return -1;
	}
	
	ioctl(fd, GET_IDS, &status);
	printf("modules: 0x%x\n", status);
	close(fd);
	status = (~status) & mask;
	
	return status;
}

extern int resetModule(int id, int val)
{
	int fd, req;
	int status = -1;
	int cmd;
	if(id < 0 || id >4){
		perror("error: id should be 1-3\n");
	}
		
	fd = open(GPIO_DEV, O_RDONLY);
	if (fd < 0) {
		perror(GPIO_DEV);
		return -1;
	}
	status = val;
	ioctl(fd, id+2, &status);
	
	close(fd);
	return 0;
}

extern int getBit(char val1, char val2){
	int i=0;
	int bit = -1;
	for(i=0; i<4; i++){
		if( (val1 ^ val2) >> i == 1){
			bit = i;
			return bit;
		}
	}
	return bit;
}


/*
 * function:send data to socket
 */
extern int sendData(int fd, int dataType, void* pbuf, int buflen)
{
	int ret  = -1;
	msg *p_responseBuf;
	p_responseBuf = (msg*)malloc(sizeof(msg));
	bzero( p_responseBuf, sizeof(msg));

	p_responseBuf->dataType = dataType;
	if( buflen > 0 && pbuf != NULL){
		p_responseBuf->dataSize = buflen;
		memcpy( p_responseBuf->dataBuf, pbuf, buflen);
	}
	deb_print("cmd = %d, buflen = %d\n",dataType, buflen);
	ret = write(fd, p_responseBuf, sizeof(msg));
	if(ret< 0){
		perror("socket write error\n");
		free(p_responseBuf);
		return -1;
	}
	free(p_responseBuf);

	return 0;
}


extern char* getModulefwVersion()
{
	char *p = NULL;
	/* FIXME */
	return p;
}

extern int sendFirmware(int connfd){
		int ret;
		int firmwareFd;
		long offset;
		struct stat stat_buf;

 		/* open the file to be sent */
        firmwareFd = open(FWNAME, O_RDONLY);
        if (firmwareFd == -1) {
            fprintf(stderr, "unable to open '%s': %s\n", FWNAME, strerror(errno));
            exit(1);
        }
 
        /* get the size of the file to be sent */
        fstat(firmwareFd, &stat_buf);
 
        /* copy file using sendfile */
        offset = 0;
        ret = sendfile (connfd, firmwareFd, &offset, stat_buf.st_size);
        if (ret == -1) {
            fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
            return -1;
        }
         
        if (ret != stat_buf.st_size) {
            fprintf(stderr,\
                "incomplete transfer from sendfile: %d of %d bytes\n",\
                ret, (int)stat_buf.st_size);
            	return -1;
        }
		return 0;
}


/*
 * function: read data from socket
 */
extern int recvData(int fd, int *id,  int *dataType, void* buf, int* buflen, int time)
{

	int ret =-1;
#if 0
	msg msgbuf;
	fd_set rdfds;
	struct timeval *p_tv;

	FD_ZERO(&rdfds);
	FD_SET(fd, &rdfds);

	if(time <= 0 ){
		p_tv = NULL;
	}else{
		p_tv = (struct timeval*)malloc(sizeof(struct timeval));
		p_tv->tv_sec = time;
		p_tv->tv_usec = 0;
	}
	ret = select(fd+1, &rdfds, NULL, NULL, p_tv);
	if(ret < 0){
		perror("select error!\n");
	}else if(ret == 0 ){
		deb_print("select timeout\n");
	}else{
		ret = read(fd, &msgbuf, sizeof(msg));
		if(ret<0){
			perror("Socket ead error\n");
			return -1;
		}
		
		*id = msgbuf.id;
		*dataType = msgbuf.cmd;
		*buflen = msgbuf.bufsize;
		if( buflen != 0)
			memcpy(buf, msgbuf.buf, msgbuf.bufsize);
		return 0;
	}
	
	if(time > 0)
		free(p_tv);
#endif
	return ret;
}

int sendBroadCast()
{
	int ret = -1;
	int sock = -1;
	int j = -1;
	int so_broadcast = 1;
	struct ifreq *ifr;
	struct ifconf ifc;
	struct sockaddr_in broadcast_addr; //广播地址
	struct sockaddr_in from_addr; //服务端地址
	int from_len = sizeof(from_addr);
	int count = -1;
	fd_set readfd; //读文件描述符集合
	char buffer[1024];
	struct timeval timeout;
	timeout.tv_sec = 2; //超时时间为2秒
	timeout.tv_usec = 0;

	deb_print(" broadcast \n");

	//建立数据报套接字
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("create socket failed:");
		return -1;
	}

	// 获取所有套接字接口
	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_buf = buffer;
	if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0)
	{
		perror("ioctl-conf:");
		return -1;
	}
	ifr = ifc.ifc_req;
	for (j = ifc.ifc_len / sizeof(struct ifreq); --j >= 0; ifr++)
	{
		if (!strcmp(ifr->ifr_name, IFNAME))
		{
			if (ioctl(sock, SIOCGIFFLAGS, (char *) ifr) < 0)
			{
				perror("ioctl-get flag failed:");
			}
			break;
		}
	}

	//将使用的网络接口名字复制到ifr.ifr_name中，由于不同的网卡接口的广播地址是不一样的，因此指定网卡接口
	//strncpy(ifr.ifr_name, IFNAME, strlen(IFNAME));
	//发送命令，获得网络接口的广播地址
	if (ioctl(sock, SIOCGIFBRDADDR, ifr) == -1)
	{
		perror("ioctl error");
		return -1;
	}
	//将获得的广播地址复制到broadcast_addr
	memcpy(&broadcast_addr, (char *)&ifr->ifr_broadaddr, sizeof(struct sockaddr_in));
	//设置广播端口号
	printf("\nBroadcast-IP: %s\n", inet_ntoa(broadcast_addr.sin_addr));
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_port = htons(MCAST_PORT);

	//默认的套接字描述符sock是不支持广播，必须设置套接字描述符以支持广播
	ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &so_broadcast,
			sizeof(so_broadcast));

	//发送多次广播，看网络上是否有服务器存在
	int times = 1;
	int i = 0;
	for (i = 0; i < times; i++)
	{
		//一共发送10次广播，每次等待2秒是否有回应
		//广播发送服务器地址请求
		timeout.tv_sec = 2;  //超时时间为2秒
		timeout.tv_usec = 0;
		ret = sendto(sock, IP_FOUND, strlen(IP_FOUND), 0,
				(struct sockaddr*) &broadcast_addr, sizeof(broadcast_addr));
		if (ret < 0)
		{
			continue;
		}

#if 0
		//文件描述符清0
		FD_ZERO(&readfd);
		//将套接字文件描述符加入到文件描述符集合中
		FD_SET(sock, &readfd);
		//select侦听是否有数据到来
		ret = select(sock + 1, &readfd, NULL, NULL, &timeout);
		switch (ret)
		{
		case -1:
			break;
		case 0:
			perror("select timeout\n");
			break;
		default:
			//接收到数据
			if (FD_ISSET(sock,&readfd))
			{
				count = recvfrom(sock, buffer, 1024, 0,
						(struct sockaddr*) &from_addr, &from_len); //from_addr为服务器端地址
				printf("\trecvmsg is %s\n", buffer);
				if (strstr(buffer, IP_FOUND_ACK))
				{
					printf("\tfound server IP is %s, Port is %d\n",
							inet_ntoa(from_addr.sin_addr),
							htons(from_addr.sin_port));
				}
				return 0;

			}
			break;
		}
#endif
	}
	return -1;
}

char* getModuleIp( int id, char* ipaddr )
{
	int len;
	int i = 0;
	int s = 0;

	char lastStr[5];
	char n_lastStr[5];
	strcpy(lastStr, strrchr(ipaddr, '.' ));
	len = strlen(lastStr);
	
	for(i=0; i< len-1; i++){
		lastStr[i]=lastStr[i+1];
	}
	lastStr[len-1] = '\0';
	s = atoi(lastStr);
	s += id;
	len = strlen(ipaddr);
	
	for(i=strlen(lastStr); i>0; i--){
		ipaddr[len-i]='\0';
	}
	
	sprintf(n_lastStr, "%d", s);
	strcat(ipaddr, n_lastStr);

	deb_print( "id: %d, ip: %s\n", id, ipaddr);
	return ipaddr;
	
}

