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

#ifndef _KERNEL_H
#define _KERNEL_H

#include <aos/const.h>
#include <aos/types.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>

/* Architecture-specific configuration */
#if defined(ARCH_X86_64) && ARCH_X86_64
/* The virtual address and (reserved) size of kernel variable */
#define KVAR_ADDR       0xc0084000ULL
#define KVAR_SIZE       0x00004000ULL
/* Boot information from the boot loader */
#define BOOTINFO_BASE   0xc0008000ULL
#else
#error "This architecture is not supported."
/* Define the kernel variables to mitigate IDE's errors in coding */
#define KVAR_ADDR       0
#define KVAR_SIZE       0
#define BOOTINFO_BASE   0
#endif
#define g_kvar          ((struct kernel_variables *)KVAR_ADDR)
#define g_kmem          g_kvar->kmem
#define g_proc_table    g_kvar->proc_table
#define g_ktask_root    g_kvar->ktask_root
#define g_syscall_table g_kvar->syscall_table
#define g_intr_table    g_kvar->intr_table
#define g_timer         g_kvar->timer
#define g_jiffies       g_kvar->jiffies
#define g_devfs         g_kvar->devfs

#define FLOOR(val, base)        (((val) / (base)) * (base))
#define CEIL(val, base)         ((((val) - 1) / (base) + 1) * (base))
#define DIV_FLOOR(val, base)    ((val) / (base))
#define DIV_CEIL(val, base)     (((val) - 1) / (base) + 1)

/*
 * NOTE FOR PAGE SIZES:
 * PAGESIZE and SUPERPAGESIZE must be consistent with the paging unit of the
 * processor's architecture.  In x86/x86-64, PAGESIZE and SUPERPAGESIZE must be
 * 4 KiB and 2 MiB, respectively.  Physical and kernel memory are managed in the
 * size of superpages.
 */
#define PAGESIZE                4096ULL         /* 4 KiB */
#define SUPERPAGESIZE           (1ULL << 21)    /* 2 MiB */
#define SP_SHIFT                9               /* log2(2M/4K)*/
/* Validation macro */
#if (PAGESIZE) << (SP_SHIFT) != (SUPERPAGESIZE)
#error "Invalid SP_SHIFT value."
#endif

#define CACHELINESIZE           64
#define CACHE_ALIGN(a)          CEIL(a, CACHELINESIZE)
#define PAGE_ALIGN(a)           CEIL(a, PAGESIZE)
#define SUPERPAGE_ALIGN(a)      CEIL(a, SUPERPAGESIZE)

#define PAGE_ADDR(i)            (PAGESIZE * (u64)(i))
#define SUPERPAGE_ADDR(i)       (SUPERPAGESIZE * (u64)(i))
#define PAGE_INDEX(a)           ((u64)(a) / PAGESIZE)
#define SUPERPAGE_INDEX(a)      ((u64)(a) / SUPERPAGESIZE)

/* Flags for struct pmem */
#define PMEM_USABLE             (1)             /* Usable */
#define PMEM_USED               (1 << 1)        /* Used */
#define PMEM_IS_FREE(x)         (PMEM_USABLE == (x)->flags ? 1 : 0)

#define PMEM_MAX_BUDDY_ORDER    18
#define PMEM_INVAL_BUDDY_ORDER  0x3f
#define PMEM_INVAL_INDEX        0xffffffffUL

/* Memory zones and NUMA domains */
#define PMEM_NUMA_MAX_DOMAINS   16
#define PMEM_ZONE_DMA           0
#define PMEM_ZONE_LOWMEM        1
#define PMEM_ZONE_UMA           2
#define PMEM_ZONE_NUMA(d)       (3 + (d))
#define PMEM_NUM_ZONES          (3 + PMEM_NUMA_MAX_DOMAINS)


/*
 * Kernel memory
 */
/* 32 (2^5) -byte is the minimum object size of a slab object */
#define KMEM_SLAB_BASE_ORDER    5
/* 8192 (2^(5 + 9 - 1)) byte is the maximum object size of a slab object */
#define KMEM_SLAB_ORDER         9
/* 2^16 objects in a cache */
#define KMEM_SLAB_NR_OBJ_ORDER  4

#define KMEM_MAX_BUDDY_ORDER    10
#define KMEM_INVAL_BUDDY_ORDER  0x3f

