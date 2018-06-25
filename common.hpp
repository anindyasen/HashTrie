#ifndef USERPLANE_COMMON_HPP_
#define USERPLANE_COMMON_HPP_

#include <cstdint>
#include <cstring>
#include <type_traits>

#ifdef __SSE2__
#include <emmintrin.h>
#endif
/**
 * @file
 * Common Utility Operations
 *
 * This file defines a generic Macro/API for utility operations.
 * The implementation is architecture-specific.
 *
 */

 /**
 * Macro
 */
//  #define always_inline static inline __attribute__ ((__always_inline__))
/* Used to pack structure elements. */
#define PACKED(x)   x __attribute__ ((packed))
#define UNUSED(x)   x __attribute__ ((unused))

/* Make a string from the macro's argument */
#define _STRING_MACRO(x) #x

#define __unused __attribute__ ((unused))
#define __weak __attribute__ ((weak))
#define __packed __attribute__ ((packed))
#define __constructor_ __attribute__ ((constructor))

#define never_inline __attribute__ ((__noinline__))

#if __DEBUG > 0
#define always_inline inline
#define always_static_inline static inline
#else
#define always_inline inline __attribute__ ((__always_inline__))
#define always_static_inline static inline __attribute__ ((__always_inline__))
#endif
/** Branch prediction ************************/
/**
 * Check if a branch is likely to be taken.
 *
 * This compiler builtin allows the developer to indicate if a branch is
 * likely to be taken. Example:
 *
 *   if (likely(x > 1))
 *      do_stuff();
 *
 */
#ifndef likely
#define likely(x)  __builtin_expect((!!(x)), 1L)
#endif /* likely */

/**
 * Check if a branch is unlikely to be taken.
 *
 * This compiler builtin allows the developer to indicate if a branch is
 * unlikely to be taken. Example:
 *
 *   if (unlikely(x < 1))
 *      do_stuff();
 *
 */
#ifndef unlikely
#define unlikely(x)  __builtin_expect((!!(x)), 0L)
#endif /* unlikely */

/*********** Macro for compile type checks ********/
/**
 * Triggers an error at compilation time if the condition is true.
 */
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))

#ifndef offsetof
/** Return the offset of a field in a structure. */
#define offsetof(TYPE, MEMBER)  __builtin_offsetof (TYPE, MEMBER)
#endif


/**
 * General memory barrier.
 *
 * Guarantees that the LOAD and STORE operations generated before the
 * barrier occur before the LOAD and STORE operations generated after.
 */
#define SetMemoryBarrier() _mm_mfence()

/**
 * Write memory barrier.
 *
 * Guarantees that the STORE operations generated before the barrier
 * occur before the STORE operations generated after.
 */
#define SetWriteMemoryBarrier() _mm_sfence()

/**
 * Read memory barrier.
 *
 * Guarantees that the LOAD operations generated before the barrier
 * occur before the LOAD operations generated after.
 */
#define SetReadMemoryBarrier() _mm_lfence()



/*********** Utility functions for calculating min, max and abs **********/

namespace utils {
template <typename T>
always_static_inline T MIN(T x, T y) {
    static_assert(std::is_scalar<T>::value, "should be scalar type");
    return x < y ? x : y;
}

template <typename T>
always_static_inline T MAX(T x, T y) {
    static_assert(std::is_scalar<T>::value, "should be scalar type");
    return x > y ? x : y;
}

template <typename T>
always_static_inline T ABS(T x) {
    static_assert(std::is_scalar<T>::value, "should be scalar type");
    return x < 0 ? -x : x;
}

template <typename T>
always_static_inline int ARRAY_LEN(const T& x) {
    static_assert(std::is_array<T>::value, "should be array type");
    return (sizeof (x)/sizeof (x[0]));
}

template <typename T>
always_static_inline bool is_power_of_2(T n) {
    static_assert(std::is_integral<T>::value, "should be integral type");
    return ((n-1) & n) == 0;
}

template <typename T>
always_static_inline T align_pow_2(T n) {
    static_assert(std::is_integral<T>::value, "should be integral type");
    if (utils::is_power_of_2(n)) {
        return n;
    }
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

template <typename T>
always_static_inline T FindMaxInArray(const T* in_array, int array_size) {
    static_assert(std::is_arithmetic<T>::value, "should be arithmetic type");
    T max = in_array[0];
    for (int i = 0; i < array_size; i++) {
        if (in_array[i] > max) {
            max = in_array[i];
        }
    }
    return max;
}
/**
 * Searches the input parameter for the least significant set bit
 * (starting from zero).
 * If a least significant 1 bit is found, its bit index is returned.
 * If the content of the input parameter is zero, then the content of the return
 * value is undefined.
 * @param v
 *     input parameter, should not be zero.
 * @return
 *     least significant set bit in the input parameter.
 */
template <typename T>
always_static_inline T bsf32(T v) {
    static_assert(std::is_same<unsigned int, T>::value, "should be unsigned int type");
    return (__builtin_ctz(v));
}
/**
 * Compiler barrier.
 * Guarantees that operation reordering does not occur at compile time
 * for operations directly before and after the barrier.
 */
always_static_inline void compiler_barrier() {
    asm volatile ("" : : : "memory");
}

#ifdef __SSE2__
/**
 * PAUSE instruction for tight loops (avoid busy waiting)
 */
always_static_inline void pause(void) {
    _mm_pause();
}
#else
always_static_inline void pause(void) {}
#endif

enum class RESULT {
    OK = 0,
    ERROR = -1
};
//  #define MAX_CPU_COUNT                128
//  #define MAX_CPU_THREAD_COUNT         32
//  #define MAX_SIBLING_PER_THREAD       4
//  #define MAX_USABLE_IFACE_COUNT   2

const uint8_t kMaxCpuCnt = 128;
const uint8_t kMaxCpuThreadCnt = 32;
const uint8_t kMaxSiblingPerThread = 4;
const uint8_t kMaxUsableInterfaceCnt =  2;
const uint16_t kMaxlen = 1024;

}  // namespace utils


#endif  // USERPLANE_COMMON_HPP_
