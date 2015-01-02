#ifndef __MODULE_H
#define __MODULE_H

int module_get(int moduleId, char *nvramDev, char* item, char* value);
int module_set(int moduleId, char *nvramDev, char* item, char* value);
int module_set_commit(int moduleId, char *nvramDev);
int reloadInernet(int moduleId);
int getModuleExist(int moduleId);
int getModuleIfExist(int moduleId, char* ifname);
int getModuleMac(int moduleId, char* ifname, char* mac);
int getModuleIp(int moduleId, char ip);
int moduleSystem(int moduleId, char* s);

#endif
