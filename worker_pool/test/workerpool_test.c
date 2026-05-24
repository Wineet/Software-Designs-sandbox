#include<stdio.h>
#include"workerpool.h"

void *print_job(void *arg)
{
	int *ret = (int *)malloc(sizeof(int));  //Not a Memory leak, should be free from user after job collect
	for(int i=0;i<5;i++)
		printf("%s \t", (char *)arg);
	printf("\n ret = 0x%p \n",ret);
 	*ret = 0xABA;
	return (void *) ret;
}

int main()
{
	
	printf("wp_init start\n");
	void *wp_handle = wp_init();
	printf("wp_init end\n");
	
	printf("Job Post start\n");
	void *job = wp_job_post(wp_handle,print_job,(void *)"print_job");
	if(!job)
	{
		printf("Job Post Failed\n");
		return -1;
	}
	printf("Job Post end\n");
	printf("Job collect start\n");
        int *ret = wp_job_collect(wp_handle,job);
	printf("job  collect end ret 0x%x\n",*ret);
	free(ret);
	// Yet to be implemented wp_deinit();
	return 0;
}
