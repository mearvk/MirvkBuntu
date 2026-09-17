/*
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 *
 * Secure JVM: Memory Proxy — Native Shim Library
 * Preloaded into child processes via LD_PRELOAD. Intercepts libc memory
 * and I/O functions, reports counters to the JVM via shared memory.
 *
 * This is a C file (not C++) for maximum compatibility with LD_PRELOAD.
 *
 * License: GPL-2.0
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

/* ========================================================================== */
/*  Shared Memory Layout (must match jvmMemoryProxy_linux.cpp)                */
/* ========================================================================== */

#define PROXY_SHM_MAGIC   0x4A564D50
#define PROXY_SHM_VERSION 1

struct ProxySharedMemory {
    uint32_t magic;
    uint32_t version;
    pid_t    child_pid;

    /* Allocation counters */
    volatile uint64_t total_alloc_bytes;
    volatile uint64_t total_free_bytes;
    volatile uint64_t alloc_count;
    volatile uint64_t free_count;
    volatile uint64_t current_allocated;
    volatile uint64_t peak_allocated;

    /* Per-size-class churn tracking */
    volatile uint32_t churn_8;
    volatile uint32_t churn_16;
    volatile uint32_t churn_32;
    volatile uint32_t churn_64;
    volatile uint32_t churn_128;
    volatile uint32_t churn_256;
    volatile uint32_t churn_512;
    volatile uint32_t churn_1024;
    volatile uint32_t churn_4096;
    volatile uint32_t churn_large;

    /* I/O counters */
    volatile uint64_t total_read_bytes;
    volatile uint64_t total_write_bytes;
    volatile uint64_t read_syscall_count;
    volatile uint64_t write_syscall_count;

    /* File descriptor tracking */
    volatile uint32_t fds_opened;
    volatile uint32_t fds_closed;
    volatile uint32_t fds_current;

    /* Process/thread tracking */
    volatile uint32_t fork_count;
    volatile uint32_t thread_count;

    /* Control flags (written by JVM, read by shim) */
    volatile uint32_t deny_alloc;
    volatile uint32_t deny_fork;
    volatile uint32_t deny_open;
    volatile uint32_t throttle_write;
    volatile int32_t  nice_adjustment;
};

/* ========================================================================== */
/*  Global State                                                               */
/* ========================================================================== */

static struct ProxySharedMemory* g_shm = NULL;
static int g_initialized = 0;

/* Original libc function pointers */
static void* (*real_malloc)(size_t) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;
static void  (*real_free)(void*) = NULL;
static ssize_t (*real_read)(int, void*, size_t) = NULL;
static ssize_t (*real_write)(int, const void*, size_t) = NULL;
static int   (*real_open)(const char*, int, ...) = NULL;
static int   (*real_close)(int) = NULL;
static pid_t (*real_fork)(void) = NULL;

/* Allocation size tracking (simple inline header) */
#define ALLOC_HEADER_MAGIC 0xA110CA7E
struct alloc_header {
    uint32_t magic;
    uint32_t size;
};
#define HEADER_SIZE (sizeof(struct alloc_header))

/* ========================================================================== */
/*  Initialization                                                             */
/* ========================================================================== */

static void shim_init(void) __attribute__((constructor));
static void shim_fini(void) __attribute__((destructor));

/* Size class for churn tracking */
static volatile uint32_t* churn_counter_for_size(size_t size) {
    if (!g_shm) return NULL;
    if (size <= 8)    return &g_shm->churn_8;
    if (size <= 16)   return &g_shm->churn_16;
    if (size <= 32)   return &g_shm->churn_32;
    if (size <= 64)   return &g_shm->churn_64;
    if (size <= 128)  return &g_shm->churn_128;
    if (size <= 256)  return &g_shm->churn_256;
    if (size <= 512)  return &g_shm->churn_512;
    if (size <= 1024) return &g_shm->churn_1024;
    if (size <= 4096) return &g_shm->churn_4096;
    return &g_shm->churn_large;
}

