#ifndef MCPSP_PLATFORM_DCACHE_H
#define MCPSP_PLATFORM_DCACHE_H

#include <stddef.h>

/*
 * Windows CE software-rendering backend.
 *
 * The PSP backend explicitly writes back CPU cache ranges before handing
 * buffers to the PSP GE. The first CE renderer keeps its framebuffer and
 * meshes in CPU-visible memory, so there is no separate GPU consumer yet.
 * Keep the call as a platform hook so the portable renderer does not need
 * PSP cache APIs.
 */
static inline void dcacheFlush(const void* p, size_t bytes)
{
    (void)p;
    (void)bytes;
}

#endif
