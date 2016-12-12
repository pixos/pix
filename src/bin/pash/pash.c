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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/pix.h>
#include "pash.h"

static struct pash_command pash_cmds[] = {
    {
        .type = PASH_BUILTIN_CLEAR,
        .name = "clear",
        .desc = "Reset functions",
    },
    {
        .type = PASH_BUILTIN_HELP,
        .name = "help",
        .desc = "Display brief help information",
    },
    {
        .type = PASH_BUILTIN_SHOW,
        .name = "show",
        .desc = "Display running system information",
    },
};

/*
 * Search the candidate modules
 */
static struct pash_module *
_search_candidate_module(struct pash *pash, const char *name)
{
    struct pash_module *m;
    struct pash_module *mhead;
    struct pash_module *mtail;
    size_t len;

    len = strlen(name);
    m = pash->modules;
    mhead = NULL;
    mtail = NULL;
    while ( NULL != m ) {
        if ( 0 == strcmp(name, m->name) ) {
            /* Exact match */
            m->work_next = NULL;
            return m;
        } else if ( 0 == strncmp(name, m->name, len) ) {
            /* Prefix match */
            if ( NULL == mtail ) {
                m->work_next = NULL;
                mhead = mtail = m;
            } else {
                m->work_next = NULL;
                mtail->work_next = m;
            }
        }
        m = m->next;
    }

    return mhead;
}

/*
 * Search the candidate commands
 */
static struct pash_command *
_search_candidate_command(struct pash *pash, const char *name)
{
    struct pash_command *cmd;
    struct pash_command *chead;
    struct pash_command *ctail;
    size_t len;
    int i;
    int nr;

    len = strlen(name);
    chead = NULL;
    ctail = NULL;
    nr = (int)(sizeof(pash_cmds) / sizeof(struct pash_command));
    for ( i = 0; i < nr; i++ ) {
        cmd = &pash_cmds[i];
        if ( 0 == strcmp(name, cmd->name) ) {
            /* Exact match */
            cmd->work_next = NULL;
            return cmd;
        } else if ( 0 == strncmp(name, cmd->name, len) ) {
            /* Prefix match */
            if ( NULL == ctail ) {
                cmd->work_next = NULL;
                chead = ctail = cmd;
            } else {
                cmd->work_next = NULL;
                ctail->work_next = cmd;
            }
        }

    }

    return chead;
}

/*
 * Register module
 */
int
pash_register_module(struct pash *pash, const char *name,
                     struct pash_module_api *api)
{
    struct pash_module *mod;
    struct pash_module *m;
    struct pash_module **mp;

    /* Allocate for this module */
    mod = malloc(sizeof(struct pash_module));
    if ( NULL == mod ) {
        return -1;
    }
    /* Copy the name of the module */
    mod->name = strdup(name);
    if ( NULL == mod->name ) {
        free(mod);
        return -1;
    }
    /* Copy the API */
    memcpy(&mod->api, api, sizeof(struct pash_module_api));

    /* Insert the module */
    m = pash->modules;
    mp = &pash->modules;
    while ( NULL != m ) {
        if ( strcmp(m->name, mod->name) > 0 ) {
            *mp = mod;
            mod->next = m;
            return 0;
        }
        mp = &m->next;
        m = m->next;
    }
    mod->next = *mp;         /* NULL */
    *mp = mod;

    return 0;
}

int
pash_builtin_clear(struct pash *pash, char *args[])
{
    return 0;
}

int
pash_builtin_help(struct pash *pash, char *args[])
{
    int i;
    int nr;

    nr = (int)(sizeof(pash_cmds) / sizeof(struct pash_command));
    for ( i = 0; i < nr; i++ ) {
        printf("%.12s %s\n", pash_cmds[i].name, pash_cmds[i].desc);
    }

    return 0;
}

int
pash_builtin_show(struct pash *pash, char *args[])
{
    const char *name;
    struct pash_module *m;

    /* Get the second argument for the module name */
    name = args[1];

    m = _search_candidate_module(pash, name);
    if ( NULL == m ) {
        fprintf(stderr, "pash: %s : module not found: %s\n", args[0], name);
        return -1;
    }
    if ( NULL == m->work_next ) {
        /* Matches then execute */
        if ( NULL != m->api.show ) {
            return m->api.show(pash, args);
        } else {
            fprintf(stderr, "pash: %s : module not found: %s\n", args[0], name);
            return -1;
        }
    } else {
        fprintf(stderr, "pash: %s : multiple modules match: %s\n", args[0],
                name);
        while ( NULL != m ) {
            printf("%s\n", m->name);
            m = m->work_next;
        }
        return -1;
    }
}

/*
 * Execute a line of command
 */
int
pash_execute(struct pash *pash, const char *cmd)
{
    char *s;
    char *args[PASH_MAX_ARGS];
    char *arg;
    int nr;
    int ret;
    struct pash_command *c;

    /* Copy the command */
    s = strdup(cmd);

    /* Parse the arguments */
    nr = 0;
    while ( NULL != s ) {
        arg = strsep(&s, " \n");
        if ( strlen(arg) > 0 ) {
            args[nr] = arg;
            nr++;
            if ( nr >= PASH_MAX_ARGS - 1 ) {
                /* Exceeding the limitation of arguments */
                return -1;
            }
        }
    }
    /* Terminate by NULL */
    args[nr] = NULL;

    if ( 0 == nr ) {
        /* No command specified, then immedicately return */
        return 0;
    }

    if ( 0 == strcmp("?", args[0]) ) {
        ret = pash_builtin_help(pash, args);
        /* Release the command */
        free(s);
        return ret;

    }

    /* Search candidate commands */
    c = _search_candidate_command(pash, args[0]);
    if ( NULL == c ) {
        fprintf(stderr, "pash: command not found: %s\n", args[0]);
        ret = -1;
    } else if ( NULL == c->work_next ) {
        /* Single candidate found */
        if ( 0 == strcmp("help", c->name) ) {
            ret = pash_builtin_help(pash, args);
        } else if ( 0 == strcmp("show", c->name) ) {
            ret = pash_builtin_show(pash, args);
        } else {
            ret = -1;
        }
    } else {
        /* Multiple command candidates found */
        fprintf(stderr, "pash: multiple modules match for %s\n", args[0]);
        while ( NULL != c ) {
            printf("%s\n", c->name);
            c = c->work_next;
        }
        ret = -1;
    }

    /* Release the command */
    free(s);

    return ret;
}

/* Modules */
int pash_module_clock_init(struct pash *);
int pash_module_cpu_init(struct pash *);

/*
 * Entry point for pash
 */
int
main(int argc, char *argv[])
{
    char buf[256];
    struct pash *pash;

    /* Print out welcome message */
    printf("Welcome to Packet Information Chaining Service (pix)\n");
    printf("pix version: %s\n", PIX_VERSION);

    /* Allocate pash management data structure */
    pash = malloc(sizeof(struct pash));
    if ( NULL == pash ) {
        fprintf(stderr, "Failed to launch pash.\n");
        return EXIT_FAILURE;
    }
    pash->modules = NULL;

    /* Load modules (but currently not loadable...) */
    pash_module_clock_init(pash);
    pash_module_cpu_init(pash);


    putchar('>');
    putchar(' ');

    while ( 1 ) {
        if ( fgets(buf, sizeof(buf), stdin) ) {
            pash_execute(pash, buf);
            putchar('>');
            putchar(' ');
        }
    }

    exit(0);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
