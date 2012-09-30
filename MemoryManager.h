#ifndef __MM_H_
#define __MM_H_
#include <stdlib.h>

struct MemoryChunk{
	char p[1024];
	struct MemoryChunk *next;

};

struct MemoryManager{
	struct MemoryChunk *head, *curr;
	int curr_count;

};

void mem_init(struct MemoryManager*);
void* mem_alloc(struct MemoryManager*, int);
void mem_free(struct MemoryManager*);

#endif
