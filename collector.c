#include "header_t.h"

#define UNTAG(p) (((uintptr_t) (p)) & 0xfffffffc)
extern header_t* usedp, *freep;
extern header_t base;

/* Scan a region and mark any items in the used list */
static void
scan_region(uintptr_t *sp, uintptr_t *end)
{
    header_t *bp;
    for (;sp < end; sp++) {
        uintptr_t v = *sp;
        bp = usedp;
        do {
            if (bp + 1 <= v &&
                bp + 1 + bp->size > v) {
                    bp->next = ((uintptr_t) bp->next) | 1; // Mark bp by unaligning it's 'next field'. next is word alligned (8 bits), so unaligned the lsb doesn't change the address
                    break;
                } 
        } while ((bp = UNTAG(bp->next)) != usedp);
    }
}

/* Scan the marked blocks for references to unmarked blocks */
static void
scan_heap(void)
{
    uintptr_t *vp;
    header_t *up, *bp;
    for (bp = UNTAG(usedp->next); bp != usedp; bp = UNTAG(bp->next)) {
        if (!((uintptr_t) bp->next & 1)) continue;

        for (vp= (uintptr_t *) (bp + 1);
            vp < (bp + bp->size + 1);
            vp++) {
                uintptr_t v = *vp;
                up = UNTAG(bp->next);
                do {
                    if (up != bp &&
                    up + 1 <= v &&
                    up + 1 + up->size > v) {
                        up ->next = ((uintptr_t) p->next) | 1;
                        break;
                    }
                } while (up = UNTAG(up->next) != bp)
            }
    }
}

void
GC_innit(void)
{
    static int innited;
    FILE *statfp;

    if (innited)
        return;

    innited = 1;

    statfp = fopen("/proc/self/stat", "r");
    assert(statfp != NULL);
    fscanf(statfp,
        "%*d %*s %*c %*d %*d %*d %*d %*d %*u "
        "%*lu %*lu %*lu %*lu %*lu %*lu %*lu %*ld %*ld "
        "%*ld %*ld %*ld %*ld %*llu %*lu %*ld "
        "%*lu %*lu %*lu %*lu ", &stack_bottom);

    fclose(statfp);

    usedp = NULL;
    base.next = freep = &base;
    base.size = 0;
}

void
GC_collect(void)
{
    header_t *p, *prevp, *tp;
    uintptr_t stack_top;
    extern char end, etext; // Provided by the linker

    if (usedp = NULL)
        return;

    // Scan the BSS and initialized data segments
    scan_region(&etext, &end);

    // scan the stack
    asm volatile ("movl %%ebp, %0" : "=r" (stack_top));
    scan_region(stack_top, stack_bottom);

    // scan heap
    scan_heap();

    for (prevp = usedp, p = UNTAG(usedp->next);; prevp = p, p = UNTAG(p->next)){
        next_chunk:
            if (!(uintptr_t)p->next & 1) {
                // Not marked, so set free
                tp = p;
                p = UNTAG(p->next);
                add_to_free_list(tp);

                if (usedp = tp) {
                    usedp = NULL;
                    break;
                }

                prevp->next = (uintptr_t) p | ((uintptr_t) prevp->next & 1);
                goto next_chunk;
            }
        p->next = ((uintptr_t) p->next) & ~1;
        if (p == usedp)
            break;
    }
}