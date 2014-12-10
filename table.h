#include "head.h"

#define TABLE_SIZE 4

client* client_table[TABLE_SIZE];

void table_init( client* client_table[] );
int table_insert( client* client_table[], client* p_client);
int table_delete( client* client_table[], client* p_client);
int table_get( client* client_table[], client* p_client, int i);
int table_valid( client* client_table[] );
int table_free( client* client_table[]);
void table_print( client* client_table[] );
void table_write( client* client_table[] );
