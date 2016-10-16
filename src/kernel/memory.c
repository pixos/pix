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


/* Prototype declarations of static functions */
static void * _kmalloc_slab(struct kmem *, size_t, int);
static void * _kmalloc_slab_partial(struct kmem *, size_t, int);
static void * _kmalloc_slab_free(struct kmem *, size_t, int);
static void * _kmalloc_slab_new(struct kmem *, size_t, int);
static void * _kmalloc_pages(struct kmem *, size_t, int);

/*
 * Allocate physical memory space
 *
 * SYNOPSIS
 *      reg_t
 *      pgalloc(int zone, size_t nr);
 *
 * DESCRIPTION
 *      The pgalloc() function allocates nr pages of contiguous physical memory.
 *
 * RETURN VALUES
 *      The pgalloc() function returns a physical address to allocated memory.
 *      If there is an error, it returns 0.
 */
reg_t
pgalloc(int zone, size_t nr)
{
    reg_t addr;

    addr = 0;
    if ( nr < (1 << SP_SHIFT) ) {
        addr = (reg_t)pmem_prim_alloc_pages(zone, 0);
    } else {
        addr = (reg_t)pmem_prim_alloc_pages(zone, bitwidth(nr >> SP_SHIFT));
    }

    return addr;
}


/*
 * Allocate memory space.
 * Note that the current implementation does not protect the slab header, so
 * the allocated memory must be carefully used.
 *
 * SYNOPSIS
 *      void *
 *      kmalloc(size_t size);
 *
 * DESCRIPTION
 *      The kmalloc() function allocates size bytes of contiguous memory.
 *
 * RETURN VALUES
 *      The kmalloc() function returns a pointer to allocated memory.  If there
 *      is an error, it returns NULL.
 */
void *
kmalloc(size_t size)
{
    size_t o;

    /* Get the bit-width of the size argument */
    o = bitwidth(size);

    if ( o < KMEM_SLAB_BASE_ORDER + KMEM_SLAB_ORDER ) {
        /* Slab */
        if ( o < KMEM_SLAB_BASE_ORDER ) {
            o = 0;
        } else {
            o = o - KMEM_SLAB_BASE_ORDER;
        }
        return _kmalloc_slab(g_kmem, o, PMEM_ZONE_LOWMEM);
    } else {
        /* Pages */
        return _kmalloc_pages(g_kmem, size, PMEM_ZONE_LOWMEM);
    }
}

/*
 * Allocate memory from the slab allocator
 */
static void *
_kmalloc_slab(struct kmem *kmem, size_t o, int zone)
{
    void *ptr;

    /* Ensure that the order is less than the maximum configured order */
    if ( o >= KMEM_SLAB_ORDER ) {
        return NULL;
    }

    /* Lock */
    spin_lock(&kmem->slab_lock);

    /* Small object: Slab allocator */
    if ( NULL != kmem->slab.gslabs[zone][o].partial ) {
        /* Partial list is available. */
        ptr = _kmalloc_slab_partial(kmem, o, zone);
    } else if ( NULL != kmem->slab.gslabs[zone][o].free ) {
        /* Partial list is empty, but free list is available. */
        ptr = _kmalloc_slab_free(kmem, o, zone);
    } else {
        /* No free space, then allocate new page for slab objects */
        ptr = _kmalloc_slab_new(kmem, o, zone);
     }

    /* Unlock */
    spin_unlock(&kmem->slab_lock);

    return ptr;
}

/*
 * Allocate memory from partial slab
 */
static void *
_kmalloc_slab_partial(struct kmem *kmem, size_t o, int zone)
{
    struct kmem_slab *hdr;
    void *ptr;
    ssize_t i;

    /* Partial list is available. */
    hdr = kmem->slab.gslabs[zone][o].partial;
    ptr = (void *)((reg_t)hdr->obj_head + hdr->free
                   * (1ULL << (o + KMEM_SLAB_BASE_ORDER)));
    hdr->marks[hdr->free] = 1;
    hdr->nused++;
    if ( hdr->nr <= hdr->nused ) {
        /* Becomes full */
        hdr->free = -1;
        kmem->slab.gslabs[zone][o].partial = hdr->next;
        /* Prepend to the full list */
        hdr->next = kmem->slab.gslabs[zone][o].full;
        kmem->slab.gslabs[zone][o].full = hdr;
    } else {
        /* Search free space for the next allocation */
        for ( i = 0; i < hdr->nr; i++ ) {
            if ( 0 == hdr->marks[i] ) {
                hdr->free = i;
                break;
            }
        }
    }

    return ptr;
}

/*
 * Allocate memory from free slab list
 */
static void *
_kmalloc_slab_free(struct kmem *kmem, size_t o, int zone)
{
    struct kmem_slab *hdr;
    void *ptr;
    ssize_t i;

    /* Partial list is empty, but free list is available. */
    hdr = kmem->slab.gslabs[zone][o].free;
    ptr = (void *)((reg_t)hdr->obj_head + hdr->free
                   * (1ULL << (o + KMEM_SLAB_BASE_ORDER)));
    hdr->marks[hdr->free] = 1;
    hdr->nused++;
    if ( hdr->nr <= hdr->nused ) {
        /* Becomes full */
        hdr->free = -1;
        kmem->slab.gslabs[zone][o].partial = hdr->next;
        /* Prepend to the full list */
        hdr->next = kmem->slab.gslabs[zone][o].full;
        kmem->slab.gslabs[zone][o].full = hdr;
    } else {
        /* Prepend to the partial list */
        hdr->next = kmem->slab.gslabs[zone][o].partial;
        kmem->slab.gslabs[zone][o].partial = hdr;
        /* Search free space for the next allocation */
        for ( i = 0; i < hdr->nr; i++ ) {
            if ( 0 == hdr->marks[i] ) {
                hdr->free = i;
                break;
            }
        }
    }

    return ptr;
}

