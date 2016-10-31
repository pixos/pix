/*_
 * Copyright (c) 2016 Hirochika Asai <asai@jar.jp>
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

#ifndef _FDB_H
#define _FDB_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

#define FDB_KEY_SIZE    8
#define FDB_MAX_ENTRIES 4096
#define FDB_AGING_TSC   (300ULL * 1000000000)

/*
 * Entry
 */
struct fdb_entry {
    /* Key */
    uint8_t key[FDB_KEY_SIZE];
    /* Destination port; -1 for broadcast/multicast */
    int port;
    /* Aging (accessed time in TSC) */
    uint64_t aging;
    /* Linked-list */
    struct fdb_entry *next;
    struct fdb_entry *prev;     /* for fdb.entries */
};

/*
 * Forwarding database
 */
struct fdb {
    /* Current version: Read-only */
    struct hopscotch_hash_table *cur;
    /* Update version: Read-write */
    struct hopscotch_hash_table *update;

    /* Entries */
    struct fdb_entry *entries;
    /* Pool */
    struct fdb_entry *pool;
};

/*
 * Read time stamp counter
 */
static __inline__ uint64_t
fdb_rdtsc(void)
{
    uint64_t a;
    uint64_t d;

    __asm__ __volatile__ ("rdtsc" : "=a" (a), "=d" (d));

    return (d << 32) | a;
}

/*
 * Initialize the forwarding database
 */
static __inline__ void *
fdb_init(void)
{
    struct fdb *fdb;
    struct fdb_entry *e;
    ssize_t i;

    /* Allocate memory space for forwarding database */
    fdb = malloc(sizeof(struct fdb));
    if ( NULL == fdb ) {
        return NULL;
    }
    fdb->cur = hopscotch_init(NULL, FDB_KEY_SIZE);
    if ( NULL == fdb->cur ) {
        free(fdb);
        return NULL;
    }
    fdb->update = hopscotch_init(NULL, FDB_KEY_SIZE);
    if ( NULL == fdb->update ) {
        free(fdb->cur);
        free(fdb);
        return NULL;
    }

    /* Allocate for entries */
    e = malloc(sizeof(struct fdb_entry) * FDB_MAX_ENTRIES);
    for ( i = 0; i < FDB_MAX_ENTRIES - 1; i++ ) {
        e[i].next = &e[i + 1];
    }
    e[i].next = NULL;

    /* Set them to the pool */
    fdb->pool = e;

    /* Initialize entries */
    fdb->entries = NULL;

    return fdb;
}

/*
 * Garbage collection
 */
static __inline__ void
fdb_gc(struct fdb *fdb)
{
    struct hopscotch_hash_table *h;
    struct fdb_entry *e;
    struct fdb_entry *rem;
    uint64_t curtsc;

    curtsc = fdb_rdtsc();
    e = fdb->entries;
    rem = NULL;
    while ( NULL != e ) {
        if ( curtsc - e->aging > FDB_AGING_TSC ) {
            /* Remove from the hash table */
            hopscotch_remove(fdb->update, e->key);
            /* Remove from the list of entries */
            if ( NULL == e->prev ){
                fdb->entries = e->next;
            } else {
                e->prev->next = e->next;
            }
            /* Add this entry to the list of entries to be removed */
            e->next = rem;
            rem = e;
        }
        e = e->next;
    }

    h = fdb->cur;
    fdb->cur = fdb->update;
    fdb->update = h;

    __sync_synchronize();

    /* Remove the old entries */
    while ( NULL != rem ) {
        e = rem;
        /* Remove from the hash table */
        hopscotch_remove(fdb->update, e->key);
        rem = e->next;
        /* Release this entry */
        e->next = fdb->pool;
        fdb->pool = e;
    }
}

/*
 * Release the forwarding database
 */
static __inline__ void
fdb_release(struct fdb *fdb)
{
    hopscotch_release(fdb->cur);
    hopscotch_release(fdb->update);
    free(fdb);
}

/*
 * Lookup
 */
static __inline__ void *
fdb_lookup(struct fdb *fdb, uint8_t *key)
{
    return hopscotch_lookup(fdb->cur, key);
}

static __inline__ int
fdb_update(struct fdb *fdb, uint8_t *key, int port)
{
    struct fdb_entry *found;
    struct hopscotch_hash_table *h;

    /* Search the data */
    found = hopscotch_lookup(fdb->cur, key);
    if ( NULL != found ) {
        /* Update the entry */
        found->port = port;
        found->aging = fdb_rdtsc();
    } else {
        /* New entry */
        found = fdb->pool;
        if ( NULL == found ) {
            /* No more entry available */
            return -1;
        }
        fdb->pool = found->next;

        /* Build an entry for this request */
        memcpy(found->key, key, FDB_KEY_SIZE);
        found->port = port;
        found->aging = fdb_rdtsc();
        found->prev = NULL;
        found->next = fdb->entries;
        if ( NULL != fdb->entries ) {
            fdb->entries->prev = found;
        }
        fdb->entries = found;

        /* Insert this to the hash table */
        hopscotch_insert(fdb->update, key, found);

        /* Replace the current hash table with the updated one */
        h = fdb->cur;
        fdb->cur = fdb->update;
        fdb->update = h;

        __sync_synchronize();

        /* Insert this to the hash table */
        hopscotch_insert(fdb->update, key, found);
    }

    return 0;
}

static __inline__ void
fdb_debug(struct fdb *fdb)
{
    struct fdb_entry *e;

    printf("Current FDB:\n");
    e = fdb->entries;
    while ( NULL != e ) {
        printf("%02x%02x.%02x%02x.%02x%02x => Port #%d\n",
               e->key[0], e->key[1], e->key[2], e->key[3], e->key[4], e->key[5],
               e->port);
        e = e->next;
    }
}

#endif /* _FDB_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
