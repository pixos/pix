/*_
 * Copyright (c) 2015-2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <aos/const.h>
#include "kernel.h"

/* ToDo: Implement segregated fit */


/* Prototype declarations */
static void * _kmem_alloc_pages(struct kmem *, int);
static void * _kmem_alloc_pages_from_new_region(struct kmem *, int);

/*
 * Allocate pages
 */
void *
kmem_alloc_pages(struct kmem *kmem, size_t npg)
{
    int order;
    void *vaddr;

    /* Calculate the order in the buddy system from the number of pages */
    order = bitwidth(npg);

    /* Check the order */
    vaddr = _kmem_alloc_pages(kmem, order);

    return vaddr;
}

void
kmem_free_pages(struct kmem *kmem, void *ptr)
{
    panic("FIXME: kmem_free_pages()");
}

/*
 * Allocate pages
 */
static void *
_kmem_alloc_pages(struct kmem *kmem, int order)
{
    struct kmem_page *pg;
    void *vaddr;
    void *paddr;
    ssize_t i;
    int ret;

    /* Try to grab pages from the kernel memory space */
    pg = kmem_grab_pages(kmem, order);
    if ( NULL == pg ) {
        /* No available pages, then try to grab from superpage  */
        panic("FIXME 12");
        return NULL;
        //return _kmem_alloc_pages_from_new_superpage(kmem, order);
    }

    /* Found */
    vaddr = kmem->space->start + KERN_PAGE_ADDR(pg - kmem->space->pages);

    /* Allocate physical memory */
    paddr = pmem_alloc_pages(PMEM_ZONE_LOWMEM, order);
    if ( NULL == paddr ) {
        /* Release the virtual memory */
        kmem_return_pages(kmem, pg);
        return NULL;
    }

    /* Map the physical and virtual memory */
    for ( i = 0; i < (1LL << order); i++ ) {
        ret = arch_kmem_map(kmem, vaddr + KERN_PAGE_ADDR(i),
                            paddr + KERN_PAGE_ADDR(i), pg->flags);
        if ( ret < 0 ) {
            /* Release the virtual and physical memory */
            kmem_return_pages(kmem, pg);
            pmem_free_pages(paddr);
            return NULL;
        }
    }

    return vaddr;
}

/*
 * Count the memory order for buddy system
 */
static int
_kmem_buddy_order(struct kmem *kmem, size_t pg)
{
    int o;
    size_t i;
    size_t npg;

    /* Calculate the number of pages */
    npg = kmem->space->len / KERN_PAGESIZE;

    /* Check the order for contiguous usable pages */
    for ( o = 0; o <= KMEM_MAX_BUDDY_ORDER; o++ ) {
        for ( i = pg; i < pg + (1ULL << o); i++ ) {
            if ( !KMEM_IS_FREE(&kmem->space->pages[pg]) ) {
                /* It contains an unusable page, then return the current order
                   minus 1, immediately. */
                return o - 1;
            }
        }
        /* Test whether the next order is feasible; feasible if it is properly
           aligned and the pages are within the range of this region. */
        if ( 0 != (pg & (1ULL << o)) || pg + (1ULL << (o + 1)) > npg ) {
            /* Infeasible, then return the current order immediately */
            return o;
        }
    }

    /* Reaches the maximum order */
    return KMEM_MAX_BUDDY_ORDER;
}

/*
 * Create buddy system for the kernel memory
 */
int
kmem_buddy_init(struct kmem *kmem)
{
    ssize_t i;
    ssize_t j;
    int o;

    /* Initialize all orders */
    for ( i = 0; i <= KMEM_MAX_BUDDY_ORDER; i++ ) {
        kmem->space->heads[i] = NULL;
    }

    /* Look through all the pages */
    for ( i = 0; i < (ssize_t)(kmem->space->len / KERN_PAGESIZE);
          i += (1ULL << o) ) {
        o = _kmem_buddy_order(kmem, i);
        if ( o < 0 ) {
            /* This page is not usable. */
            o = 0;
        } else {
            /* This page is usable. */
            for ( j = 0; j < (1LL << o); j++ ) {
                kmem->space->pages[i + j].order = o;
            }
            /* Add this to the buddy system at the order of o */
            kmem->space->pages[i].prev = NULL;
            kmem->space->pages[i].next = kmem->space->heads[o];
            if ( NULL != kmem->space->heads[o] ) {
                kmem->space->heads[o]->prev = &kmem->space->pages[i];
            }
            kmem->space->heads[o] = &kmem->space->pages[i];
        }
    }

    return 0;
}








/*
 * Split the buddies so that we get at least one buddy at the order of o
 */
static int
_kmem_buddy_pg_split(struct kmem *kmem, int o)
{
    int ret;
    struct kmem_page *p0;
    struct kmem_page *p1;
    struct kmem_page *next;
    size_t i;

    /* Check the head of the current order */
    if ( NULL != kmem->space->heads[o] ) {
        /* At least one memory block (buddy) is available in this order. */
        return 0;
    }

    /* Check the order */
    if ( o + 1 >= KMEM_MAX_BUDDY_ORDER ) {
        /* No space available */
        return -1;
    }

    /* Check the upper order */
    if ( NULL == kmem->space->heads[o + 1] ) {
        /* The upper order is also empty, then try to split one more upper. */
        ret = _kmem_buddy_pg_split(kmem, o + 1);
        if ( ret < 0 ) {
            /* Cannot get any */
            return ret;
        }
    }

    /* Save next at the upper order */
    next = kmem->space->heads[o + 1]->next;
    /* Split it into two */
    p0 = kmem->space->heads[o + 1];
    p1 = p0 + (1ULL << o);

    /* Set the order for all the pages in the pair */
    for ( i = 0; i < (1ULL << (o + 1)); i++ ) {
        p0[i].order = o;
    }

    /* Insert it to the list */
    p0->prev = NULL;
    p0->next = p1;
    p1->prev = p0;
    p1->next = kmem->space->heads[o];
    //kmem->space->heads[o]->prev = p1;
    kmem->space->heads[o] = p0;
    /* Remove the split one from the upper order */
    kmem->space->heads[o + 1] = next;
    if ( NULL != next ) {
        next->prev = NULL;
    }

    return 0;
}

