#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "workerpool.h"
#include "ll_list.h"

static void kill_all_threads(struct wp_handle *handle);
static void discard_all_jobs(struct wp_handle *handle);
static int wp_thread_create(struct wp_handle *handle);
static void *wp_thread_routine(void *arg);
static unsigned int get_active_thread_count(struct wp_handle *handle);

void *wp_init(unsigned int max_threads)
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
	pthread_mutex_init(&wp->t_mutex,NULL);
	wp->max_threads = max_threads;
	return wp;
}

/*Note Need better sync between thread create return and thread starts running
 * becuase thread might not start running yet but job posting is done and it iterate 
 * through list but it doesn;t find the thread in the list
 * */

/* wp_thread_create:
 * 	Func: used to create worker pool thread only
 * 		Restricted access for wp Library */
static int wp_thread_create(struct wp_handle *handle)
{
	if( get_active_thread_count(handle) >= handle->max_threads )
	{
		printf("%s Error: active thread MAX \n",__func__);
		return -1;
	}
	struct wp_thread *tn = calloc(sizeof(struct wp_thread),1);
	if(tn == NULL)
	{
		printf("%s Error: calloc Failed\n",__func__);
		goto bail;
	}
	tn->status = THREAD_READY;
	tn->exit = false;
	sem_init(&tn->t_wait,0,0);
	sem_init(&tn->exit_wait,0,0);
	tn->handle = handle;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_t tid;
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if(0 != pthread_create(&tid,&attr,wp_thread_routine,tn))
	{
		printf("%s Error: wp_thread_create Failed\n",__func__);
		goto bail;
	}
	tn->tid = tid;

	pthread_mutex_lock(&handle->t_mutex);
	handle->active_threads++;
	list_Append(&handle->thread_ready,&tn->node);
	pthread_mutex_unlock(&handle->t_mutex);
bail:
	return 0;
}

/* Fetch pending job validate and return */
static struct job *get_pending_job(struct wp_handle *handle)
{
	struct job *obj = NULL;
	struct l_node *temp = NULL;
	pthread_mutex_lock(&handle->j_mutex);
	LIST_ITERATOR(&handle->job_pending,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if(obj != NULL && obj->f_ptr != NULL && obj->status == JOB_PENDING)
		{
			list_remove(&handle->job_pending,&obj->node);
			list_Append(&handle->job_running,&obj->node);
			break;
		}
		else
		{
			printf("Job Validation failed \n");
			continue;
		}
	}
	pthread_mutex_unlock(&handle->j_mutex);
	return obj;
}
static unsigned int get_active_thread_count(struct wp_handle *handle)
{
	pthread_mutex_lock(&handle->t_mutex);
	unsigned int t_cnt = handle->active_threads;
	pthread_mutex_unlock(&handle->t_mutex);
	return t_cnt;
}
static void *wp_thread_routine(void *arg)
{

	/* Init thread */
	char name[10]={0};
	char temp[10]={0};
	struct wp_thread *tn = (struct wp_thread*) arg;
	struct wp_handle *handle = tn->handle;
	unsigned int t_cnt = get_active_thread_count(handle);
	snprintf(name,50,"%s%d","worker_",t_cnt);
	pthread_setname_np(pthread_self(),name);
	sem_wait(&(tn->t_wait)); // Initial wait to sync with job Adding
	while(!tn->exit)
	{
		struct job *obj = get_pending_job(handle);	
		if( obj == NULL)
		{
			sem_wait(&(tn->t_wait));
			if(tn->exit == true)
				break;
			else
				continue;
		}
		pthread_mutex_lock(&handle->t_mutex);
		list_remove(&handle->thread_ready,&tn->node);
		list_Append(&handle->thread_running,&tn->node);
		pthread_mutex_unlock(&handle->t_mutex);
		pthread_getname_np(pthread_self(),temp,sizeof(temp));
		obj->status = JOB_RUNNING;
		obj->ret = obj->f_ptr(obj->arg); //  job is excuting
		obj->status = JOB_DONE;
		obj->status = JOB_RUNNING;
		pthread_mutex_lock(&handle->j_mutex);
		list_remove(&handle->job_running,&obj->node);
		list_Append(&handle->job_completed,&obj->node);
		pthread_mutex_unlock(&handle->j_mutex);

		sem_post(&obj->job_done); //In case if collect_job API is blocked
		
		pthread_mutex_lock(&handle->t_mutex);
		list_remove(&handle->thread_running,&tn->node);
		list_Append(&handle->thread_ready,&tn->node);
		pthread_mutex_unlock(&handle->t_mutex);
	}
	printf("Thread Exiting ... exit=%d\n",tn->exit);
	sem_post(&tn->exit_wait);
	return NULL;
}

