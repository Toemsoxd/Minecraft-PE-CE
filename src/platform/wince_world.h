#ifndef MCPECE_WINCE_WORLD_H
#define MCPECE_WINCE_WORLD_H

#include "world/level/world.h"
#include "world/level/level.h"

extern World g_world;
extern Level g_level;
extern bool g_worldBuilt;

bool winceWorldInit();
bool winceWorldLoadFirst();
void winceWorldShutdown();
int winceWorldProgress();

#endif
