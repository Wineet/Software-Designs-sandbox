#ifndef __LINK_LIST_H
#define __LINK_LIST_H

#include<stdio.h>
#include<stdbool.h>

typedef struct l_node{
	struct l_node *next;
}l_node;

typedef struct list{
	struct l_node n;
}list;

/* node is just an reference of current object for user to use,
 * One case: if user wants to free that node 
 *  Used "true" bool with or for only one entry case _temp->next will be null, problem with one or last entry
 *  Last entry should be processed*/
#define LIST_ITERATOR(list,node)  for( l_node *_temp = (list)->n.next, *_next= NULL; (_temp != NULL) && (node = _temp) && ( (_next = _temp->next)|| true) ; _temp = _next )
#define OFFSET(type,member) (unsigned int)((char*) (&(((type *)0)->member)))
#define RECOVER_LIST_DATA(type,member,node)   (type *) ((char*)node - (char *)OFFSET(type,member))




bool is_list_empty(struct list *head);
l_node *list_remove(struct list *head,struct l_node *n);
void list_Append(struct list *head,struct l_node *node);
void list_dtor(struct list *lst);
void list_ctor(struct list *lst);

#endif