static struct job *create_new_job(pfn f_ptr,void *arg)
{
	struct job *obj = calloc(sizeof(struct job),1);
	if(obj == NULL)
	{
		return NULL;
	}
	obj->status = JOB_PENDING;
	obj->f_ptr = f_ptr;
	obj->arg = arg;
	obj->ret = NULL;
	sem_init(&obj->job_done,0,0);
	return obj;

}


static void start_thread(struct wp_thread *tn)
{
	if(tn != NULL)
	{
		sem_post(&tn->t_wait);
	}
	return;
}

static struct wp_thread *get_active_thread(struct wp_handle *handle)
{
	if(handle == NULL)
		return NULL;
	struct wp_thread *obj = NULL;
	struct l_node *temp = NULL;
	pthread_mutex_lock(&handle->t_mutex);
	LIST_ITERATOR(&handle->thread_ready,temp)
	{
		obj = RECOVER_LIST_DATA(struct wp_thread,node,temp);
		if( obj != NULL && obj->status == THREAD_READY )
		{
			break;
		}
		else
		{
			if(obj!=NULL)
				printf("%s:Error: Thread status %d\n",__func__,obj->status);
			else
				printf("%s:Error: Thread Validation failed\n",__func__);
			continue;
		}
	}
	pthread_mutex_unlock(&handle->t_mutex);
	return obj;
}


/*
	1. create job, add job to pending list
	2. Add job in pending list
	3. check any active thread present
 	if No, & active thread< max thread create new thread and add it to active list if Yes
	4. take ready thread refernce and signal the thread to start
	5. thread will pick up the job from pending list and starts working
*/

void *wp_job_post(struct wp_handle *handle,pfn f_ptr,void *arg)
{
	struct job *obj = create_new_job(f_ptr,arg);
	if( obj == NULL)
		return NULL;
	struct wp_thread *tn = NULL;
	/* post job in pending list*/
	pthread_mutex_lock(&handle->j_mutex);
	list_Append(&handle->job_pending,&obj->node);
	pthread_mutex_unlock(&handle->j_mutex);
	while( (tn = get_active_thread(handle) ) == NULL)
	{

		if( get_active_thread_count(handle) >= handle->max_threads)
		{
			printf("warning: Max active threads \n");
			break;
		}
		if(wp_thread_create(handle))
			printf("warning: wp_thread_create Failed \n");
	}
	if(tn == NULL)
		return obj;
#if 0
/* To Do Think: Is it a good idea to club append in get_active_thread call??*/
	pthread_mutex_lock(&handle->t_mutex);
	list_Append(&handle->thread_running,&tn->node);
	pthread_mutex_unlock(&handle->t_mutex);
#endif
	start_thread(tn);
	return (void *)obj;
}



bool is_valid_job(struct wp_handle *handle, void *user_job)
{
	if(handle == NULL || user_job == NULL)
	{
		printf("Error: %s Invalid Arguments \n",__func__);
		return false;
	}
	struct job *obj = NULL;
	struct l_node *temp = NULL;
	bool ret = false;
	pthread_mutex_lock(&handle->j_mutex);
	LIST_ITERATOR(&handle->job_pending,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj == user_job )
		{
			ret = true;
			goto bail;
		}

	}
	LIST_ITERATOR(&handle->job_running,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj == user_job )
		{
			ret = true;
			goto bail;
		}
	}
	LIST_ITERATOR(&handle->job_completed,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj == user_job )
		{
			ret = true;
			goto bail;
		}
	}
bail:
	pthread_mutex_unlock(&handle->j_mutex);
	return ret;
}

static void destroy_job(struct job *obj)
{
	sem_destroy(&obj->job_done);
	free(obj);
}

