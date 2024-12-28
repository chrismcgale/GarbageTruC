#include "malloc.h"

static header_t base;           /* Zero sized block to get us started */
static header_t *freep = &base; /* Points to first free block */
static header_t *usedp;         /* Points to first used block */

/*
 * Scan the free list and look for a place to put the block
*/

static void 
add_to_free_list(header_t *bp) 
{
    header_t *p;

    for (p = freep; !(bp > p && bp < p->next); p = p->next)
        if (p >= p->next && (bp > p || bp < p->next))
            break;
    
    if (bp + bp->size == p->next) {
        bp->size += p->next->size;
        bp->next = p->next->next;
    } else {
        bp->next = p->next;
    }

    if (p + p->size == bp->next) {
        p->size += bp->next->size;
        p->next = bp->next->next;
    } else {
        p->next = bp->next;
    }

    freep = p;
}

#define MIN_ALLOC_SIZE 4096;

static header_t *
morecore(size_t num_units)
{
    void *vp;
    header_t *up;
    
    if (num_units > MIN_ALLOC_SIZE) num_units = MIN_ALLOC_SIZE / sizeof(header_t);

    if ((vp = sbrk(num_units * sizeof(header_t))) == (void *) - 1)
        return NULL;

    up = (header_t *) vp;
    up->size = num_units;
    add_to_free_list(up);
    return freep;
}

void *
gc_malloc(size_t alloc_size)
{
    size_t num_units;
    header_t *p,*prevp;

    num_units = (alloc_size + sizeof(header_t) - 1) / sizeof(header_t) + 1;
    prevp = freep;

    for (p = prevp->next;;  prevp = p; p = p->next) {
        if (p->size >= num_units) { // big enough
            if (p->size == num_units) { // exact fit
                prevp->next = p->next;
            } else {
                p->size -= num_units;
                p += p->size;
                p->size = num_units;
            }

            freep = prevp;

            if (usedp = NULL) {
                usedp = p->next = p;
            } else {
                p->next = usedp->next;
                usedp->next = p;
            }
            return (void *) p + 1;
        } 
        
        if (p == freep) { // Not enough memory
            p = morecore(num_units);
            if (p == NULL) {
                return NULL; // Memory request failed
            }
        }
    }
}