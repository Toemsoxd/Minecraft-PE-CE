#ifndef MCPECE_RENDERER_MEMORY_H
#define MCPECE_RENDERER_MEMORY_H

#include <stdlib.h>

static inline void rendererDeferFree(void* p)
{
    if (p)
        free(p);
}

#endif
