#include "platform/path.h"

#include <windows.h>
#include <cstdio>
#include <cstring>

static char g_base[MAX_PATH] = "";

static void ensureTrailingSlash(void)
{
    size_t n = strlen(g_base);
    if (n == 0) return;
    if (g_base[n - 1] != '\\' && g_base[n - 1] != '/')
    {
        if (n + 1 < sizeof(g_base))
        {
            g_base[n] = '\\';
            g_base[n + 1] = '\0';
        }
    }
}

void pathInit(const char* argv0)
{
    DWORD n;

    g_base[0] = '\0';

    n = GetModuleFileNameA(NULL, g_base, sizeof(g_base));
    if (n == 0 || n >= sizeof(g_base))
    {
        if (argv0 && argv0[0])
            strncpy(g_base, argv0, sizeof(g_base) - 1);
        else
            strcpy(g_base, ".");
        g_base[sizeof(g_base) - 1] = '\0';
    }

    char* slash = strrchr(g_base, '\\');
    char* slash2 = strrchr(g_base, '/');
    if (slash2 && (!slash || slash2 > slash))
        slash = slash2;

    if (slash)
        slash[1] = '\0';
    else
        strcpy(g_base, ".\\");

    ensureTrailingSlash();
}

const char* assetPath(const char* rel)
{
    static char buf[MAX_PATH * 2];
    if (!rel) rel = "";
    snprintf(buf, sizeof(buf), "%s%s", g_base, rel);
    return buf;
}

const char* savePath(const char* rel)
{
    return assetPath(rel);
}

const char* pathDevice(void)
{
    static char dev[8];
    if (g_base[0] == '\\' && g_base[1] != '\\')
    {
        const char* p = strchr(g_base + 1, ':');
        if (p && p - g_base < (int)sizeof(dev))
        {
            size_t n = (size_t)(p - g_base + 1);
            memcpy(dev, g_base, n);
            dev[n] = '\0';
            return dev;
        }
    }

    return "";
}

void savePathInit(void)
{
    CreateDirectory(savePath("saves"), NULL);
}
