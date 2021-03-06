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

#ifndef _PASH_H
#define _PASH_H

#ifndef PIX_VERSION
#define PIX_VERSION     "unknown"
#endif

#define PASH_MAX_ARGS   4096

#include <stdlib.h>

struct pash;

struct pash_module_func {
    void (*func)(void);
    char *desc;
};
struct pash_module_api {
    int (*clear)(struct pash *pash, char *args[]);
    int (*help)(struct pash *pash, char *args[]);
    int (*request)(struct pash *pash, char *args[]);
    int (*show)(struct pash *pash, char *args[]);
};

struct pash_module {
    char *name;
    struct pash_module_api api;
    /* Pointer to the next module */
    struct pash_module *next;
    /* Pointer to the next module used to partial command search */
    struct pash_module *work_next;
};

/*
 * Command
 */
enum pash_builtin_command {
    PASH_BUILTIN_CLEAR,
    PASH_BUILTIN_HELP,
    PASH_BUILTIN_REQUEST,
    PASH_BUILTIN_SHOW,
};
struct pash_command {
    enum pash_builtin_command type;
    char *name;
    char *desc;
    struct pash_command *work_next;
};

/*
 * pix advanced shell
 */
struct pash {
    struct pash_module *modules;
};


/* Prototype declaration */
int pash_register_module(struct pash *, const char *, struct pash_module_api *);

#endif /* _PASH_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
