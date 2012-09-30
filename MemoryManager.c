#include "MemoryManager.h"

void mem_init(struct MemoryManager *m)
{
	m->head = (struct MemoryChunk*)malloc(sizeof(struct MemoryChunk));
	m->curr = m->head;
	m->head->next = 0;
	m->curr_count = 0;
}

void* mem_alloc(struct MemoryManager *m, int size)
{
	if(!size)
		return 0;
	
	if(m->curr_count + size > 1024)
	{
		struct MemoryChunk *p;
		for(p = m->head;p->next != 0;p = p->next);
		p->next = (struct MemoryChunk*)malloc(sizeof(struct MemoryChunk));
		m->curr = p->next;
		m->curr->next = 0;
		m->curr_count = 0;
	}

	void* q = (void*)(m->curr->p + m->curr_count);
	m->curr_count += size;
	return q;
}

void mem_free(struct MemoryManager *m)
{
	struct MemoryChunk *p, *q;
	for(p = m->head;p != 0;p = q)
	{
		q = p->next;
		free(p);
	}
}
