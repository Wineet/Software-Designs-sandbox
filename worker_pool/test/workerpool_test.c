#include<stdio.h>
#include<unistd.h>
#include"workerpool.h"

typedef enum _job_type
{
	PRINT_JOB = 0,
	BUSY_LOOP,
	WAIT_10_SEC,
	WAIT_2_SEC,
	WAIT_5_SEC_THREAD_EXIT
}job_type;

void *job(void *arg)
{
	job_type job = *(int *)arg;
	switch(job)
	{
		case PRINT_JOB:
		{
			int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
			for(int i=0;i<5;i++)
				printf("%lu : PRINT_JOB \n",pthread_self());
 			*ret = 0xABA;
			return (void *) ret;
		}
		case WAIT_10_SEC:
		{
			int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
			sleep(10);
 			*ret = 0xABB;
			return (void *) ret;
		}
		case WAIT_2_SEC:
		{
			int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
			sleep(2);
 			*ret = 0xABC;
			return (void *) ret;
		}
		case BUSY_LOOP:
		{
			int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
			printf("%lu : BUSY LOOP start\n",pthread_self());
			while(1)
			{
				sleep(1);
			}
 			*ret = 0xABD;
			return (void *) ret;
		}
		case WAIT_5_SEC_THREAD_EXIT:
		{
			int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
			printf("%lu : 5_SEC_WAIT start\n",pthread_self());
			sleep(5);
			printf("%lu : THREAD EXIT done\n",pthread_self());
 			*ret = 0xABE;
			pthread_exit(NULL);
			return (void *) ret;
		}
		default:
			printf("UNKNOWN Job Type \n");
			break;
	}
	return NULL;
}

/*--------------- Test Functions ----------------------- */

int  launch_jobs(void *wp_handle,int num_jobs,job_type job_arg);
void simple_single_job_test()
{


	printf("----- %s Test start ------\n",__func__);
	void *wp_handle = wp_init(2);	
	job_type job_arg = PRINT_JOB;
	void *ret_job = wp_job_post(wp_handle,job,(void *)&job_arg);
	if(!ret_job)
	{
		printf("Job Post Failed\n");
		return ;
	}
        int *ret = wp_job_collect(wp_handle,ret_job);
	free(ret);
	wp_deinit(wp_handle);
	printf("----- %s Test Passed ------\n",__func__);
}

void   max_job_creation_test()
{
	#define MAX_JOB 10

	printf("----- %s Test start ------\n",__func__);
	void *wp_handle = wp_init(2);	
	int ret = launch_jobs(wp_handle,10,WAIT_2_SEC);
	if(ret!=0)
		goto bail;
	sleep(1);

bail:
	if(ret !=0)
	{
		printf("ERROR: %s Failed\n",__func__);
	}
	else
	{
		wp_deinit(wp_handle);
		printf("------- %s Test Passed-------- \n\n",__func__);
	}
	return ;
}

void   back_to_back_job_test()
{

	printf("----- %s Test start ------\n",__func__);
	void *wp_handle = wp_init(2);	
	int ret = 0;
	
	ret = launch_jobs(wp_handle,5,WAIT_2_SEC);
	if(ret!=0)
		goto bail;
	sleep(1);
	
	ret = launch_jobs(wp_handle,5,WAIT_2_SEC);
	if(ret!=0)
		goto bail;
	sleep(1);

	ret = launch_jobs(wp_handle,5,WAIT_2_SEC);
	if(ret!=0)
		goto bail;
	sleep(1);

bail:
	if(ret !=0)
	{
		printf("ERROR: %s Failed\n",__func__);
	}
	else
	{
		wp_deinit(wp_handle);
		printf("----- %s Test Passed ------\n\n",__func__);
	}

		return ;
}

int  launch_jobs(void *wp_handle,int num_jobs,job_type job_arg)
{
	void **ret_job = (void *)calloc(sizeof(void *),num_jobs);
	int ret = 0;
	for(int i=0;i<num_jobs;i++)
	{
		printf("posting job[%d] type %d\n",i,job_arg);
		ret_job[i] = wp_job_post(wp_handle,job,(void *)&job_arg);
		if(!ret_job[i])
		{
			ret = -1;
			break;
		}
	}
	if(ret !=0)
	{
		printf("ERROR: %s Job Post Failed\n",__func__);
		free(ret_job);
		return -1;
	}
	for(int i=0;i<num_jobs;i++)
	{
		printf("collecting job[%d] type %d\n",i,job_arg);
        	int *job_ret = wp_job_collect(wp_handle,ret_job[i]);
		if(*job_ret != 0xABC)
		{
			ret = -1;
			printf("ERROR: %s Job collect Failed\n",__func__);
			free(ret_job);
			return -1;
		}
		free(job_ret);
	}
	free(ret_job);
	return 0;
}


int main()
{
	simple_single_job_test();
	max_job_creation_test();
	back_to_back_job_test();
	return 0;
}
