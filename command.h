#include "head.h"
#include "tool.h"

typedef int ( *CommandFunction )(int cmd, char* buf, int *len);

typedef struct{
	int commandNum;
	CommandFunction cmdfunc;
}commandNode;



#define SET_MODULEON 		 20
#define RESPONSE_SETMODULEON 21

#define GET_INFO 	 			22
#define RESPONSE_GETSSID 	 	23

#define CLOSE	0

int cf_moduleon(int cmd, char* buf, int *len)
{
	printf("hello cf_moduleon\n");
	return 0;
}

int cf_getclientSsid(int cmd, char* buf, int *len)
{
	printf("cf_getclientSsid\n");
	*len = strlen("hellossid");
	memcpy(buf, "hellossid", strlen("hellossid"));
	return 0;
}

commandNode cmdTable[]={
	{GET_INFO, cf_getclientSsid},
};


#define CMDTABLESIZE  sizeof(cmdTable)/sizeof(commandNode)

int doCommand(int cNum, char* buf, int *len )
{
	int i = 0;
	int ret = -1;
	
	deb_print("cmd number is: %d\n", cNum);
	for(i = 0; i< CMDTABLESIZE; i++){
		if( cmdTable[i].commandNum == cNum ){
			ret = cmdTable[i].cmdfunc(cNum, buf, len);
			return ret;
		}
	}
	printf("CommadNum is not in cmdTable\n");
	return ret;
}

