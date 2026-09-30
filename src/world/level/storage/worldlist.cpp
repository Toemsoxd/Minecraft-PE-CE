#include "world/level/storage/worldlist.h"
#include "world/level/levelgen/level_source.h"
#include "world/level/levelgen/gen_features.h"
#include "world/level/storage/level_storage.h"

#include "platform/path.h"

#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

static long dateKey(const SYSTEMTIME& t)
{
    int y = (int)t.wYear;
    int m = (int)t.wMonth;
    int d = (int)t.wDay;
    int days;

    if (m < 3)
    {
        y--;
        m += 12;
    }

    days = 365 * (y - 1970)
         + (y - 1969) / 4
         - (y - 1901) / 100
         + (y - 1900) / 400
         + (153 * (m - 3) + 2) / 5
         + d - 1;

    return (long)days * 86400L
         + (long)t.wHour * 3600L
         + (long)t.wMinute * 60L
         + (long)t.wSecond;
}

static bool getFileDate(const char* path, SYSTEMTIME* out)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    SYSTEMTIME local;

    if (!out)
        return false;

    ZeroMemory(&data, sizeof(data));
    if (!GetFileAttributesEx(path, GetFileExInfoStandard, &data))
        return false;

    if (!FileTimeToLocalFileTime(&data.ftLastWriteTime, &data.ftLastWriteTime))
        return false;

    if (!FileTimeToSystemTime(&data.ftLastWriteTime, &local))
        return false;

    *out = local;
    return true;
}

long worldSeedFromString(const char* str)
{
    if (!str || str[0] == '\0')
        return (long)time(NULL);

    const char* p = str;
    if (*p == '-' || *p == '+') p++;

    bool numeric = (*p != '\0');
    const char* q;
    for (q = p; *q; q++)
        if (*q < '0' || *q > '9') { numeric = false; break; }

    if (numeric)
        return strtol(str, NULL, 10);

    unsigned int h = 0;
    for (q = str; *q; q++)
        h = h * 31u + (unsigned char)*q;

    return (long)(int)h;
}

void worldListScan(WorldList* out)
{
    out->count = 0;

    const char* dir = savePath("saves");
    CreateDirectory(dir, NULL);

    char pattern[320];
    snprintf(pattern, sizeof(pattern), "%s\\*", dir);

    WIN32_FIND_DATA fd;
    HANDLE h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (out->count >= MCPSP_MAX_WORLDS)
            break;

        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(fd.cFileName, ".") != 0 &&
            strcmp(fd.cFileName, "..") != 0)
        {
            int n = out->count;
            snprintf(out->names[n], sizeof(out->names[0]), "%.63s", fd.cFileName);

            SYSTEMTIME localTime;
            if (FileTimeToLocalFileTime(&fd.ftLastWriteTime, &fd.ftLastWriteTime) &&
                FileTimeToSystemTime(&fd.ftLastWriteTime, &localTime))
            {
                snprintf(out->dates[n], sizeof(out->dates[0]),
                         "%02d/%02d/%02d %02d:%02d",
                         (int)localTime.wMonth, (int)localTime.wDay,
                         (int)(localTime.wYear % 100),
                         (int)localTime.wHour, (int)localTime.wMinute);
            }
            else
            {
                strcpy(out->dates[n], "Unknown");
            }

            out->gameModes[n] = 0;
            out->seeds[n] = 0;
            out->worldTypes[n] = WORLD_TYPE_OLD;
            out->genMasks[n] = GEN_FEATURES_ALL_ON;
            snprintf(out->displayNames[n], sizeof(out->displayNames[0]),
                     "%.63s", fd.cFileName);

            char dirRel[320];
            char dirAbs[320];
            snprintf(dirRel, sizeof(dirRel), "saves/%s", fd.cFileName);
            snprintf(dirAbs, sizeof(dirAbs), "%s", savePath(dirRel));

            char infoPath[360];
            snprintf(infoPath, sizeof(infoPath), "%s\\level.txt", dirAbs);

            FILE* info = fopen(infoPath, "rb");
            if (info)
            {
                char buf[256];
                size_t got = fread(buf, 1, sizeof(buf) - 1, info);
                fclose(info);

                if (got > 0)
                {
                    char* nl;
                    buf[got] = '\0';
                    out->gameModes[n] = atoi(buf);

                    nl = strchr(buf, '\n');
                    if (nl && nl[1] != '\0')
                    {
                        char* name = nl + 1;
                        char* end = strchr(name, '\n');
                        if (end && end[1] != '\0')
                        {
                            out->seeds[n] = strtol(end + 1, NULL, 10);

                            char* end2 = strchr(end + 1, '\n');
                            if (end2 && end2[1] != '\0')
                            {
                                out->worldTypes[n] = atoi(end2 + 1);

                                char* end3 = strchr(end2 + 1, '\n');
                                if (end3 && end3[1] != '\0')
                                    out->genMasks[n] = atoi(end3 + 1);
                            }
                        }

                        if (end) *end = '\0';
                        if (name[0] != '\0')
                            snprintf(out->displayNames[n],
                                     sizeof(out->displayNames[0]), "%s", name);
                    }
                }
            }
            else
            {
                if (LevelStorage::readInfo(dirAbs, out->displayNames[n],
                                           sizeof(out->displayNames[0]),
                                           &out->gameModes[n], &out->seeds[n]))
                {
                    if (out->displayNames[n][0] == '\0')
                        snprintf(out->displayNames[n],
                                 sizeof(out->displayNames[0]), "%.63s", fd.cFileName);
                }
            }

            out->count++;
        }
    }
    while (FindNextFile(h, &fd));

    FindClose(h);

    long keys[MCPSP_MAX_WORLDS];
    int i;
    int j;

    for (i = 0; i < out->count; i++)
    {
        keys[i] = 0;

        char p[360];
        SYSTEMTIME t;
        snprintf(p, sizeof(p), "saves/%s/lastplayed", out->names[i]);
        if (!getFileDate(savePath(p), &t))
        {
            snprintf(p, sizeof(p), "saves/%s/level.dat", out->names[i]);
            getFileDate(savePath(p), &t);
        }

        if (t.wYear != 0)
            keys[i] = dateKey(t);
    }

    for (i = 1; i < out->count; i++)
    {
        j = i;
        while (j > 0 && keys[j] > keys[j - 1])
        {
            long k = keys[j];
            keys[j] = keys[j - 1];
            keys[j - 1] = k;

            char tmp[64];
            memcpy(tmp, out->names[j], 64);
            memcpy(out->names[j], out->names[j - 1], 64);
            memcpy(out->names[j - 1], tmp, 64);

            memcpy(tmp, out->displayNames[j], 64);
            memcpy(out->displayNames[j], out->displayNames[j - 1], 64);
            memcpy(out->displayNames[j - 1], tmp, 64);

            memcpy(tmp, out->dates[j], 64);
            memcpy(out->dates[j], out->dates[j - 1], 64);
            memcpy(out->dates[j - 1], tmp, 64);

            int g = out->gameModes[j];
            out->gameModes[j] = out->gameModes[j - 1];
            out->gameModes[j - 1] = g;

            long sd = out->seeds[j];
            out->seeds[j] = out->seeds[j - 1];
            out->seeds[j - 1] = sd;

            int wt = out->worldTypes[j];
            out->worldTypes[j] = out->worldTypes[j - 1];
            out->worldTypes[j - 1] = wt;

            int gm2 = out->genMasks[j];
            out->genMasks[j] = out->genMasks[j - 1];
            out->genMasks[j - 1] = gm2;

            j--;
        }
    }
}

