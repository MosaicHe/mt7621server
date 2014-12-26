#include "head.h"
#include "tool.h"
#include "table.h"

typedef struct{
	int moduleId;
	int req;
	char data[128];
}Request;

#define REQ_MODULE_EXIST	1
#define REQ_IF_EXIST		2
#define REQ_MAC				3
#define REQ_IP				4

int handleUnixdomainSocket( int fd)
{
	Request r;
	int id;
	client *pclient;
	int ret;
	deb_print("here here\n");
	ret=read(fd, &r, sizeof(Request));
	if(ret!= sizeof(Request))
		return -1;

	id = r.moduleId;
	ret = table_get_by_id(client_table, &pclient, id);
	if(ret<0 ){
		memcpy(r.data, ret, sizeof(ret));
	}
	else{
		switch(r.req){
			case REQ_MODULE_EXIST:
				memcpy(r.data, 1, sizeof(int));
				break;

			case REQ_IF_EXIST:
				if( !strcmp(r.data, "ra0") )
					memcpy(r.data, pclient->mdInfo.state_24g, sizeof(pclient->mdInfo.state_24g));
				else
					memcpy(r.data, pclient->mdInfo.state_5g, sizeof(pclient->mdInfo.state_24g));			
				break;

			case REQ_MAC:
				if( !strcmp(r.data, "ra0") )
					memcpy(r.data, pclient->mdInfo.mac_24g, strlen(pclient->mdInfo.mac_5g)+1 );
				else
					memcpy(r.data, pclient->mdInfo.mac_5g, strlen(pclient->mdInfo.mac_5g)+1 );			
				break;
			case REQ_IP:
					memcpy(r.data, inet_ntoa(pclient->addr.sin_addr), 
								1+strlen(inet_ntoa(pclient->addr.sin_addr)));
					deb_print("ip:%s\n", inet_ntoa(pclient->addr.sin_addr) );
				break;

			default:
				break;
		}
	}
	write(fd, &r, sizeof(Request));
	close(fd);

	return 0;		
}