/*
 * Merge buddies onto the upper order on if possible
 */
static void
_kmem_buddy_pg_merge(struct kmem *kmem, struct kmem_page *off, int o)
{
    struct kmem_page *p0;
    struct kmem_page *p1;
    size_t pi;
    size_t i;

    if ( o + 1 >= KMEM_MAX_BUDDY_ORDER ) {
        /* Reached the maximum order */
        return;
    }

    /* Check the region */
    pi = off - kmem->space->pages;
    if ( pi >= kmem->space->len / KERN_PAGESIZE ) {
        /* Out of this region */
        return;
    }

    /* Get the first page of the upper order */
    p0 = &kmem->space->pages[FLOOR(pi, 1ULL << (o + 1))];

    /* Get the neighboring buddy */
    p1 = p0 + (1ULL << o);

    /* Ensure that p0 and p1 are free */
    if ( !KMEM_IS_FREE(p0) || !KMEM_IS_FREE(p1) ) {
        return;
    }

    /* Check the order of p1 */
    if ( p0->order != o || p1->order != o ) {
        /* Cannot merge because of the order mismatch */
        return;
    }

    /* Remove both of the pair from the list of current order */
    if ( p0->prev == NULL ) {
        /* Head */
        kmem->space->heads[o] = p0->next;
        kmem->space->heads[o]->prev = NULL;
    } else {
        /* Otherwise */
        p0->prev->next = p0->next;
        p0->next->prev = p0->prev;
    }
    if ( p1->prev == NULL ) {
        /* Head; note that this occurs if the p0 was the head and p1 was next to
           p0. */
        kmem->space->heads[o] = p1->next;
        kmem->space->heads[o]->prev = NULL;
    } else {
        p1->prev->next = p1->next;
        p1->next->prev = p1->prev;
    }

    /* Set the order for all the pages in the pair */
    for ( i = 0; i < (1ULL << (o + 1)); i++ ) {
        p0[i].order = o + 1;
    }

    /* Prepend it to the upper order */
    p0->prev = NULL;
    p0->next = kmem->space->heads[o + 1];
    if ( NULL != kmem->space->heads[o + 1] ) {
        kmem->space->heads[o + 1]->prev = p0;
    }
    kmem->space->heads[o + 1] = p0;

    /* Try to merge the upper order of buddies */
    _kmem_buddy_pg_merge(kmem, p0, o + 1);
}

/*
 * Find available virtual pages
 */
struct kmem_page *
kmem_grab_pages(struct kmem *kmem, int order)
{
    struct kmem_page *pg;
    int ret;
    ssize_t i;

    /* Check the order first */
    if ( order < 0 || order > KMEM_MAX_BUDDY_ORDER ) {
        /* Invalid order */
        return NULL;
    }

    /* Split first if needed */
    ret = _kmem_buddy_pg_split(kmem, order);
    if ( ret < 0 ) {
        return NULL;
    }
    /* Get one from the buddy system, and remove that from the list */
    pg = kmem->space->heads[order];
    kmem->space->heads[order] = pg->next;
    if ( NULL != kmem->space->heads[order] ) {
        kmem->space->heads[order]->prev = NULL;
    }

    /* Validate all the pages are not used*/
    for ( i = 0; i < (1LL << order); i++ ) {
        if ( !KMEM_IS_FREE(&pg[i]) ) {
            return NULL;
        }
    }

    /* Mark the contiguous pages as "used" */
    for ( i = 0; i < (1LL << order); i++ ) {
        pg[i].flags |= KMEM_USED;
    }

    /* Return the first superpage of the allocated memory */
    return pg;
}

/*
 * Release pages
 */
void
kmem_return_pages(struct kmem *kmem, struct kmem_page *pg)
{
    int order;
    ssize_t i;

    /* Get the order */
    order = pg->order;

    /* Check the argument of pages is aligned */
    if ( (pg - kmem->space->pages) & ((1ULL << order) - 1) ) {
        /* Not aligned */
        return;
    }

    /* Check the flags and order of all the pages */
    for ( i = 0; i < (1LL << order); i++ ) {
        if ( order != pg[i].order || KMEM_IS_FREE(&pg[i]) ) {
            /* Invalid order or not used */
            return;
        }
    }

    /* Unmark "used" */
    for ( i = 0; i < (1LL << order); i++ ) {
        pg[i].flags &= ~KMEM_USED;
    }

    /* Return the released pages to the buddy */
    pg->prev = NULL;
    pg->next = kmem->space->heads[order];
    if ( NULL != kmem->space->heads[order] ) {
        kmem->space->heads[order]->prev = pg;
    }

    /* Merge buddies if possible */
    _kmem_buddy_pg_merge(kmem, pg, order);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
