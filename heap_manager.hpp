// Copyright 2018 Polaris Networks (www.polarisnetworks.net).
#ifndef USERPLANE_HEAP_MANAGER_HPP_
#define USERPLANE_HEAP_MANAGER_HPP_

/**
 * Memory Manager
 *
 */
#include "common.hpp"

namespace heap {
always_inline
void* MiniMalloc(uint8_t core, size_t size) {
    if (core != 0)
      return malloc(size);
    else
      return ::operator new (size);
}
always_inline
void MiniFree(uint8_t core, void** mem) {
      if (likely(NULL != *mem)) {
          free(*mem);
          *mem = NULL;
      }
}
}  //  namespace heap
#endif  // USERPLANE_HEAP_MANAGER_HPP_