/*
 * Allocate memory from a new slab objects
 */
static void *
_kmalloc_slab_new(struct kmem *kmem, size_t o, int zone)
{
    struct kmem_slab *hdr;
    struct kmem_page *page;
    void *ptr;
    ssize_t i;
    size_t s;
    size_t nr;
    int idx;

    /* No free space, then allocate new page for slab objects */
    s = (1ULL << (o + KMEM_SLAB_BASE_ORDER + KMEM_SLAB_NR_OBJ_ORDER))
        + sizeof(struct kmem_slab);
    /* Align the page to fit to the buddy system, and get the order */
    nr = DIV_CEIL(s, SUPERPAGESIZE);
    /* Allocate pages */
    hdr = kmem_alloc_pages(kmem, nr, zone);
    if ( NULL == hdr ) {
        return NULL;
    }
    /* Page */
    idx = SUPERPAGE_INDEX((void *)hdr - kmem->space->start);
    page = &kmem->space->pages[idx];
    /* Calculate the number of slab objects in this block; N.B., + 1 in the
       denominator is the `marks' for each objects. */
    hdr->nr = (nr * SUPERPAGESIZE - sizeof(struct kmem_slab))
        / ((1ULL << (o + KMEM_SLAB_BASE_ORDER)) + 1);
    /* Set the size of each object */
    hdr->size = (1ULL << (o + KMEM_SLAB_BASE_ORDER));
    /* Pointer to the free list */
    hdr->free_list = &kmem->slab.gslabs[zone][o];
    /* Set the name */
    hdr->name = NULL;
    /* Reset counters */
    hdr->nused = 0;
    hdr->free = 0;
    /* Set the address of the first slab object */
    hdr->obj_head = (void *)((u64)hdr + (nr * SUPERPAGESIZE)
                             - ((1ULL << (o + KMEM_SLAB_BASE_ORDER))
                                * hdr->nr));
    /* Reset marks and next cache */
    kmemset(hdr->marks, 0, hdr->nr);
    hdr->next = NULL;

    /* Retrieve a slab */
    ptr = (void *)((u64)hdr->obj_head + hdr->free
                   * (1ULL << (o + KMEM_SLAB_BASE_ORDER)));
    hdr->marks[hdr->free] = 1;
    hdr->nused++;

    if ( hdr->nr <= hdr->nused ) {
        /* Becomes full */
        hdr->free = -1;
        kmem->slab.gslabs[zone][o].partial = hdr->next;
        /* Prepend to the full list */
        hdr->next = kmem->slab.gslabs[zone][o].full;
        kmem->slab.gslabs[zone][o].full = hdr;
    } else {
        /* Prepend to the partial list */
        hdr->next = kmem->slab.gslabs[zone][o].partial;
        kmem->slab.gslabs[zone][o].partial = hdr;
        /* Search free space for the next allocation */
        for ( i = 0; i < hdr->nr; i++ ) {
            if ( 0 == hdr->marks[i] ) {
                hdr->free = i;
                break;
            }
        }
    }

    /* Set the slab to page management data structure */
    page->slab = hdr;
    page->flags |= KMEM_SLAB;

    return ptr;
}

/*
 * Allocate memory from the slab allocator
 */
static void *
_kmalloc_pages(struct kmem *kmem, size_t size, int zone)
{
    void *ptr;

    /* Lock */
    spin_lock(&kmem->slab_lock);

    /* Large object: Page allocator */
    ptr = kmem_alloc_pages(kmem, DIV_CEIL(size, SUPERPAGESIZE), zone);

    /* Unlock */
    spin_unlock(&kmem->slab_lock);

    return ptr;
}

/*
 * Deallocate memory space pointed by ptr
 *
 * SYNOPSIS
 *      void
 *      kfree(void *ptr);
 *
 * DESCRIPTION
 *      The kfree() function deallocates the memory allocation pointed by ptr.
 *
 * RETURN VALUES
 *      The kfree() function does not return a value.
 */
void
kfree(void *ptr)
{
    ssize_t i;
    int found;
    u64 asz;
    struct kmem_slab *hdr;
    //struct kmem_slab **hdrp;
    struct kmem_page *page;
    int idx;

    /* Lock */
    spin_lock(&g_kmem->slab_lock);

    if ( 0 == ((u64)ptr % SUPERPAGESIZE) ) {
        /* Free pages */
        kmem_free_pages(g_kmem, ptr);
    } else {
        /* Resolve the slab data structure from the given virtual address */
        idx = SUPERPAGE_INDEX(ptr - g_kmem->space->start);
        page = &g_kmem->space->pages[idx];
        hdr = page->slab;

        /* The size of the object */
        asz = hdr->size;

        found = -1;
        for ( i = 0; i < hdr->nr; i++ ) {
            if ( ptr == (void *)((u64)hdr->obj_head + i * asz) ) {
                /* Found */
                found = i;
                break;
            }
        }
        if ( found >= 0 ) {
            hdr->nused--;
            /* Unmark the found object */
            hdr->marks[found] = 0;
            /* Update the last freed index */
            hdr->free = found;
            if ( hdr->nused <= 0 ) {
                /* If all the objects in this slab becomes free, move it to the
                   free list */
                //*hdrp = hdr->next;
                hdr->next = hdr->free_list->free;
                hdr->free_list->free = hdr;
            }
            spin_unlock(&g_kmem->slab_lock);
            return;
        }
    }

    /* Unlock */
    spin_unlock(&g_kmem->slab_lock);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
