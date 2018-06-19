// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_CACHE_HANDLER_HPP_
#define USERPLANE_CACHE_HANDLER_HPP_

/**
 * This is a Cache Utility Macros
 *
 */

namespace cache {
const uint32_t kCacheLineBytes = 6;
const uint32_t kCacheLineSize =  (1 << kCacheLineBytes);
const uint32_t kCacheLineMask =  (kCacheLineSize-1);
/**
 * Prefetch a cache line into all cache levels.
 * @param p
 *   Address to prefetch
 */
static inline void Prefetch0(volatile void *p) {
    __asm__ __volatile__ ("prefetcht0 %[p]" : [p] "+m" (*(volatile char *)p));
}

/**
 * Prefetch a cache line into all cache levels except the 0th cache level.
 * @param p
 *   Address to prefetch
 */
static inline void Prefetch1(volatile void *p) {
    __asm__ __volatile__ ("prefetcht1 %[p]" : [p] "+m" (*(volatile char *)p));
}

/**
 * Prefetch a cache line into all cache levels except the 0th and 1th cache
 * levels.
 * @param p
 *   Address to prefetch
 */
static inline void Prefetch2(volatile void *p) {
    __asm__ __volatile__ ("prefetcht2 %[p]" : [p] "+m" (*(volatile char *)p));
}

}  //  namespace cache

/*********** Macro for cache allignment type checks ********/
#define CACHE_LINE_ALIGN_MARK(mark) uint8 mark[0] __attribute__((aligned(cache::kCacheLineSize)))
#define __cache_aligned __attribute__((__aligned__(cache::kCacheLineSize)))



/* Read/write arguments to __builtin_prefetch. */
#define _PREFETCH_READ 0
#define _PREFETCH_LOAD 0    /* alias for read */
#define _PREFETCH_WRITE 1
#define _PREFETCH_STORE 1   /* alias for write */

#define _PREFETCH(n, size, type) \
if ((size) > (n) * cache::kCacheLineSize) \
    __builtin_prefetch(_addr + (n) * cache::kCacheLineSize, \
    _PREFETCH_##type, \
/* locality */ 3);

#define _PREFETCH_DATA(addr, size, type) \
do { \
    void * _addr = (addr); \
    ASSERT((size) <= 4 * cache::kCacheLineSize); \
    _PREFETCH(0, size, type); \
    _PREFETCH(1, size, type); \
    _PREFETCH(2, size, type); \
    _PREFETCH(3, size, type); \
} while (0)


/* Full memory barrier (read and write). */
#define _MEMORY_BARRIER() __sync_synchronize()
#if __x86_64__
#define _MEMORY_STORE_BARRIER() __builtin_ia32_sfence()
#else
#define _MEMORY_STORE_BARRIER() __sync_synchronize()
#endif


#endif  // USERPLANE_CACHE_HANDLER_HPP_
