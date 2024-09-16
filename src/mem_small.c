/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    void* head = arena.chunkpool;
    if(head == NULL){
         unsigned long len = mem_realloc_small();
         head = arena.chunkpool;
         void** temp_ptr = (void **)head;
         for(int i = 0; i < len - CHUNKSIZE ; i += 96 ){
            *(temp_ptr) = (void *)((char*)temp_ptr + CHUNKSIZE);
            temp_ptr = (void**) *temp_ptr;
         }
    }
    arena.chunkpool = (void*)((char*)arena.chunkpool + CHUNKSIZE );
    return mark_memarea_and_get_user_ptr(head,CHUNKSIZE, SMALL_KIND);
}
void efree_small(Alloc a) {
    void* ex_head = arena.chunkpool;
    arena.chunkpool = a.ptr;
    *((void **)(arena.chunkpool)) = ex_head;
}
