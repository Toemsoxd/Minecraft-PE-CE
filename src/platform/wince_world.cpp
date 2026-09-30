#include "platform/wince_world.h"

#include "platform/path.h"
#include "world/level/storage/worldlist.h"
#include "world/level/storage/level_storage.h"
#include "world/level/tile/tile.h"
#include "world/item/item.h"

#include <windows.h>
#include <cstring>

World g_world;
Level g_level(&g_world);
bool g_worldBuilt = false;

static bool g_runtimeReady = false;

bool winceWorldInit()
{
    if (g_runtimeReady)
        return true;

    ZeroMemory(&g_world, sizeof(g_world));
    g_level.w = &g_world;
    g_level.player = 0;

    Tile::initTiles();
    Item::initItems();

    g_runtimeReady = true;
    g_worldBuilt = false;
    return true;
}

bool winceWorldLoadFirst()
{
    WorldList list;
    int i;
    char absDir[320];
    long seed = 0;
    int gameType = 1;

    if (!winceWorldInit())
        return false;

    worldListScan(&list);
    if (list.count <= 0)
        return false;

    if (!worldListCreate)
        return false;

    snprintf(absDir, sizeof(absDir), "%s", savePath("saves"));
    {
        size_t n = strlen(absDir);
        if (n > 0 && absDir[n - 1] != '\\')
        {
            absDir[n] = '\\';
            absDir[n + 1] = '\0';
        }
    }
    strncat(absDir, list.names[0], sizeof(absDir) - strlen(absDir) - 1);

    if (!LevelStorage::readInfo(absDir, 0, 0, &gameType, &seed))
        return false;

    LevelStorage::setActiveWorld(absDir, seed, gameType, list.displayNames[0]);

    if (!LevelStorage::load(&g_world, absDir, &seed, &gameType))
    {
        g_worldBuilt = false;
        return false;
    }

    g_worldBuilt = true;
    return true;
}

void winceWorldShutdown()
{
    if (!g_runtimeReady)
        return;

    if (g_worldBuilt)
        worldFree(&g_world);

    g_level.removeAllEntities();
    g_level.removeAllTileEntities();

    g_worldBuilt = false;
    g_runtimeReady = false;
}

int winceWorldProgress()
{
    return (int)g_terrainProgress;
}
