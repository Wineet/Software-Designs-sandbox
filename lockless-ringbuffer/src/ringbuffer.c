#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"ringbuffer.h"
#include<sys/types.h>
#include<stdatomic.h>
#include<stdbool.h>
#include<pthread.h>
#define MAX_RETRY 10

/* BUg experiments
 * Try memory ordering for updating addresses
 * Try atomic tail address and entry count update
 * read address looks to be exceeding and reading stale not new data
 * */


/* TODO:
 * provide init modes for overwrite and lazy tail update etc
 *
 struct ringbuffer{
	_Atomic(void *) start_addr;
	_Atomic(void *) end_addr;
	unsigned int max_entry;
	unsigned int msg_size;
	unsigned int  entry_count;
	_Atomic(void *) read_addr;
	_Atomic(void *) read_tail_addr;
	_Atomic(void *) write_addr;
	_Atomic(void *) write_tail_addr;
};

*/

static inline void barrier_rw_complete(void)
{
	asm volatile("dsb sy" ::: "memory");
}

#undef MUTEX_LOCK
#ifdef MUTEX_LOCK
pthread_mutex_t lock;
#endif

void* ringbuffer_init(unsigned int msg_size,unsigned int max_entry)
{
	if(msg_size == 0 || max_entry == 0)
		return NULL;
	struct ringbuffer *handle = (struct ringbuffer *) malloc(sizeof(struct ringbuffer)); 
	if(handle == NULL)
		return NULL;
	handle->start_addr = (void *) malloc(msg_size*max_entry);
	if(handle->start_addr == NULL)
	{
		free(handle);
		return NULL;
	}

#ifdef MUTEX_LOCK
	pthread_mutex_init(&lock,NULL);
#endif
	handle->max_entry = max_entry;
	handle->rx = 0;
	handle->tx = 0;
	handle->msg_size = msg_size;
	handle->end_addr = (char *)(handle->start_addr) + (msg_size*max_entry);
	handle->read_addr = handle->write_addr = handle->start_addr;
	handle->read_tail_addr = handle->write_tail_addr = handle->start_addr;
	printf("Debug: %s start_addr=0x%p end_addr=0x%p max_entry=%u msg_size=%u \n",__func__,\
		handle->start_addr,handle->end_addr,handle->max_entry,handle->msg_size);
	return (void *)handle ;
}

/*
 * ringbuffer_write --> writes message packet to ring buffer of passed handle
 * 			message size of init will be copied to ring buffer
 * 			
 * Notes:
 * 		current "write_addr" point to the empty location which need to be written
 * */

int ringbuffer_write(void *handle_arg, void *msg)
{
	struct ringbuffer *handle = (struct ringbuffer*) handle_arg;
	int retry_cnt = 0;
	if(!handle || !msg )
		return -1;
#ifdef MUTEX_LOCK
	pthread_mutex_lock(&lock);
#endif
retry:
	barrier_rw_complete();
	void *local_write = atomic_load_explicit(&handle->write_addr,memory_order_seq_cst);
	void *local_read_tail = atomic_load_explicit(&handle->read_tail_addr,memory_order_seq_cst);
	void *local_next = (char *)local_write + handle->msg_size;
	unsigned int l_rx =  atomic_load(&handle->rx);
	unsigned int l_tx =  atomic_load(&handle->tx);
	if(local_next == atomic_load_explicit(&handle->end_addr,memory_order_seq_cst))
		local_next = atomic_load_explicit(&handle->start_addr,memory_order_seq_cst);

	if(local_write == local_read_tail && abs(l_tx-l_rx) >= handle->max_entry)
	{
#ifdef MUTEX_LOCK
		pthread_mutex_unlock(&lock);
#endif
		return -1;
	}
	if(false == atomic_compare_exchange_strong_explicit( &handle->write_addr,&local_write, local_next,memory_order_seq_cst,memory_order_seq_cst) )
	{
		if(retry_cnt >= MAX_RETRY)
		{
#ifdef MUTEX_LOCK
			pthread_mutex_unlock(&lock);
#endif
			return -1;
		}
		retry_cnt++;
		goto retry;
	}
	memcpy(local_write,msg,handle->msg_size);
	while(false == atomic_compare_exchange_strong_explicit( &handle->write_tail_addr,&local_write, local_next,memory_order_seq_cst,memory_order_seq_cst) );
	l_tx = (l_tx+1)%100; // need to decide this?? TODO !!!!
	atomic_exchange(&handle->tx,l_tx);
	barrier_rw_complete();
#ifdef MUTEX_LOCK
	pthread_mutex_unlock(&lock);
#endif
	return 0;
}

/*
 * ringbuffer_read --> reads the current location data from ring buffer
 * 			
 * */

int ringbuffer_read(void *handle_arg, void *msg)
{
	struct ringbuffer *handle = (struct ringbuffer*) handle_arg;
	int retry_cnt = 0;
	if(!handle || !msg )
		return -1;
retry:

	barrier_rw_complete();
#ifdef MUTEX_LOCK
	pthread_mutex_lock(&lock);
#endif
	void *local_read = atomic_load_explicit(&handle->read_addr,memory_order_seq_cst);
	void *local_write_tail = atomic_load_explicit(&handle->write_tail_addr,memory_order_seq_cst);
	void *local_next = local_read + handle->msg_size;
	unsigned int l_rx = atomic_load(&handle->rx);
	unsigned int l_tx = atomic_load(&handle->tx);

	if(local_next == atomic_load_explicit(&handle->end_addr,memory_order_seq_cst))
		local_next = atomic_load_explicit(&handle->start_addr,memory_order_seq_cst);
	if(local_read == local_write_tail && abs(l_tx-l_rx)  == 0)
	{
#ifdef MUTEX_LOCK
		pthread_mutex_unlock(&lock);
#endif
		return -1;
	}
	if(false == atomic_compare_exchange_strong_explicit( &handle->read_addr,&local_read, local_next,memory_order_seq_cst,memory_order_seq_cst ) )
	{
		if(retry_cnt >= MAX_RETRY)
		{
#ifdef MUTEX_LOCK
			pthread_mutex_unlock(&lock);
#endif
			return -1;
		}
		retry_cnt++;
		goto retry;
	}
	memcpy(msg,local_read,handle->msg_size);
	while(false == atomic_compare_exchange_strong_explicit(&handle->read_tail_addr, &local_read, local_next,memory_order_seq_cst,memory_order_seq_cst));
	l_rx = (l_rx+1)%100;
	atomic_exchange(&handle->rx,l_rx);
	barrier_rw_complete();
#ifdef MUTEX_LOCK
	pthread_mutex_unlock(&lock);
#endif
	return 0;
}

