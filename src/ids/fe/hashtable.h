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

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HOPSCOTCH_INIT_BSIZE_FACTOR     10
#define HOPSCOTCH_HOPINFO_SIZE          32

struct hopscotch_bucket {
    uint8_t *key;
    void *data;
    uint32_t hopinfo;
    uint32_t rsvd;
};
struct hopscotch_hash_table {
    size_t pfactor;
    size_t keylen;
    struct hopscotch_bucket *buckets;
    int _allocated;
};

/*
 * Jenkins Hash Function
 */
static __inline__ uint32_t
_jenkins_hash(uint8_t *key, size_t len)
{
    uint32_t hash;
    size_t i;

    hash = 0;
    for ( i = 0; i < len; i++ ) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/*
 * Initialize hash table
 */
static __inline__ struct hopscotch_hash_table *
hopscotch_init(struct hopscotch_hash_table *ht, size_t keylen)
{
    int pfactor;
    struct hopscotch_bucket *buckets;

    /* Allocate buckets first */
    pfactor = HOPSCOTCH_INIT_BSIZE_FACTOR;
    buckets = malloc(sizeof(struct hopscotch_bucket) * (1 << pfactor));
    if ( NULL == buckets ) {
        return NULL;
    }
    memset(buckets, 0, sizeof(struct hopscotch_bucket) * (1 << pfactor));

    if ( NULL == ht ) {
        ht = malloc(sizeof(struct hopscotch_hash_table));
        if ( NULL == ht ) {
            return NULL;
        }
        ht->_allocated = 1;
    } else {
        ht->_allocated = 0;
    }
    ht->pfactor = pfactor;
    ht->buckets = buckets;
    ht->keylen = keylen;

    return ht;
}

/*
 * Release the hash table
 */
static __inline__ void
hopscotch_release(struct hopscotch_hash_table *ht)
{
    free(ht->buckets);
    if ( ht->_allocated ) {
        free(ht);
    }
}

/*
 * Lookup
 */
static __inline__ void *
hopscotch_lookup(struct hopscotch_hash_table *ht, uint8_t *key)
{
    uint32_t h;
    size_t idx;
    size_t i;
    size_t sz;

    sz = 1ULL << ht->pfactor;
    h = _jenkins_hash(key, ht->keylen);
    idx = h & (sz - 1);

    if ( !ht->buckets[idx].hopinfo ) {
        return NULL;
    }
    for ( i = 0; i < HOPSCOTCH_HOPINFO_SIZE; i++ ) {
        if ( ht->buckets[idx].hopinfo & (1 << i) ) {
            if ( 0 == memcmp(key, ht->buckets[idx + i].key, ht->keylen) ) {
                /* Found */
                return ht->buckets[idx + i].data;
            }
        }
    }

    return NULL;
}


/*
 * Insert an entry to the hash table
 */
static __inline__ int
hopscotch_insert(struct hopscotch_hash_table *ht, uint8_t *key, void *data)
{
    uint32_t h;
    size_t idx;
    size_t i;
    size_t sz;
    size_t off;
    size_t j;

    /* Ensure the key does not exist.  Duplicate keys are not allowed. */
    if ( NULL != hopscotch_lookup(ht, key) ) {
        /* The key already exists. */
        return -1;
    }

    sz = 1ULL << ht->pfactor;
    h = _jenkins_hash(key, ht->keylen);
    idx = h & (sz - 1);

    /* Linear probing to find an empty bucket */
    for ( i = idx; i < sz; i++ ) {
        if ( NULL == ht->buckets[i].key ) {
            /* Found an available bucket */
            while ( i - idx >= HOPSCOTCH_HOPINFO_SIZE ) {
                for ( j = 1; j < HOPSCOTCH_HOPINFO_SIZE; j++ ) {
                    if ( ht->buckets[i - j].hopinfo ) {
                        off = __builtin_ctz(ht->buckets[i - j].hopinfo);
                        if ( off >= j ) {
                            continue;
                        }
                        ht->buckets[i].key = ht->buckets[i - j + off].key;
                        ht->buckets[i].data = ht->buckets[i - j + off].data;
                        ht->buckets[i - j + off].key = NULL;
                        ht->buckets[i - j + off].data = NULL;
                        ht->buckets[i - j].hopinfo &= ~(1ULL << off);
                        ht->buckets[i - j].hopinfo |= (1ULL << j);
                        i = i - j + off;
                        break;
                    }
                }
                if ( j >= HOPSCOTCH_HOPINFO_SIZE ) {
                    return -1;
                }
            }

            off = i - idx;
            ht->buckets[i].key = key;
            ht->buckets[i].data = data;
            ht->buckets[idx].hopinfo |= (1ULL << off);

            return 0;
        }
    }

    return -1;
}

/*
 * Remove an item
 */
static __inline__ void *
hopscotch_remove(struct hopscotch_hash_table *ht, uint8_t *key)
{
    uint32_t h;
    size_t idx;
    size_t i;
    size_t sz;
    void *data;

    sz = 1ULL << ht->pfactor;
    h = _jenkins_hash(key, ht->keylen);
    idx = h & (sz - 1);

    if ( !ht->buckets[idx].hopinfo ) {
        return NULL;
    }
    for ( i = 0; i < HOPSCOTCH_HOPINFO_SIZE; i++ ) {
        if ( ht->buckets[idx].hopinfo & (1 << i) ) {
            if ( 0 == memcmp(key, ht->buckets[idx + i].key, ht->keylen) ) {
                /* Found */
                data = ht->buckets[idx + i].data;
                ht->buckets[idx].hopinfo &= ~(1ULL << i);
                ht->buckets[idx + i].key = NULL;
                ht->buckets[idx + i].data = NULL;
                return data;
            }
        }
    }

    return NULL;
}

/*
 * Resize the bucket size of the hash table
 */
static __inline__ int
hopscotch_resize(struct hopscotch_hash_table *ht, int delta)
{
    size_t sz;
    size_t opfactor;
    size_t npfactor;
    ssize_t i;
    struct hopscotch_bucket *nbuckets;
    struct hopscotch_bucket *obuckets;
    int ret;

    opfactor = ht->pfactor;
    npfactor = ht->pfactor + delta;
    sz = 1ULL << npfactor;

    nbuckets = malloc(sizeof(struct hopscotch_bucket) * sz);
    if ( NULL == nbuckets ) {
        return -1;
    }
    memset(nbuckets, 0, sizeof(struct hopscotch_bucket) * sz);
    obuckets = ht->buckets;

    ht->buckets = nbuckets;
    ht->pfactor = npfactor;

    for ( i = 0; i < (1LL << opfactor); i++ ) {
        if ( obuckets[i].key ) {
            ret = hopscotch_insert(ht, obuckets[i].key, obuckets[i].data);
            if ( ret < 0 ) {
                ht->buckets = obuckets;
                ht->pfactor = opfactor;
                free(nbuckets);
                return -1;
            }
        }
    }
    free(obuckets);

    return 0;
}




#endif /* _HASHTABLE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
