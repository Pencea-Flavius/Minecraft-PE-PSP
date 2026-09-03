
#ifndef MCPSP_UTIL_FAST_MEMCPY_H
#define MCPSP_UTIL_FAST_MEMCPY_H

#include <stddef.h>

static inline int vfpuCopyWorthIt(unsigned int src, unsigned int dst, size_t size) {
    return !((src & 3u) != (dst & 3u) || size < 16);
}

void memcpy_vfpu(void* dst, const void* src, size_t size);

#endif
