#include "platform/wince_world.h"

#include "platform/path.h"
#include "world/level/storage/worldlist.h"
#include "world/level/storage/level_storage.h"
#include "world/level/tile/tile.h"
#include "world/item/item.h"
#include "world/level/levelgen/mcpegen.h"
#include "world/level/chunk/chunk_cache.h"
#include "world/level/levelgen/level_source.h"

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
    char absDir[320];
    long seed = 0;
    int gameType = 1;

    if (!winceWorldInit())
        return false;

    worldListScan(&list);
    if (list.count <= 0)
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

bool winceWorldCreateTest()
{
    const long seed = 0x13579BDFL;
    const int spawnX = WORLD_W / 2;
    const int spawnZ = WORLD_D / 2;
    char absDir[320];

    if (!winceWorldInit())
        return false;

    if (!worldAllocArrays(&g_world))
        return false;

    snprintf(absDir, sizeof(absDir), "%s", savePath("saves\\CE_TEST"));
    CreateDirectory(savePath("saves"), NULL);
    CreateDirectory(absDir, NULL);

    LevelStorage::setActiveWorld(absDir, seed, 1, "CE TEST WORLD",
                                 WORLD_TYPE_OLD, GEN_FEATURES_ALL_ON);

    chunkStorageInit(absDir);
    worldGenInit(seed, GEN_FEATURES_ALL_ON);

    g_terrainProgress = 0;
    worldEnsureArea(&g_world, spawnX >> 4, spawnZ >> 4, 2);
    g_terrainProgress = 70;

    lightCompactAll(&g_world);
    g_world.lightReady = true;
    worldUpdateSkyDarken(&g_world);
    g_terrainProgress = 100;

    g_level.spawnX = spawnX;
    g_level.spawnZ = spawnZ;
    worldFindSpawn(&g_world, &g_level.spawnX, &g_level.spawnZ, &g_level.spawnY);

    g_worldBuilt = true;
    return true;
}

bool winceWorldStart()
{
    if (!winceWorldInit())
        return false;

    WorldList list;
    worldListScan(&list);

    if (list.count > 0)
        return winceWorldLoadFirst();

    return winceWorldCreateTest();
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