#define KMEM_USABLE             (1)
#define KMEM_USED               (1<<1)
#define KMEM_SLAB               (1<<2)
#define KMEM_IS_FREE(x)         (KMEM_USABLE == ((x)->flags & 0x3))

/*
 * Virtual memory
 */
#define VMEM_MAX_BUDDY_ORDER    18
#define VMEM_INVAL_BUDDY_ORDER  0x3f

#define VMEM_USABLE             (1)
#define VMEM_USED               (1<<1)
#define VMEM_GLOBAL             (1<<2)
#define VMEM_SUPERPAGE          (1<<3)
#define VMEM_IS_FREE(x)         (VMEM_USABLE == ((x)->flags & 0x3))
#define VMEM_IS_SUPERPAGE(x)    (VMEM_SUPERPAGE & (x)->flags)

#define INITRAMFS_BASE          0x30000ULL
#define USTACK_INIT             0xbfe00000ULL
#define CODE_INIT               0x40000000ULL
#define KSTACK_SIZE             (4096 * 512)
#define USTACK_SIZE             (4096 * 512)

/* Process table size */
#define PROC_NR                 65536

/* Maximum number of file descriptors */
#define FD_MAX                  1024

/* Maximum bytes in the path name */
#define PATH_MAX                1024

/* Task policy */
#define KTASK_POLICY_KERNEL     0
#define KTASK_POLICY_DRIVER     1
#define KTASK_POLICY_SERVER     2
#define KTASK_POLICY_USER       3

/* Tick */
#define HZ                      100
#define IV_LOC_TMR              0x40
#define IV_LOC_TMR_XP           0x41 /* Exclusive processor */
#define IV_CRASH                0xfe
#define NR_IV                   0x100
#define IV_IRQ(n)               (0x20 + (n))


#define ENOENT                  2
#define EINTR                   4
#define EIO                     5
#define ENOEXEC                 8
#define EBADF                   9
#define ENOMEM                  12
#define EACCES                  13
#define EFAULT                  14
#define EINVAL                  22

/*
 * String
 */
struct kstring {
    void *base;
    size_t sz;
};

/*
 * File descriptor
 */
struct fildes {
    void *data;
    off_t pos;
    ssize_t (*read)(struct fildes *, void *, size_t);
    ssize_t (*write)(struct fildes *, const void *, size_t);
    off_t (*lseek)(struct fildes *, off_t, int);
};

/*
 * Virtual page
 */
struct vmem_page {
    /* Physical address: least significant bits are plan to be used for flags */
    reg_t addr;
    /* Order */
    int order;
    /* Flags */
    int flags;
    /* Back-link to the corresponding superpage */
    struct vmem_superpage *superpage;
    /* Buddy system */
    struct vmem_page *next;
    struct vmem_page *prev;
};

/*
 * Virtual superpage
 */
struct vmem_superpage {
    union {
        /* Superpage */
        struct {
            /* Physical address */
            reg_t addr;
        } superpage;
        /* Page */
        struct {
            /* Pages */
            struct vmem_page *pages;
        } page;
    } u;
    /* Order */
    int order;
    /* Flags */
    int flags;
    /* Back-link to the corresponding region */
    struct vmem_region *region;
    /* Buddy system */
    struct vmem_superpage *next;
    struct vmem_superpage *prev;
};

/*
 * Virtual memory region
 */
struct vmem_region {
    /* Region information */
    ptr_t start;
    size_t len;                 /* Constant multiplication of SUPERPAGESIZE */

    /* Capacity and the number of used pages */
    //size_t total_pgs;
    //size_t used_pgs;

    /* Superpages belonging to this region */
    struct vmem_superpage *superpages;

    /* Buddy system for superpages and pages */
    struct vmem_superpage *spgheads[VMEM_MAX_BUDDY_ORDER + 1];
    struct vmem_page *pgheads[SP_SHIFT + 1];

    /* Pointer to the next region */
    struct vmem_region *next;
};

/*
 * Virtual memory space
 */
struct vmem_space {
    /* Virtual memory region */
    struct vmem_region *first_region;

    /* Virtual page table */
    void *vmap;

    /* Architecture specific data structure (e.g., page table)  */
    void *arch;
};

/*
 * Kernel page
 */
struct kmem_page {
    /* Physical address */
    reg_t addr;

    /* Flags */
    int flags;

