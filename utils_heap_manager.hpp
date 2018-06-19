// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_UTILS_HEAP_MANAGER_HPP_
#define USERPLANE_UTILS_HEAP_MANAGER_HPP_

/**
 * Memory Manager
 *
 */
#include "utils_common.hpp"

namespace heap {
void* MiniMalloc(int core, size_t size) {
    return malloc(size);
}
void MiniFree(void** mem) {
    if (likely(NULL != *mem)) {
        free(*mem);
        *mem = NULL;
    }
}
}  //  namespace heap
#endif  // USERPLANE_UTILS_HEAP_MANAGER_HPP_
