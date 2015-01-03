#include "module.h"


int main(int argc, char** argv)
{
	int i;
	char mac[128];
	char *ipaddr;

	i=2;
	ipaddr = (char*)malloc(128);

	if( getModuleExist(i) != 1 ){
		printf("module %d not exist!\n", i);
	}else{
		printf("module %d exist!\n", i);
		
		bzero(ipaddr, 128);
		getModuleIp(i, ipaddr);
		printf("module %d ip : %s\n", i, ipaddr);
	
		free(ipaddr);
	}

	moduleSystem(2, "ifconfig ra0 down");
}