    /* Order of the buddy system */
    u8 order;

    /* Zone */
    u16 zone;

    /* Slab */
    struct kmem_slab *slab;

    /* Buddy system */
    struct kmem_page *next;
    struct kmem_page *prev;
};

/*
 * Kernel memory
 */
struct kmem_space {
    ptr_t start;
    size_t len;                 /* Constant multiplication of SUPERPAGESIZE */

    /* Superpages belonging to kernel */
    struct kmem_page *pages;

    /* Buddy system for superpages and pages */
    struct kmem_page *heads[KMEM_MAX_BUDDY_ORDER + 1];

    /* Architecture-specific data structure (struct arch_kmem_space) */
    void *arch;
};


/*
 * The management data structure of physical pages (2-MiB pages) are ensured to
 * be mapped in the kernel memory region so that the kernel memory allocator
 * does not recursively depend on any memory allocator.  4-KiB sub-pages can be
 * allocated another interface that uses the kernel memory allocator.
 */

/*
 * Physical subpage
 */
struct pmem_subpage {
    /* Parent page */
    struct pmem_page *page;
    /* Order */
    int order;
    /* Flags */
    int flags;
    /* Buddy system */
    struct pmem_subpage *prev;
    struct pmem_subpage *next;
};

/*
 * Physical page
 */
struct pmem_page {
    u16 zone;
    u8 flags;
    /* Buddy system */
    u8 order;
    u32 next;
    /* Sub-pages (segregated list) */
    void *subpages;
} __attribute__((packed));

/*
 * Buddy system
 */
struct pmem_buddy {
    u32 heads[PMEM_MAX_BUDDY_ORDER + 1];
};

/*
 * Memory zone
 */
struct pmem_zone {
    /* Buddy system */
    struct pmem_buddy buddy;
    /* Statistics */
    size_t total;
    size_t used;
};

/*
 * Protocol to operate the physical memory
 */
struct pmem_proto {
    /* Allocate 2^order pages from a particular domain */
    void * (*alloc_pages)(int domain, int order);
    /* Allocate a page from a particular domain */
    void * (*alloc_page)(int domain);
    /* Free pages */
    void (*free_pages)(void *page);
};

/*
 * Physical memory
 */
struct pmem {
    /* Lock variable for physical memory operations */
    spinlock_t lock;

    /* The number of pages */
    size_t nr;

    /* Physical pages */
    struct pmem_page *pages;

    /* Zones (NUMA domains) */
    struct pmem_zone zones[PMEM_NUM_ZONES];
};

/*
 * Slab objects
 *  slab_hdr
 *    object 0
 *    object 1
 *    ...
 */
struct kmem_slab {
    /*
     * slab_hdr
     */
    /* A pointer to the next slab header */
    struct kmem_slab *next;
    /* Parent free list */
    struct kmem_slab_free_list *free_list;
    /* The name of this slab */
    char *name;
    /* The size of each object in the slab */
    size_t size;
    /* The number of objects in this slab */
    int nr;
    /* The number of used (allocated) objects */
    int nused;
    /* The number of unused (free) objects */
    int free;
    /* The pointer to the array of objects */
    void *obj_head;
    /* Free marks follows (nr byte) */
    u8 marks[0];
    /* Objects follows */
} __attribute__ ((packed));

/*
 * Free list of slab objects
 */
struct kmem_slab_free_list {
    struct kmem_slab *partial;
    struct kmem_slab *full;
    struct kmem_slab *free;
} __attribute__ ((packed));

/*
 * Root data structure of slab objects
 */
struct kmem_slab_root {
    /* Generic slabs */
    struct kmem_slab_free_list gslabs[PMEM_NUM_ZONES][KMEM_SLAB_ORDER];
};

/*
 * Free pages in kmem region
 */
struct kmem_mm_page {
    struct kmem_mm_page *next;
};

/*
 * Kernel memory
 */
struct kmem {
    /* Lock */
    spinlock_t lock;
    spinlock_t slab_lock;

    /* Slab allocator */
    struct kmem_slab_root slab;

    /* Kernel memory */
    struct kmem_space *space;

    /* Memory pool; Page data structure pool */
    struct {
        /* Free pages for page table */
        struct kmem_mm_page *mm_pgs;
    } pool;

    /* Physical memory */
    struct pmem *pmem;
};

