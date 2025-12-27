#ifndef __RING_BUFFER_H_
#define __RING_BUFFER_H_
#include<stdio.h>
#include<stdatomic.h>

struct ringbuffer{

	unsigned int max_entry;
	unsigned int msg_size;
	atomic_uint  rx;
	atomic_uint  tx;
	_Atomic(void *) start_addr;
	_Atomic(void *) end_addr;
	_Atomic(void *) read_addr;
	_Atomic(void *) read_tail_addr;
	_Atomic(void *) write_addr;
	_Atomic(void *) write_tail_addr;
};

void *ringbuffer_init(unsigned int, unsigned int);
int ringbuffer_write(void *, void *);
int ringbuffer_read(void *, void *);

#endif