static void shim_init(void) {
    if (g_initialized) return;

    /* Resolve real libc functions */
    real_malloc  = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    real_calloc  = (void* (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    real_realloc = (void* (*)(void*, size_t))dlsym(RTLD_NEXT, "realloc");
    real_free    = (void (*)(void*))dlsym(RTLD_NEXT, "free");
    real_read    = (ssize_t (*)(int, void*, size_t))dlsym(RTLD_NEXT, "read");
    real_write   = (ssize_t (*)(int, const void*, size_t))dlsym(RTLD_NEXT, "write");
    real_open    = (int (*)(const char*, int, ...))dlsym(RTLD_NEXT, "open");
    real_close   = (int (*)(int))dlsym(RTLD_NEXT, "close");
    real_fork    = (pid_t (*)(void))dlsym(RTLD_NEXT, "fork");

    if (!real_malloc || !real_free) {
        /* Cannot proceed without basic allocation */
        return;
    }

    /* Open shared memory segment */
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/jvm-proxy-%d", (int)getpid());

    int fd = shm_open(shm_name, O_RDWR, 0600);
    if (fd < 0) {
        /* Shared memory not created by JVM — we're not being proxied */
        g_initialized = 1;
        return;
    }

    g_shm = (struct ProxySharedMemory*)mmap(
        NULL, sizeof(struct ProxySharedMemory),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (g_shm == MAP_FAILED) {
        g_shm = NULL;
        g_initialized = 1;
        return;
    }

    /* Verify magic */
    if (g_shm->magic != PROXY_SHM_MAGIC || g_shm->version != PROXY_SHM_VERSION) {
        munmap(g_shm, sizeof(struct ProxySharedMemory));
        g_shm = NULL;
        g_initialized = 1;
        return;
    }

    g_initialized = 1;
}

static void shim_fini(void) {
    if (g_shm) {
        munmap(g_shm, sizeof(struct ProxySharedMemory));
        g_shm = NULL;
    }
}

/* ========================================================================== */
/*  Intercepted Functions: Memory Allocation                                   */
/* ========================================================================== */

void* malloc(size_t size) {
    if (!g_initialized) shim_init();
    if (!real_malloc) {
        /* Fallback: cannot intercept, return NULL */
        errno = ENOMEM;
        return NULL;
    }

    /* Check if JVM has denied allocations */
    if (g_shm && g_shm->deny_alloc) {
        errno = ENOMEM;
        return NULL;
    }

    /* Allocate with header for size tracking */
    void* ptr = real_malloc(size + HEADER_SIZE);
    if (!ptr) return NULL;

    struct alloc_header* hdr = (struct alloc_header*)ptr;
    hdr->magic = ALLOC_HEADER_MAGIC;
    hdr->size = (uint32_t)size;

    /* Update shared memory counters */
    if (g_shm) {
        __sync_fetch_and_add(&g_shm->total_alloc_bytes, size);
        __sync_fetch_and_add(&g_shm->alloc_count, 1);
        __sync_fetch_and_add(&g_shm->current_allocated, size);

        /* Update peak */
        uint64_t current = g_shm->current_allocated;
        while (current > g_shm->peak_allocated) {
            __sync_val_compare_and_swap(&g_shm->peak_allocated,
                                        g_shm->peak_allocated, current);
            current = g_shm->current_allocated;
        }
    }

    return (char*)ptr + HEADER_SIZE;
}

void* calloc(size_t nmemb, size_t size) {
    if (!g_initialized) shim_init();

    /* calloc is tricky with LD_PRELOAD because dlsym itself may call calloc.
       Use a static buffer for early allocations. */
    static char early_buf[4096];
    static size_t early_used = 0;

    if (!real_calloc) {
        /* During early init, return from static buffer */
        size_t total = nmemb * size;
        if (early_used + total <= sizeof(early_buf)) {
            void* p = &early_buf[early_used];
            early_used += total;
            memset(p, 0, total);
            return p;
        }
        errno = ENOMEM;
        return NULL;
    }

    if (g_shm && g_shm->deny_alloc) {
        errno = ENOMEM;
        return NULL;
    }

    size_t total = nmemb * size;
    void* ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!g_initialized) shim_init();
    if (!real_realloc) {
        errno = ENOMEM;
        return NULL;
    }

    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (g_shm && g_shm->deny_alloc) {
        errno = ENOMEM;
        return NULL;
    }

    /* Get old size from header */
    struct alloc_header* old_hdr = (struct alloc_header*)((char*)ptr - HEADER_SIZE);
    uint32_t old_size = 0;
    if (old_hdr->magic == ALLOC_HEADER_MAGIC) {
        old_size = old_hdr->size;
    }

    void* new_raw = real_realloc((char*)ptr - HEADER_SIZE, size + HEADER_SIZE);
    if (!new_raw) return NULL;

    struct alloc_header* new_hdr = (struct alloc_header*)new_raw;
    new_hdr->magic = ALLOC_HEADER_MAGIC;
    new_hdr->size = (uint32_t)size;

    if (g_shm) {
        if (size > old_size) {
            __sync_fetch_and_add(&g_shm->total_alloc_bytes, size - old_size);
            __sync_fetch_and_add(&g_shm->current_allocated, size - old_size);
        } else {
            __sync_fetch_and_sub(&g_shm->current_allocated, old_size - size);
            __sync_fetch_and_add(&g_shm->total_free_bytes, old_size - size);
        }
    }

    return (char*)new_raw + HEADER_SIZE;
}

void free(void* ptr) {
    if (!ptr) return;
    if (!g_initialized) shim_init();
    if (!real_free) return;

    /* Check for our header */
    struct alloc_header* hdr = (struct alloc_header*)((char*)ptr - HEADER_SIZE);
    if (hdr->magic == ALLOC_HEADER_MAGIC) {
        uint32_t size = hdr->size;

        if (g_shm) {
            __sync_fetch_and_add(&g_shm->total_free_bytes, size);
            __sync_fetch_and_add(&g_shm->free_count, 1);
            __sync_fetch_and_sub(&g_shm->current_allocated, size);

            /* Increment churn counter for this size class */
            volatile uint32_t* counter = churn_counter_for_size(size);
            if (counter) __sync_fetch_and_add(counter, 1);
        }

        hdr->magic = 0; /* Invalidate */
        real_free(hdr);
    } else {
        /* Not our allocation — pass through */
        real_free(ptr);
    }
}

/* ========================================================================== */
/*  Intercepted Functions: I/O                                                 */
/* ========================================================================== */

ssize_t read(int fd, void* buf, size_t count) {
    if (!g_initialized) shim_init();
    if (!real_read) {
        errno = EIO;
        return -1;
    }

    ssize_t result = real_read(fd, buf, count);

    if (result > 0 && g_shm) {
        __sync_fetch_and_add(&g_shm->total_read_bytes, (uint64_t)result);
        __sync_fetch_and_add(&g_shm->read_syscall_count, 1);
    }

    return result;
}

ssize_t write(int fd, const void* buf, size_t count) {
    if (!g_initialized) shim_init();
    if (!real_write) {
        errno = EIO;
        return -1;
    }

    /* Check if JVM is throttling writes */
    if (g_shm && g_shm->throttle_write) {
        /* Add 1ms delay per write call (backpressure) */
        usleep(1000);
    }

    ssize_t result = real_write(fd, buf, count);

    if (result > 0 && g_shm) {
        __sync_fetch_and_add(&g_shm->total_write_bytes, (uint64_t)result);
        __sync_fetch_and_add(&g_shm->write_syscall_count, 1);
    }

    return result;
}

int open(const char* pathname, int flags, ...) {
    if (!g_initialized) shim_init();
    if (!real_open) {
        errno = ENOSYS;
        return -1;
    }

    /* Check if JVM has denied opens */
    if (g_shm && g_shm->deny_open) {
        errno = EMFILE;
        return -1;
    }

    /* Handle variadic mode argument */
    int result;
    if (flags & O_CREAT) {
        va_list ap;
        __builtin_va_start(ap, flags);
        mode_t mode = __builtin_va_arg(ap, mode_t);
        __builtin_va_end(ap);
        result = real_open(pathname, flags, mode);
    } else {
        result = real_open(pathname, flags);
    }

    if (result >= 0 && g_shm) {
        __sync_fetch_and_add(&g_shm->fds_opened, 1);
        __sync_fetch_and_add(&g_shm->fds_current, 1);
    }

    return result;
}

int close(int fd) {
    if (!g_initialized) shim_init();
    if (!real_close) {
        errno = ENOSYS;
        return -1;
    }

    int result = real_close(fd);

    if (result == 0 && g_shm) {
        __sync_fetch_and_add(&g_shm->fds_closed, 1);
        if (g_shm->fds_current > 0) {
            __sync_fetch_and_sub(&g_shm->fds_current, 1);
        }
    }

    return result;
}

/* ========================================================================== */
/*  Intercepted Functions: Process Creation                                    */
/* ========================================================================== */

pid_t fork(void) {
    if (!g_initialized) shim_init();
    if (!real_fork) {
        errno = ENOSYS;
        return -1;
    }

    /* Check if JVM has denied fork */
    if (g_shm && g_shm->deny_fork) {
        errno = EAGAIN;
        return -1;
    }

    pid_t result = real_fork();

    if (result > 0 && g_shm) {
        /* Parent: record the fork */
        __sync_fetch_and_add(&g_shm->fork_count, 1);
    }

    return result;
}
