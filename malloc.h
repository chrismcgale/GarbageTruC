// malloc.h
#ifndef MALLOC_H
#define MALLOC_H

typedef struct header {
    unsigned int    size;
    struct header   *next;
} header_t;


static void add_to_free_list(header_t *bp);

static header_t * morecore(size_t num_units);

void * gc_malloc(size_t alloc_size);

#endif // MALLOC_H
