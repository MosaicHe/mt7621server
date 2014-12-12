
#ifndef __LIST_H
#define __LIST_H

#include "head.h"

typedef struct _listnode{
	struct _listnode* prev;
	struct _listnode* next;
	struct Event  *event;
}LISTNODE, *P_LISTNODE;


LISTNODE* list_create(LISTNODE** p_head);
int list_insert(LISTNODE* p_head, struct Event p_event);
int list_append(LISTNODE* p_head, struct Event p_evnet);
int list_pop(LISTNODE* p_head, struct Event** p_event);
int list_size(LISTNODE* p_head);
int list_destroy();

#endif
