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
#include <sys/syscall.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

typedef __builtin_va_list va_list;
#define va_start(ap, last)      __builtin_va_start((ap), (last))
#define va_arg                  __builtin_va_arg
#define va_end(ap)              __builtin_va_end(ap)
#define va_copy(dest, src)      __builtin_va_copy((dest), (src))
#define alloca(size)            __builtin_alloca((size))

/* in libcasm.s */
unsigned long long syscall(int, ...);

/*
 * exit
 */
void
exit(int status)
{
    syscall(SYS_exit, status);

    /* Infinite loop to prevent the warning: 'noreturn' function does return */
    while ( 1 ) {}
}

/*
 * fork
 */
pid_t
fork(void)
{
    return syscall(SYS_fork);
}

/*
 * read
 */
ssize_t
read(int fildes, void *buf, size_t nbyte)
{
    return syscall(SYS_read, fildes, buf, nbyte);
}

/*
 * write
 */
ssize_t
write(int fildes, const void *buf, size_t nbyte)
{
    return syscall(SYS_write, fildes, buf, nbyte);
}

/*
 * open
 */
int
open(const char *path, int oflag, ...)
{
    va_list ap;
    int ret;

    va_start(ap, oflag);
    ret = syscall(SYS_open, path, oflag, ap);
    va_end(ap);

    return ret;
}

/*
 * close
 */
int
close(int fildes)
{
    return syscall(SYS_close, fildes);
}

/*
 * waitpid
 */
pid_t
waitpid(pid_t pid, int *stat_loc, int options)
{
    return syscall(SYS_wait4, pid, stat_loc, options, NULL);
}

/*
 * execve
 */
int
execve(const char *path, char *const argv[], char *const envp[])
{
    return syscall(SYS_execve, path, argv, envp);
}

/*
 * mmap
 */
void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    return (void * )syscall(SYS_mmap, addr, len, prot, flags, fd, offset);
}

/*
 * munmap
 */
int
munmap(void *addr, size_t len)
{
    return syscall(SYS_munmap, addr, len);
}

/*
 * getpid
 */
pid_t
getpid(void)
{
    return syscall(SYS_getpid);
}

/*
 * getppid
 */
pid_t
getppid(void)
{
    return syscall(SYS_getppid);
}

/*
 * lseek
 */
off_t
lseek(int fildes, off_t offset, int whence)
{
    return syscall(SYS_lseek, fildes, offset, whence);
}

/*
 * malloc
 */
void *
malloc(size_t size)
{
    void *ptr;
    size_t aligned_size;

    aligned_size = ((((size) - 1) / (1ULL << 21) + 1) * (1ULL << 21));
    ptr = mmap(NULL, aligned_size, PROT_READ | PROT_WRITE,
               MAP_ANON | MAP_PRIVATE, -1, 0);

    return ptr;
}

/*
 * calloc
 */
void *
calloc(size_t count, size_t size)
{
    void *ptr;
    ssize_t i;

    ptr = malloc(count * size);
    for ( i = 0; i < (ssize_t)(count * size); i++ ) {
        *(uint8_t *)(ptr + i) = 0;
    }

    return ptr;
}

/*
 * free
 */
void
free(void *ptr)
{
    //munmap(ptr, len);
}

/*
 * ioctl
 */
int
ioctl(int fildes, unsigned long request, ...)
{
    return syscall(SYS_ioctl, fildes, request);
}

/*
 * fcntl
 */
int
fcntl(int fildes, int cmd, ...)
{
    return syscall(SYS_fcntl, fildes, cmd);
}

/*
 * nanosleep
 */
int
nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
    return syscall(SYS_nanosleep, rqtp, rmtp);
}

/*
 * Write zeros to a byte string
 *
 * SYNOPSIS
 *      void
 *      bzero(void *s, size_t n);
 *
 * DESCRIPTION
 *      The bzero() function writes n zeroed bytes to the string s.  If n is
 *      zero, bzero() does nothing.
 *
 */
void
bzero(void *s, size_t n)
{
    memset(s, 0, n);
}

/*
 * Convert ASCII string to integer
 *
 * SYNOPSIS
 *      int
 *      atoi(const char *str);
 *
 * DESCRIPTION
 *      The atoi() function converts the initial portion of the string pointed
 *      to by str to int representation.
 */
int
atoi(const char *str)
{
    int ret;

    ret = 0;
    while  ( *str >= '0' && *str <= '9' ) {
        ret *= 10;
        ret += *str - '0';
        str++;
    }

    return ret;
}

/*
 * Architecture-specific system call
 *
 * SYNOPSIS
 *      int
 *      sysarch(int number, void *args);
 *
 * DESCRIPTION
 *      The sysarch() function calls the architecture-specific system call.
 */
int
sysarch(int number, void *args)
{
    return syscall(SYS_sysarch, number, args);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
