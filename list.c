#include <stdlib.h>
#include <stdio.h>
#include "list.h"

extern LISTNODE* list_create( )
{
	LISTNODE* p_head;
	p_head = (LISTNODE*) malloc(sizeof(LISTNODE));
	return p_head;
}

extern int list_insert(LISTNODE* p_head, struct Event* p_event)
{
	LISTNODE* p = p_head->next;
	LISTNODE *q;
	while(p != NULL)
		p = p->next;
	q = (LISTNODE*)malloc(sizeof(LISTNODE));
	q->next = NULL;
	q->event = p_event;

	p->next = q;
	return 0;	
}

extern int list_delete(LISTNODE* p_head, LISTNODE* p_node)
{
		
}

extern int list_get(LISTNODE* p_head, struct Event* p_event)
{
	LISTNODE* p = p_head->next;
	if(p == NULL)
		return -1;
	else{
		p_event=p->event;
		free(p);
		p_head->next = p->next;	
		return 0;
	}
}

extern int list_size(LISTNODE* p_head)
{
	LISTNODE* p;
	p = p_head->next;
	int i=0;
	while(p != NULL)
		i++;
	return i;			
}

extern int list_destroy()
{
	return 0;
}

