#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<stdbool.h>
#include<unistd.h>
#include<limits.h>
#include<stdint.h>
#include<sys/syscall.h>
#include<time.h>
#include"ringbuffer.h"
#include <stdatomic.h> //debug
#define NUM_THREAD 2
#define MAX_RETRY_CNT 1000
bool all_write_exit =false;

void sleep_ms(int milliseconds)
{
	struct timespec req;
	req.tv_sec = milliseconds/1000;
	req.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&req,NULL);
}

uint64_t result[NUM_THREAD][2]={0}; // global array to validate result
struct msg{
	pthread_t thread_id;
	uint64_t value;
};
void write_wrapper(void *handle,void *msg)
{	
	int retry_cnt = 0;
	if(!handle || !msg)
		return;
retry:
	if( ringbuffer_write(handle,msg))
	{
		if(retry_cnt>MAX_RETRY_CNT)
		{
		//	printf("** Write Failed **\n");
		}
		retry_cnt++;
		goto retry;
	}

}
int read_wrapper(void *handle,void *msg_arg)
{
	if(!handle || !msg_arg)
		return -1;
	if(ringbuffer_read(handle,msg_arg))
	{
		//printf("%s Failed \n",__func__);
		return -1;
	}
	return 0;
}
int write_packet_count = 0;
void *helper_write_routine(void *ringbuffer_handle)
{
//	printf("%s Started 0x%lx \n",__func__,pthread_self());
	struct msg msg={0};
	for(int i=1;i<100;i++)
	{
		msg.thread_id = pthread_self();
		msg.value = i;
		write_wrapper(ringbuffer_handle, &msg);
	//	printf("WRITE: Packet thread_id=0x%lx value=%llu \n", msg.thread_id,msg.value);
		memset(&msg,0,sizeof(msg));
		write_packet_count++;
	}
	printf("WRITE Done 0x%lx \n",pthread_self());
	return NULL;

}
int read_packet_count =0;
void *helper_read_routine(void *ringbuffer_handle)
{
	printf("%s Started 0x%lx\n",__func__,pthread_self());
	struct msg msg={0};
	bool exit_flag = false;
	struct ringbuffer *debug= (struct ringbuffer *)ringbuffer_handle;
	while(!exit_flag)
	{
		if(read_wrapper(ringbuffer_handle,&msg))
		{
			if(all_write_exit)
			{
				return NULL;
			}
			continue;//failed to read can be buffer empty
		}
//		printf("READ: Packet thread_id=0x%lx value=%llu \n", msg.thread_id,msg.value);
//		printf("READ: Read Addr=0x%p write_addr=0x%p \n", atomic_load(&debug->read_addr),atomic_load(&debug->write_addr));
		for(int i=0;i<NUM_THREAD;i++)
		{
			if(result[i][0] == msg.thread_id)
			{
				if (result[i][1]+1 != msg.value)
				{
					printf("READ: Wrong packet, possible corruption tid=0x%lx expec=%llu actual=%llu\n",msg.thread_id,result[i][1]+1,msg.value);
					printf("********** Debug Info **********\n");	
					printf(" total_read_packet %d \n",read_packet_count);
					printf(" total_write_packet %d \n",write_packet_count);
					printf(" total_entry count rx= %d tx=%d \n",((struct ringbuffer* )ringbuffer_handle)->tx,((struct ringbuffer* )ringbuffer_handle)->rx);	
					printf("********** **********\n");	
					exit_flag = true;
					exit(0);			
					break;
				}
				result[i][1]++;
			}
			else
			{
//				printf("READ: No thread Entry present\n");
			}
		}
		read_packet_count++;
	}	
	return NULL;
}
void multiple_write_single_read_test()
{
	pthread_t thread_ids[NUM_THREAD]={0};
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	void *ringbuffer_handle = ringbuffer_init(sizeof(struct msg),10);
	int result_idx = 0;
	printf("%s start \n",__func__);
	for(int i=0;i<NUM_THREAD-1;i++)
	{
		if( 0 != pthread_create(thread_ids+i,&thread_attr,helper_write_routine,ringbuffer_handle))
		{
			printf("Thread creation failed");
			return;
		}
		result[result_idx++][0]=thread_ids[i];
	}
	//sleep_ms(100);
	if( 0 != pthread_create(thread_ids+NUM_THREAD-1,&thread_attr,helper_read_routine,ringbuffer_handle))
	{
		printf("Thread creation failed");
		return;
	}
	
	for(int i=0;i<NUM_THREAD-1;i++)
	{
		pthread_join(thread_ids[i],NULL); //Joining for all write threads only
	}
	all_write_exit = true;
	printf("%s Done \n",__func__);
	return;

}
int main()
{

	multiple_write_single_read_test();
	return 0;
}
