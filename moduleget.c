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
	char value[128];
	
	if( argc!=4){
		showUsge();
		return 0;
	}
	
	moduleId = atoi(argv[1]);
	if(0>moduleId || moduleId>3){
		printf("moduleId should be 1-3");
		return -1;
	}
    module_get(moduleId, argv[2], argv[3], value);
	printf("%s\n", value);

    return 0;
}