void worldListTouch(const char* absDir)
{
    char p[336];
    snprintf(p, sizeof(p), "%s\\lastplayed", absDir);

    FILE* f = fopen(p, "ab");
    if (f)
    {
        fputc('1', f);
        fclose(f);
    }
}

bool worldListCreate(WorldList* list, const char* inName, char* outName,
                     int gamemode, long seed, int worldType, int genMask)
{
    if (list->count >= MCPSP_MAX_WORLDS)
        return false;

    char displayName[64];
    if (inName && inName[0] != '\0')
        snprintf(displayName, sizeof(displayName), "%s", inName);
    else
        strcpy(displayName, "New world");

    char candidate[64];
    snprintf(candidate, sizeof(candidate), "%s", displayName);

    for (int suffix = 0; suffix < 60; suffix++)
    {
        if (suffix > 0)
            strcat(candidate, "-");

        bool taken = false;
        int i;
        for (i = 0; i < list->count; i++)
        {
            if (strcmp(list->names[i], candidate) == 0)
            {
                taken = true;
                break;
            }
        }

        if (!taken)
            break;
    }

    char full[320];
    snprintf(full, sizeof(full), "saves/%s", candidate);
    CreateDirectory(savePath(full), NULL);

    char infoPath[320];
    snprintf(infoPath, sizeof(infoPath), "%s\\level.txt", savePath(full));

    FILE* fd = fopen(infoPath, "wb");
    if (fd)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "%d\n%s\n%ld\n%d\n%d\n",
                 gamemode, displayName, seed, worldType, genMask);
        fwrite(buf, 1, strlen(buf), fd);
        fclose(fd);
    }

    snprintf(list->names[list->count], sizeof(list->names[0]), "%s", candidate);
    snprintf(list->displayNames[list->count],
             sizeof(list->displayNames[0]), "%s", displayName);
    snprintf(list->dates[list->count], sizeof(list->dates[0]), "Just now");

    list->gameModes[list->count] = gamemode;
    list->seeds[list->count] = seed;
    list->worldTypes[list->count] = worldType;
    list->genMasks[list->count] = genMask;

    strncpy(outName, candidate, 64);
    list->count++;
    return true;
}

static void deleteDirectoryContents(const char* dir)
{
    char pattern[360];
    WIN32_FIND_DATA fd;
    HANDLE h;

    snprintf(pattern, sizeof(pattern), "%s\\*", dir);
    h = FindFirstFile(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char child[400];
        snprintf(child, sizeof(child), "%s\\%s", dir, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            deleteDirectoryContents(child);
            RemoveDirectory(child);
        }
        else
        {
            DeleteFile(child);
        }
    }
    while (FindNextFile(h, &fd));

    FindClose(h);
}

bool worldListDelete(WorldList* list, int index)
{
    if (index < 0 || index >= list->count)
        return false;

    char dirRel[320];
    char dirFull[320];

    snprintf(dirRel, sizeof(dirRel), "saves/%s", list->names[index]);
    snprintf(dirFull, sizeof(dirFull), "%s", savePath(dirRel));

    deleteDirectoryContents(dirFull);
    RemoveDirectory(dirFull);

    int i;
    for (i = index; i < list->count - 1; i++)
    {
        strcpy(list->names[i], list->names[i + 1]);
        strcpy(list->displayNames[i], list->displayNames[i + 1]);
        strcpy(list->dates[i], list->dates[i + 1]);
        list->gameModes[i] = list->gameModes[i + 1];
        list->seeds[i] = list->seeds[i + 1];
        list->worldTypes[i] = list->worldTypes[i + 1];
        list->genMasks[i] = list->genMasks[i + 1];
    }

    list->count--;
    return true;
}
