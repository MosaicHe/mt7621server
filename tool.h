extern int sendData(int fd, char dataType, void* buf, int buflen);
extern int recvData(int fd, msg* pmsg, struct timeval* ptv);
extern int getBit(char val1, char val2);
extern int resetModule(int id, int val);
extern int getModuleState();
extern int testModuleOn();
extern int sendFirmware();
extern char* getModulefwVersion();
extern int sendHeartbeat( client *pclient);
extern void printModuleInfo( moduleInfo *m);
