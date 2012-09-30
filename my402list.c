#include "my402list.h"

#include <stdio.h>
#include <stdlib.h>

int  My402ListLength(My402List *l)
{
	return l->num_members;
}

int  My402ListEmpty(My402List *l)
{
	if(l->num_members == 0)
		return TRUE;
	return FALSE;
}

int  My402ListAppend(My402List *l, void *data)
{
	My402ListElem *p = (My402ListElem*)malloc(sizeof(My402ListElem));

	if(!p)
		return FALSE;

	My402ListElem *last = My402ListLast(l);

	last->next = p;
	p->prev = last;
	p->next = &l->anchor;
	l->anchor.prev = p;
	
	p->obj = data;

	l->num_members++;

	return TRUE;
}

int  My402ListPrepend(My402List *l, void *data)
{
	My402ListElem *p = (My402ListElem*)malloc(sizeof(My402ListElem));

	if(!p)
		return FALSE;

	My402ListElem *first = My402ListFirst(l);

	p->next = first;
	first->prev = p;
	p->prev = &l->anchor;
	l->anchor.next = p;
	
	p->obj = data;

	l->num_members++;

	return TRUE;
}

void My402ListUnlink(My402List *l, My402ListElem *el)
{
	el->prev->next = el->next;
	el->next->prev = el->prev;

	free((void*)el);
	l->num_members--;
}

void My402ListUnlinkAll(My402List *l)
{
	My402ListElem *p,*q;
    for(p = My402ListFirst(l);p != &l->anchor;p = q)
    {
        q = My402ListNext(l, p);
		free((void*)p);
    }

	l->num_members = 0;
}

int  My402ListInsertAfter(My402List *l, void *data, My402ListElem *el)
{
	if(!el)
		return My402ListAppend(l, data);

	My402ListElem *p = (My402ListElem*)malloc(sizeof(My402ListElem));

	if(!p)
		return FALSE;
	p->obj = data;

	el->next->prev = p;
	p->next = el->next;
	el->next = p;
	p->prev = el;

	l->num_members++;

	return TRUE;
}

int  My402ListInsertBefore(My402List *l, void *data, My402ListElem *el)
{
	if(!el)
		return My402ListPrepend(l, data);

	My402ListElem *p = (My402ListElem*)malloc(sizeof(My402ListElem));

	if(!p)
		return FALSE;
	p->obj = data;

	el->prev->next = p;
	p->prev = el->prev;
	p->next = el;
	el->prev = p;

	l->num_members++;

	return TRUE;
}

 My402ListElem* My402ListFirst(My402List *l)
 {
     return l->anchor.next;
 }

 My402ListElem* My402ListLast(My402List *l)
 {
	 return l->anchor.prev;
 }

 My402ListElem* My402ListNext(My402List *l, My402ListElem *elem)
 {
     return elem->next;
 }

 My402ListElem* My402ListPrev(My402List *l, My402ListElem *elem)
 {
     return elem->prev;
 }

My402ListElem *My402ListFind(My402List *l, void *data)
{
	My402ListElem *p;

	for(p = My402ListFirst(l);p != 0;p = My402ListNext(l,p))
	{
		if(p->obj == data)
			break;
	}

	return p;
}

int My402ListInit(My402List *l)
{
	if(l)
	{
		l->anchor.next = &l->anchor;
		l->anchor.prev = &l->anchor;
		l->num_members = 0;
		return TRUE;
	}
	return FALSE;
}
