#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "workerpool.h"
#include "ll_list.h"

struct wp_handle *wp_init()
{
	printf("** wp_init **\n");
	struct wp_handle *wp = calloc(sizeof(struct wp_handle),1);
	if(wp == NULL)
		return NULL;
	list_ctor(&wp->job_pending);
	list_ctor(&wp->job_completed);
	list_ctor(&wp->job_running);
	list_ctor(&wp->thread_ready);
	list_ctor(&wp->thread_running);
	pthread_mutex_init(&wp->j_mutex,NULL);
	return wp;
};
/*Note Need better sync between thread create return and thread starts running
 * becuase thread might not start running yet but job posting is done and it iterate 
 * through list but it doesn;t find the thread in the list
 * */
int wp_thread_create(struct wp_handle *handle)
{
	/*
	 * should return thread node which is used in list*/
	
	/* create thread structure and add into ready list*/
	struct wp_thread *tn = calloc(sizeof(struct wp_thread),1);
	if(tn == NULL)
	{
		goto bail;
	}
	tn->status = THREAD_READY;
	sem_init(&tn->t_wait,0,0);
	tn->handle = handle;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_t tid;
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if(0 != pthread_create(&tid,&attr,wp_thread_routine,tn))
	{
		printf("Error wp_thread_create Failed\n");
		goto bail;
	}
	tn->tid = tid;
	list_Append(&handle->thread_ready,&tn->node);
bail:
	return 0;
}

void *wp_thread_routine(void *arg)
{
	/* Init thread */
	char name[10]={0};
	struct wp_thread *tn = (struct wp_thread*) tn;
	struct wp_handle *handle = tn->handle;
	snprintf(name,50,"%s%d","worker_",handle->active_thread); //count should increase where we are calling create thread
	pthread_setname_np(pthread_self(),name);
	while(1)
	{
		sem_wait(&(tn->t_wait));
		struct l_node *temp=NULL;
		/*
		 * Need to iterate for all pending jobs till job list is empty because if all max threads are busy user will put job in pending */
		LIST_ITERATOR(&handle->job_pending,temp)
		{

		 /* pickup job from pending list*/
			struct job *obj = RECOVER_LIST_DATA(struct job,node,temp);
			/* Validating job here*/
			if(obj != NULL && obj->f_ptr != NULL && obj->status == JOB_PENDING)
			{
		/*  remove job from the list and put it in running list  */
// Do correct here !!!				list_remjob_don (&handle->job_pending,temp);
				list_remove(&handle->thread_ready,&tn->node);
				list_Append(&handle->thread_running,&tn->node);
				obj->ret = obj->f_ptr(obj->arg);
				obj->status = JOB_DONE;
				list_Append(&handle->job_completed,temp);
				sem_post(&obj->job_done);
				list_remove(&handle->thread_running,&tn->node);
				list_Append(&handle->thread_ready,&tn->node);

			}
			else
			{
				printf("Job Validation failed \n");
				/*handle if any locks acquired and continue for next*
				 */
				continue;
			}
		}

		/* 
		 * remove thread from thread ready list and add in running list (should be done by job post API??)
		 * start executing the job
		 * update the job in completed list once job is completed, by setting job complete signal
		 * put the thread itself in remove from thread running list and put in thread_ready list  (should be done by job collect API??)
		 *
		 * and come back, those jobs need to picked up
		 */

	}
	return NULL;
}

int wp_job_post()
{
/*
	1. create job, add job to pending list
	2. check any active thread present
 			if No, & active thread< max thread create new thread and add it to active list
if Yes
	3. take ready thread refernce and signal the thread to start
	4. thread will pick up the job from pending list and starts working
	5. return refernce job id back to user */
	return 0;
}
void *wp_job_collect(void *job)
{
	/* search the job in completed list
	 * if job present return result and destroy created job node
	 * if job not completed wait on job complete signal for that job
	 * */
	return NULL;
}