/*
 * Pager
 */
typedef void * (*page_alloc_f)(void);
typedef void (*page_free_f)(void *);
struct pager {
    page_alloc_f *alloc_page;
    page_free_f *free_page;
};

/*
 * Process
 */
struct proc {
    /* Process ID */
    pid_t id;

    /* Name */
    char name[PATH_MAX];

    /* Parent process */
    struct proc *parent;

    /* Tasks */
    struct ktask *tasks;

    /* User information */
    uid_t uid;
    gid_t gid;

    /* Architecture specific structure; i.e., (struct arch_proc) */
    void *arch;

    /* Memory */
    struct vmem_space *vmem;

    /* Policy */
    int policy;

    /* File descriptors */
    struct fildes *fds[FD_MAX];


    /* Code */
    void *code_paddr;
    size_t code_size;

    /* Exit status */
    int exit_status;
};

/*
 * Process table
 */
struct proc_table {
    /* Process table */
    struct proc *procs[PROC_NR];
    /* pid last assigned (to find the next pid by sequential search) */
    pid_t lastpid;
};

/*
 * Kernel task state
 */
enum ktask_state {
    KTASK_STATE_CREATED,
    KTASK_STATE_READY,
    KTASK_STATE_BLOCKED,
    KTASK_STATE_TERMINATED,
};

/*
 * Kernel task data structure
 */
struct ktask {
    /* Architecture specific structure; i.e., (struct arch_task) */
    void *arch;
    /* State */
    enum ktask_state state;

    /* Process */
    struct proc *proc;

    /* Task type: Tick-full or tickless */
    int type;

    /* Linked list of tasks in the same process */
    struct ktask *proc_task_next;

    /* Pointers for scheduler (runqueue) */
    struct ktask *next;
    int credit;                 /* quantum */
};

/*
 * Kernel task list
 */
struct ktask_list {
    struct ktask *ktask;
    struct ktask_list *next;
};
struct ktask_root {
    /* Running */
    struct {
        struct ktask_list *head;
        struct ktask_list *tail;
    } r;
    /* Blocked (or others) */
    struct {
        struct ktask_list *head;
        struct ktask_list *tail;
    } b;
};

/*
 * Special device
 */
struct devfs_chr {
    /* Also map to driver's virtual memory */
    struct driver_device_chr *dev;
};
struct devfs_blk {
    void *blk;
};
struct devfs_entry {
    char *name;
    int flags;
    struct proc *proc;
    union {
        /* Character device */
        struct devfs_chr chr;
        /* Block device */
        struct devfs_blk blk;
    } spec;
    struct devfs_entry *next;
};
struct devfs {
    struct devfs_entry *head;
};

/*
 * Kernel timer
 */
struct ktimer_event {
    reg_t jiffies;
    struct proc *proc;
    struct ktimer_event *next;
};
struct ktimer {
    /* Head of timer list */
    struct ktimer_event *head;
};

/* Kernel event handler */
typedef void (*kevent_handler_f)(void);
struct kevent_handlers {
    /* Interrupt vector table */
    kevent_handler_f ivt[NR_IV];
};

/* Interrupt handler */
typedef void (*interrupt_handler_f)(void);
struct interrupt_handler {
    /* Interrupt handler */
    interrupt_handler_f f;
    /* Process to set an appropriate page table before call it */
    struct proc *proc;
};
struct interrupt_handler_table {
    struct interrupt_handler ivt[NR_IV];
};

/* Global variables */
struct kernel_variables {
    struct kmem *kmem;
    struct proc_table *proc_table;
    struct ktask_root *ktask_root;
    void *syscall_table[SYS_MAXSYSCALL];
    struct interrupt_handler_table *intr_table;
    /* Timer */
    struct ktimer timer;
    reg_t jiffies;
    /* devfs */
    struct devfs devfs;
};

/* for variable-length arguments */
typedef __builtin_va_list va_list;
#define va_start(ap, last)      __builtin_va_start((ap), (last))
#define va_arg                  __builtin_va_arg
#define va_end(ap)              __builtin_va_end(ap)
#define va_copy(dest, src)      __builtin_va_copy((dest), (src))
#define alloca(size)            __builtin_alloca((size))


