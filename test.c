#include "module.h"


int main(int argc, char** argv)
{
	int i;
	char mac[128];
	char *ipaddr;

	i=2;
	ipaddr = (char*)malloc(128);
<<<<<<< HEAD

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
		
=======
	for(i=1; i<4; i++){
		if( getModuleExist(i) != 1 ){
			printf("module %d not exist!\n", i);
		}else{
			printf("module %d\n", i);
			bzero(ipaddr, 128);
			getModuleIp(i, ipaddr);
			printf("\tip: %s\n", ipaddr);
			if(getModuleIfExist(i, "ra0")){
				getModuleMac(i, "ra0", mac);
				printf("\tra0 mac: %s\n", mac);
			}
			if(getModuleIfExist(i, "rai0")){
				getModuleMac(i, "rai0", mac);
				printf("\trai0 mac: %s\n", mac);
			}
			free(ipaddr);
		}
	}
>>>>>>> f0859847bf569dce5e37059de7e1ac4ec7ba91c9
#if 0
	printf("module %d exist!\n");

	if( getModuleIfExist(i, "ra0") ){
		printf("module ra0 exist\n");
		getModuleMac(i, "ra0", mac);
		printf("mac:%s\n", mac);
	}
	if(getModuleIfExist(i, "rai0")){
		printf("module rai0 exist\n");
		getModuleMac(i, "rai0", mac);
		printf("mac:%s\n", mac);
	}
#endif

}
