#ifndef MCPECE_PLATFORM_TIME_WINCE_H
#define MCPECE_PLATFORM_TIME_WINCE_H
#include <windows.h>
static inline unsigned long timeMillis(){return (unsigned long)GetTickCount();}
static inline float nowSecondsWince(){return (float)timeMillis()/1000.0f;}
static inline bool timeReachedWince(unsigned long now,unsigned long deadline){return (long)(now-deadline)>=0;}
#endif
