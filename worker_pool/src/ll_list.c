/*
 * Author: Wineet
 * File: ll_list.c
 * Note: Single linked list Data sturcture implementation, generic for all type of user_data
 *       API's are Not lock protected, external synchronization needed by user
 * */



#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdint.h>
#include"ll_list.h"

void list_ctor(struct list *lst)
{
	lst->n.next = NULL;
}
/* Warning: list_dtor call removes only all entries & NOT Freeing nodes memory */
void list_dtor(struct list *lst)
{
	while(!is_list_empty(lst))
	{
		struct l_node *temp = lst->n.next;
		list_remove(lst,temp);
	}
	lst->n.next=NULL;
}

void list_Append(struct list *head,struct l_node *node)
{
	struct l_node *prev = &head->n;
	struct l_node *head_next = head->n.next;
	prev->next= node;
	node->next = head_next;
  	return;
}

bool is_list_empty(struct list *head)
{
	return (head->n.next == NULL?true:false);
}

l_node *list_remove(struct list *head,struct l_node *n)
{
	struct l_node *prev = &head->n;
	struct l_node *temp = NULL;
	LIST_ITERATOR(head,temp)
	{
		if(temp == n){
		    prev->next = temp->next;
		    temp->next = NULL;
		    return temp;
		}
		prev = temp;
	}
	return NULL;
}
#if 0
/* 
 * Note: use below code only for testing purpose
 *       Keep the code till all functionality implemented of list
 */

struct user_data{
int x;
int y;
void *ptr;
struct l_node node;
};

void main()
{
	struct list head;
	struct user_data obj;
	list_ctor(&head);
	for(int i=0;i<10;i++)
	{
		struct user_data *ptr = calloc(sizeof(struct user_data),1);
		ptr->x = i;
		ptr->ptr = NULL;
		ptr->y = i+10;
		list_Append(&head,&ptr->node);
	}
	struct l_node *temp = NULL,*del = NULL,*next=NULL;	
	LIST_ITERATOR(&head,temp)
	{
		struct user_data *obj = RECOVER_LIST_DATA(struct user_data,node,temp);
		printf("obj->x=%d obj->y=%d temp->next=0x%p\n",obj->x,obj->y,temp->next);
	}
	
	
	if(!is_list_empty(&head))
	{
		list_dtor(&head);
	}
	LIST_ITERATOR(&head,temp)
	{
		struct user_data *obj = RECOVER_LIST_DATA(struct user_data,node,temp);
		printf("obj->x=%d obj->y=%d\n",obj->x,obj->y);
	}
}
#endif
