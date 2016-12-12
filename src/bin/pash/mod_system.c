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

#include <stdio.h>
#include "pash.h"

int
pash_module_system_help(struct pash *pash, char *args[])
{
    return 0;
}

int
pash_module_system_request(struct pash *pash, char *args[])
{
    return 0;
}

int
pash_module_system_show(struct pash *pash, char *args[])
{
    printf("Displaying the running system information \n");
    printf("pix version: %s\n", PIX_VERSION);

    return 0;
}

static char *pash_module_system_name = "system";
static struct pash_module_api pash_module_system_api = {
    .clear = NULL,
    .help = &pash_module_system_help,
    .request = &pash_module_system_request,
    .show = &pash_module_system_show,
};


/*
 * Initialize
 */
int
pash_module_system_init(struct pash *pash)
{
    return pash_register_module(pash, pash_module_system_name,
                                &pash_module_system_api);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