/* in kernel.c */
void kinit(void);
void kernel(void);
int kstrcmp(const char *, const char *);
size_t kstrlen(const char *);
char * kstrcpy(char *, const char *);
char * kstrncpy(char *, const char *, size_t);
size_t kstrlcpy(char *, const char *, size_t);
char * kstrdup(const char *);

/* in strfmt.c */
int kvsnprintf(char *, size_t, const char *, va_list);
int ksnprintf(char *, size_t, const char *, ...);

/* in asm.s */
#define HAS_KMEMSET     1       /* kmemset is implemented in asm.s. */
#define HAS_KMEMCMP     1       /* kmemcmp is implemented in asm.s. */
#define HAS_KMEMCPY     1       /* kmemcpy is implemented in asm.s. */
void * kmemset(void *, int, size_t);
int kmemcmp(const void *, const void *, size_t);
void * kmemcpy(void *__restrict, const void *__restrict, size_t);

/* in sched.c */
void sched_high(void);

/* in memory.c */
int pmem_init(struct pmem *);
int kmem_init(void);
void * kmalloc(size_t);
void kfree(void *);
void * vmalloc(size_t);
void vfree(void *);
struct vmem_region * vmem_region_create(void);
struct vmem_space * vmem_space_create(void);
void vmem_space_delete(struct vmem_space *);

int vmem_buddy_init(struct vmem_region *);
void * vmem_alloc_pages(struct vmem_space *, int);
void vmem_free_pages(struct vmem_space *, void *);
void * vmem_buddy_alloc_superpages(struct vmem_space *, int);
void * vmem_buddy_alloc_pages(struct vmem_space *, int);
void vmem_buddy_free_superpages(struct vmem_space *, void *);
void vmem_buddy_free_pages(struct vmem_space *, void *);
void * vmem_search_available_region(struct vmem_space *, size_t);

struct vmem_superpage * vmem_grab_superpages(struct vmem_space *, int);
void vmem_return_superpages(struct vmem_superpage *);
struct vmem_page * vmem_grab_pages(struct vmem_space *, int);
void vmem_return_pages(struct vmem_page *);

/* in kmem.c */
int kmem_buddy_init(struct kmem *);
void * kmem_alloc_pages(struct kmem *, size_t, int);
void kmem_free_pages(struct kmem *, void *);

/* in pmem.c */
void * pmem_prim_alloc_pages(int, int);
void * pmem_prim_alloc_page(int);
void pmem_prim_free_pages(void *);

/* in ramfs.c */
int ramfs_init(u64 *);

/* in syscall.c */
void sys_exit(int);
ssize_t sys_read(int, void *, size_t);
ssize_t sys_write(int, const void *, size_t);
int sys_open(const char *, int, ...);
int sys_close(int);
pid_t sys_wait4(pid_t, int *, int, struct rusage *);
pid_t sys_getpid(void);
uid_t sys_getuid(void);
int sys_kill(pid_t, int);
pid_t sys_getppid(void);
gid_t sys_getgid(void);
int sys_execve(const char *, char *const [], char *const []);
void * sys_mmap(void *, size_t, int, int, int, off_t);
int sys_munmap(void *, size_t);
off_t sys_lseek(int, off_t, int);
int sys_nanosleep(const struct timespec *, struct timespec *);
void sys_xpsleep(void);
int sys_driver(int, void *);
int sys_sysarch(int, void *);

/* The followings are mandatory functions for the kernel and should be
   implemented somewhere in arch/<arch_name>/ */
reg_t bitwidth(reg_t);
struct ktask * this_ktask(void);
void set_next_ktask(struct ktask *);
void set_next_idle(void);
void panic(const char *);
void halt(void);
struct proc * proc_fork(struct proc *, struct ktask *, struct ktask **);
void task_set_return(struct ktask *, unsigned long long);
pid_t sys_fork(void);
void spin_lock(u32 *);
void spin_unlock(u32 *);
int arch_vmem_map(struct vmem_space *, void *, void *, int);
int arch_kmem_map(struct kmem *, void *, void *, int);
int arch_kmem_unmap(struct kmem *, void *);
int arch_address_width(void);
void * arch_kmem_addr_v2p(struct kmem *, void *);
void * arch_vmem_addr_v2p(struct vmem_space *, void *);
int arch_vmem_init(struct vmem_space *);
void syscall_setup(void *, size_t);
void arch_switch_page_table(struct vmem_space *);

#endif /* _KERNEL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
