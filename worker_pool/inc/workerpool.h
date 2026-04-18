#ifndef __WORKERPOOL_H__
#define __WORKERPOOL_H__

#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#include<stdbool.h>
#include "ll_list.h"


typedef enum job_status{
	JOB_PENDING = 0,
	JOB_RUNNING,
	JOB_DONE
}job_status;

typedef enum thread_status{
	THREAD_READY = 0,
	THREAD_RUNNING
}thread_status;

typedef void* (*pfn)(void*);

struct job{
	job_status status;
	pfn f_ptr;
	void *arg;
	void *ret;
	sem_t job_done;
	struct l_node node;
};

struct wp_handle{
	pthread_mutex_t j_mutex;
	list job_pending;
	list job_completed;
	list job_running;
	list thread_ready;
	list thread_running;
	unsigned int active_thread;
	unsigned int max_threads;
};

// Keep in Mind
// user exposed API should be concurrent/ synchronised internally

struct wp_thread{
	thread_status status;
	struct l_node node;
	pthread_t tid;
	sem_t t_wait;
	struct wp_handle *handle;
};

struct wp_handle *wp_init(void);
int wp_thread_create(struct wp_handle *handle);
void *wp_thread_routine(void *arg);
int wp_job_post();
void *wp_job_collect(void *job);


#endif
