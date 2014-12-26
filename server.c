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
//#include "list.h"
#include "workthread.h"
#include "pingthread.h"
#include "handleweb.h"


#define HB_TIMEOUT 5
 
static pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
int modulefds[10];

#define MAX(a, b) (a>b?a:b)
#define MIN(a, b) (a>b?b:a)


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
	int on = 1;
	ret = setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

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

	if( listen(server_fd,5) < 0)
	{
		perror("listen");
		exit(1);
	}

	return server_fd;

}

int main(int argc, char**argv)
{
	int ret = -1;
	int i;
	char cmd[256];
	int server_fd,cli_fd, unix_fd, temp_fd;
	int udp_fd;
	pthread_t ptd, pingthd;
	
	fd_set rdfds;
	int maxfd;
	struct moduleSocketInfo* pmsi;

	struct sockaddr_in peer_addr;
	socklen_t addrlen;
	addrlen = sizeof(struct sockaddr);

	table_init(client_table);

	pthread_mutex_init(&mutex, NULL);		
	
	//open Unix domain socket for webserver
	unix_fd = openUnixdomainSocket();

	//open TCP listen socket
	server_fd = openListenTcpSocket();
	
	ret = pthread_create(&pingthd, NULL, pingThread, NULL);
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
		ret = select( maxfd+1, &rdfds, NULL, NULL, NULL);
		if(ret<0){
			perror("select error\n");
			exit(-1);
		}else if(FD_ISSET(server_fd, &rdfds)){
			cli_fd=accept(server_fd, (struct sockaddr*)&peer_addr, &addrlen);
			pmsi = (struct moduleSocketInfo*)malloc(sizeof(struct moduleSocketInfo));
			pmsi->fd = cli_fd;
			pmsi->addr = peer_addr;
			deb_print("ip:%s connected\n",inet_ntoa(peer_addr.sin_addr));
			ret=pthread_create( &ptd, NULL, workThread, pmsi);
			if(ret<0){
				perror("pthread_create error!\n");
				exit(-1);
			}
		}else if(FD_ISSET(unix_fd, &rdfds)){
			deb_print("%d\n", unix_fd);
			cli_fd=accept(unix_fd, NULL, NULL);
			if(temp_fd<0){
				perror("cannot accept client connect request\n");
				close( temp_fd);
				unlink(UNIX_DOMAIN);
				exit(-1);
			}
			handleUnixdomainSocket(cli_fd);
		}
	}

	close(unix_fd);
	close(server_fd);
	pthread_cancel(pingthd);
	unlink(UNIX_DOMAIN);
	return 0;
}