/* search the job in completed list
 * if job present return result and destroy created job node
 * if job not completed wait on job complete signal for that job
 * */

void *wp_job_collect(struct wp_handle *handle,void *user_job)
{
	if(false == is_valid_job(handle,user_job))
		return NULL;
	struct job *obj = (struct job*)user_job;
	if(obj->status != JOB_DONE)
		sem_wait(&obj->job_done);
	void *ret = obj->ret;

	pthread_mutex_lock(&handle->j_mutex);
	if(list_remove(&handle->job_completed,&obj->node ) != NULL)
		destroy_job(obj);
	pthread_mutex_unlock(&handle->j_mutex);
	return ret;
}

static void kill_all_threads(struct wp_handle *handle)
{
	if(handle == NULL)
	{
		printf("ERROR : %s Invalid handle",__func__);
		return;
	}
	struct wp_thread *obj = NULL;
	struct l_node *temp = NULL;
	pthread_mutex_lock(&handle->t_mutex);
	LIST_ITERATOR(&handle->thread_ready,temp)
	{
		obj = RECOVER_LIST_DATA(struct wp_thread,node,temp);
		if( obj != NULL )
		{
			list_remove(&handle->thread_ready,&obj->node);
			obj->exit = true;
			start_thread(obj); // In case thread waiting for job
			sem_wait(&obj->exit_wait);
			free(obj); //List iterator needed to modify
		}	
	}
	obj = NULL; temp = NULL;
	LIST_ITERATOR(&handle->thread_running,temp)
	{
		obj = RECOVER_LIST_DATA(struct wp_thread,node,temp);
		if( obj != NULL )
		{
			list_remove(&handle->thread_running,&obj->node);	
			obj->exit = true;
			start_thread(obj);  
			sem_wait(&obj->exit_wait);
		   	free(obj); 
		}	
	}
	pthread_mutex_unlock(&handle->t_mutex);
	return;
}

static void discard_all_jobs(struct wp_handle *handle)
{
	if(handle == NULL)
	{
		printf("ERROR : %s Invalid handle",__func__);
		return;
	}
	struct job *obj = NULL;
	struct l_node *temp = NULL;
	pthread_mutex_lock(&handle->j_mutex);
	LIST_ITERATOR(&handle->job_pending,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj != NULL )
		{
			list_remove(&handle->job_pending,&obj->node);
			printf(" %d obj=%p \n",__LINE__,obj);
			free(obj);
		}

	}
	obj = NULL; temp = NULL;
	LIST_ITERATOR(&handle->job_running,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj != NULL )
		{
			list_remove(&handle->job_running,&obj->node);
			printf(" %d obj=%p \n",__LINE__,obj);
			free(obj);
		}
	}
/* Note: completed job may or may not be collected by user, if not collected remove job from completed when user collects it
 */
	obj = NULL; temp = NULL;
	LIST_ITERATOR(&handle->job_completed,temp)
	{
		obj = RECOVER_LIST_DATA(struct job,node,temp);
		if( obj != NULL )
		{
			list_remove(&handle->job_completed,&obj->node);
			printf(" %d obj=%p \n",__LINE__,obj);
/*Release semphore of job too before free*/
			free(obj);
		}
	}
	pthread_mutex_unlock(&handle->j_mutex);
	return;

}
/* wp_deinit()
 * --> if deinit called all running jobs will be abruptly closed, complete job status will be lost, pending jobs will be discarded
 * 1. Kill all worker pool (ready, running) threads, remove entries while doing
 * 2. destroy all Jobs, running, pending and in progress, remove entries while doing
 * 3. Deinit locks
 * 4. Free wp memory
 * */

void wp_deinit(struct wp_handle *handle)
{
	printf("** wp_deinit ** \n");
	if(handle == NULL)
	{
		printf("ERROR: %s Invalid Handle",__func__);
		return ;
	}
	kill_all_threads(handle);
	discard_all_jobs(handle);
	if ( pthread_mutex_destroy(&handle->t_mutex) || pthread_mutex_destroy(&handle->j_mutex) )
	{
		printf("ERROR: %s Mutex Destroy Failed",__func__);
		return;
	}
	free(handle);
	return;
}
