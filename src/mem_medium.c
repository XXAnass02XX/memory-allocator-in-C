/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

unsigned int puiss2(unsigned long size)
{
    unsigned int p = 0;
    size = size - 1; // allocation start in 0
    while (size)
    { // get the largest bit
        p++;
        size >>= 1;
    }
    if (size > (1 << p))
        p++;
    return p;
}

void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    unsigned int req_size = puiss2(size);
    int i = 0;
    if (arena.TZL[req_size] != 0)
    {
        void **case_actuelle = (void **)arena.TZL[req_size];
        if (*case_actuelle != 0){
            long a = 1 << req_size;
            arena.TZL[req_size] = (void*)((char *)case_actuelle + a);
        }
        return mark_memarea_and_get_user_ptr(arena.TZL[req_size],1 << req_size, MEDIUM_KIND);
    }
    else
    {
        while (arena.TZL[req_size + i] == NULL && req_size + i < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant)
        {
            i++;
        }
        if(arena.TZL[req_size + i] != NULL){
            while (i != 0)
            {
                void **case_actuelle = (void **)arena.TZL[req_size + i];
                if(*case_actuelle == NULL){
                    arena.TZL[req_size + i] = NULL;
                }
                else {
                    arena.TZL[req_size + i] = (void*) *case_actuelle;
                }
                long a = 1 << (req_size + i-1);
                arena.TZL[req_size + i - 1] = (void *)(case_actuelle);
//                *case_actuelle = (void*)((char *)case_actuelle + a);
                *((void **)arena.TZL[req_size + i - 1]) = (void*)((char *)case_actuelle + a);
                i--;
            }
        }

        if (req_size + i >= FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) {
            while (req_size + i >= FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) {
                mem_realloc_medium();
            }
            emalloc_medium(size);
        }
    }

    return mark_memarea_and_get_user_ptr(arena.TZL[req_size],1 << req_size, MEDIUM_KIND);
}

void efree_medium(Alloc a)
{
    unsigned long index = puiss2(a.size);
    void *block_addr = a.ptr;
    unsigned long buddy_offset = a.size;
    void *buddy = (void *)((unsigned long )a.ptr ^ buddy_offset);


    if (arena.TZL[index] == buddy){
        arena.TZL[index] = NULL;
        // Fusionner les deux blocs
        block_addr = buddy < block_addr ? buddy : block_addr;
//        *((void**)block_addr) = buddy;
        Alloc new_a = {block_addr, MEDIUM_KIND, a.size << 1};
        efree_medium(new_a);
    }else{
        arena.TZL[index] = buddy;
    }

}


