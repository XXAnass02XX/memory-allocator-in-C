/******************************************************
 * Copyright Grégory Mounié 2018-2022                 *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    /*calcule la valeure magique*/
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

MemKind
get_memkind(unsigned long value) {

    unsigned long last_two_bytes = value & 0xFFFF;

    unsigned long kind_bits = last_two_bytes & 0b11;

    // Convert the extracted bits to MemKind
    switch (kind_bits) {
        case 0b00:
            return SMALL_KIND;
        case 0b01:
            return MEDIUM_KIND;
        case 0b11:
            return LARGE_KIND;
        default:
            return -1;
    }
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    /* écrit le marquage dans les 16 premiers et les 16 derniers octets du bloc pointé
    par ptr et d’une longueur de size octets. Elle renvoie l’adresse de la zone
    utilisable par l’utilisateur, 16 octets après ptr */
    unsigned long magic_number = knuth_mmix_one_round((unsigned long)ptr);
    magic_number &= ~0b11;
    magic_number |= k;


    *((unsigned long *)(ptr)) = size;
    *((unsigned long*)(ptr) + 1) = magic_number;


    *((unsigned long*)(ptr) + size/sizeof(unsigned long) - 2) = magic_number;
    *((unsigned long*)(ptr) + size/sizeof(unsigned long) - 1) = size;



    return (void *)((char *)(ptr) + 16 );
}

Alloc
mark_check_and_get_alloc(void *ptr)
{
    /* ------ */

    Alloc a = {};
    a.ptr = (void *)((char *)(ptr) - 16);

    unsigned long size = *((unsigned long *)(ptr) - 2);
    unsigned long magic_number = *((unsigned long*)(ptr) - 1);

    MemKind k = get_memkind(magic_number);

    unsigned long real_magic_number = (knuth_mmix_one_round((unsigned long)a.ptr) & ~0b11) | k;
    assert(real_magic_number == magic_number);

    unsigned long size_last = *((unsigned long*)(a.ptr) + size/sizeof(unsigned long) - 1);
    unsigned long magic_number_last = *((unsigned long*)(a.ptr) + size/sizeof(unsigned long) - 2);

    assert(size == size_last);
    assert(magic_number == magic_number_last);

    a.size = size;
    a.kind = k;


    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1UL << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}





// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
