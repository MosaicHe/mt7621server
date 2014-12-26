#include "module.h"

void showUsge()
{
	printf("moduleset Usge:\n");
	printf("\t moduleset $moduleId [$nvramDev] $item $value\n");
	printf("\t Example: moduleset 1 rt2860 SSID1\n");
}

int main(int argc, char** argv )
{
	int moduleId;
	int ret;
	
	if( argc!=5){
		showUsge();
		return 0;
	}
	
	moduleId = atoi(argv[1]);
	if(moduleId<0 || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
    ret = module_set(moduleId, argv[2], argv[3], argv[4]);
	if(ret<0)
		printf("error\n");

	ret = module_set_commit(moduleId, argv[2]);
	if(ret<0)
		printf("error\n");

    return 0;
}
