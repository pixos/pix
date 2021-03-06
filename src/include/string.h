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

#ifndef _STRING_H
#define _STRING_H

#include <aos/types.h>

size_t strlen(const char *);
char * strncpy(char *__restrict__, const char *__restrict__, size_t);
size_t strlcpy(char *__restrict__, const char *__restrict__, size_t);
char * strcpy(char *, const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
char * strchr(const char *, int);
char * strrchr(const char *, int);
char * strtok(char *__restrict__, const char *__restrict__);
char * strsep(char **, const char *);
char * strdup(const char *);

void * memset(void *, int, size_t);
void * memcpy(void *__restrict, const void *__restrict, size_t);
void * memmove(void *, const void *, size_t);

int memcmp(const void *, const void *, size_t);

#endif /* _STRING_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
