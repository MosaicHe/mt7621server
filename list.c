#include <stdlib.h>
#include <stdio.h>
#include "list.h"

extern LISTNODE* list_create(LISTNODE** p_head)
{
	*p_head = (LISTNODE*) malloc(sizeof(LISTNODE));
	(*p_head)->next = NULL;
	return 0;
}

extern int list_insert(LISTNODE* p_head, struct Event event)
{
	return 0;	
}

extern int list_append(LISTNODE* p_head, struct Event event)
{
	LISTNODE* p = p_head;
	LISTNODE* q;
	while( p->next != NULL){
		deb_print(" p->next!=NULL\n");		
		p = p->next;
	}
	q = (LISTNODE*)malloc(sizeof(LISTNODE));
	q->next = NULL;
	q->event = (struct Event*)malloc( sizeof(struct Event) );
	*(q->event)= event;

	p->next = q;
	deb_print("list size:%d\n",list_size(p_head));

	return 0;
}


extern int list_delete(LISTNODE* p_head, LISTNODE* p_node)
{
		
}

extern int list_pop(LISTNODE* p_head, struct Event** p_event)
{
	LISTNODE* p = p_head->next;
	if(p == NULL)
		return -1;
	else{
		*(p_event)= p->event;
		p_head->next = p->next;
		return 0;
	}
}

extern int list_size(LISTNODE* p_head)
{
	LISTNODE* p;
	int i=0;
	p = p_head->next;
	while(p != NULL ){
		i++;
		p = p->next;
	}
	return i;			
}

extern int list_destroy()
{
	return 0;
}

