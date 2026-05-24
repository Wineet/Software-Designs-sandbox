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

 void list_remove(struct list *head,struct l_node *n)
{
	struct l_node *prev = &head->n;
	struct l_node *temp = NULL;
	LIST_ITERATOR(head,temp)
	{
		if(temp == n){
		    prev->next = temp->next;
		    temp->next = NULL;
	//	    void *ptr  = (void *) RECOVER_LIST_DATA(struct user_data,node,temp);
	//	    printf(" deleting object ptr->x=%d ptr->y=%d\n",ptr->x,ptr->y);
	//	    free(ptr);
		    break;
		}
		prev = temp;
	}
}

/* 
 * Note: use below code only for testing purpose
 *       Keep the code till all functionality implemented of list
 */
/*
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
	struct l_node *temp = NULL,*del = NULL;
	int i=5;
	LIST_ITERATOR(&head,temp)
	{
		i--;
		if(i==0)
			del=temp;
		struct user_data *obj = RECOVER_LIST_DATA(struct user_data,node,temp);
		printf("obj->x=%d obj->y=%d\n",obj->x,obj->y);
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
*/
