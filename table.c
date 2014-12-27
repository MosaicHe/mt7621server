#include "table.h"

#if 0
void client_print( client* p)
{
	printf("      id: %d\n", p->id);
	printf("      fd: %d\n", p->fd);
	
	printf("    ssid: %s\n", p->ssid);
	printf("     mac: %s\n", p->mac);
	printf(" channel: %d\n", p->channel);
	
	if(p->have5g){
		printf("5G\n");
		printf("    ssid: %s\n", p->ssid_5g);
		printf("     mac: %s\n", p->mac_5g);
		printf(" channel: %d\n", p->channel_5g);
	}
}


void table_print( client* client_table[] )
{
	int i = 0;
	for(i=1; i<TABLE_SIZE; i++){
		if( client_table[i]!=NULL){
			printf("MODULE %d\n",i);
			printf("-------------------\n");
			client_print( client_table[i]);
			printf("-------------------\n");
			printf("\n");
		}
	}
}

#endif
void table_init( client* client_table[] )
{
	int i = 0;
	for(i=1; i<TABLE_SIZE; i++){
		client_table[i] = NULL;
	}
}

int table_insert( client* client_table[], client* p_client)
{	
	int index = 0;
	client* p;
	
	index = table_valid( client_table );
	if( index < 1)
		return -1;
		
	p = (client*)malloc(sizeof(client));
	if(p == NULL ){
		perror("malloc error !");
		return -1;
	}
	memcpy(p, p_client, sizeof(client));
	client_table[ index ] = p;
	return index;
}

int table_delete( client* client_table[], client* p_client)
{
	int i;
	if(p_client==NULL)
		return -1;
	for(i=1; i<TABLE_SIZE; i++){
		if( client_table[i]== p_client ){
			free( client_table[i] );
			client_table[i] = NULL;
			return 0;
		}
	}
	return -1;	
}

int table_get( client* client_table[], client** p_client, int i)
{	
	if(i > TABLE_SIZE-1){
		perror("index is larger than limited\n");
		return -1;
	}
	if( client_table[i]!=NULL){
		*p_client = client_table[i];
		return 0;
	}else{ 
		*p_client = NULL;
		return -1;
	}
}

int table_get_by_id(client* client_table[], client** p_client, int id)
{
	int i;
	if(id<1 || id > 3){
		printf("Id:%d is error\n",id);
		return -1;
	}
	for(i=1;i<TABLE_SIZE-1;i++){
		if( client_table[i]!=NULL){
			if(client_table[i]->moduleID==id){
				(*p_client) = client_table[i];
				return 0;
			}
		}
	}
	
	(*p_client) = NULL;
	return -1;
}

int table_valid( client* client_table[])
{
	int i = 1;
	while( client_table[i] != NULL && i<TABLE_SIZE-1 ){
		i++;
	}
	if(i == TABLE_SIZE)
		return -1;
	else{
		return i;
	}

}

int table_size( client* client_table[])
{
	int i = 1;
	while( client_table[i] != NULL && i<TABLE_SIZE-1 ){
		i++;
	}
	return i-1;
}

int table_free( client* client_table[])
{
	int i =0;
	for(i =1; i<TABLE_SIZE; i++){
		if( client_table[i]!=NULL ){
			free(client_table[i]);
			client_table[i]=NULL;
		}
	}
	return 0;
}

void table_write( client* client_table[] )
{
#if 0
	int i;
	FILE * fp;
    fp=fopen( TMPFILE, "wb");

	if(fp == NULL)
			return;
	
	for(i=1; client_table[i]!=NULL; i++){
		fwrite( client_table[i], sizeof( client), 1, fp);
	}
	fclose(fp);
#endif
}


