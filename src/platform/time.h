#ifndef MCPSP_PLATFORM_TIME_H
#define MCPSP_PLATFORM_TIME_H

#include <windows.h>

static inline bool timeReached(unsigned long nowMs, unsigned long deadlineMs)
{
    return (long)(nowMs - deadlineMs) >= 0;
}

extern unsigned long g_timeBootMs;

static inline float nowSeconds()
{
    unsigned long now = GetTickCount();

    if (g_timeBootMs == 0)
        g_timeBootMs = now;

    return (float)(now - g_timeBootMs) / 1000.0f;
}

extern float g_gameSeconds;
extern bool  g_gameFrozen;

static inline float gameSeconds() { return g_gameSeconds; }
static inline bool  gameFrozen()  { return g_gameFrozen; }

#endif
