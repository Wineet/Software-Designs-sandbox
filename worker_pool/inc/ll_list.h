#ifndef __LINK_LIST_H
#define __LINK_LIST_H

#include<stdio.h>
#include<stdbool.h>

#define LIST_ITERATOR(list,node)  for(node = (list)->n.next ; node != NULL;node=node->next)
#define OFFSET(type,member) (unsigned int)((char*) (&(((type *)0)->member)))
#define RECOVER_LIST_DATA(type,member,node)   (type *) ((char*)node - (char *)OFFSET(type,member))

typedef struct l_node{
	struct l_node *next;
}l_node;

typedef struct list{
	struct l_node n;
}list;

bool is_list_empty(struct list *head);
void list_remove(struct list *head,struct l_node *n);
void list_Append(struct list *head,struct l_node *node);
void list_dtor(struct list *lst);
void list_ctor(struct list *lst);

#endif
