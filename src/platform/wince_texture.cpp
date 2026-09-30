#include "platform/wince_texture.h"
#include "data/images/terrain_565.h"

#include <math.h>

static bool g_textureReady = false;

/*
 * terrain_565 contains a 13-word PVR header followed by 256*256 RGB565
 * pixels packed two 16-bit pixels per 32-bit word.
 */
static const unsigned short* terrainPixels()
{
    return ((const unsigned short*)terrain_565) + 26;
}

static unsigned int rgb565To8888(unsigned short p)
{
    unsigned int r = (unsigned int)((p >> 11) & 31);
    unsigned int g = (unsigned int)((p >> 5) & 63);
    unsigned int b = (unsigned int)(p & 31);

    r = (r * 255u + 15u) / 31u;
    g = (g * 255u + 31u) / 63u;
    b = (b * 255u + 15u) / 31u;

    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

static unsigned int multiplyTint(unsigned int c, unsigned int tint)
{
    unsigned int r = ((c >> 16) & 255u) * ((tint >> 16) & 255u) / 255u;
    unsigned int g = ((c >> 8) & 255u) * ((tint >> 8) & 255u) / 255u;
    unsigned int b = (c & 255u) * (tint & 255u) / 255u;
    unsigned int a = (tint >> 24) & 255u;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

bool winceTextureInit()
{
    g_textureReady = true;
    return true;
}

unsigned int winceTextureSample(int col, int row, float u, float v,
                                unsigned int tint)
{
    int tx, ty;
    const unsigned short* pixels;

    if (!g_textureReady)
        winceTextureInit();

    if (col < 0) col = 0;
    if (col > 15) col = 15;
    if (row < 0) row = 0;
    if (row > 15) row = 15;

    if (u < 0.0f) u = 0.0f;
    if (u > 0.99999f) u = 0.99999f;
    if (v < 0.0f) v = 0.0f;
    if (v > 0.99999f) v = 0.99999f;

    tx = col * 16 + (int)(u * 16.0f);
    ty = row * 16 + (int)(v * 16.0f);

    pixels = terrainPixels();
    return multiplyTint(rgb565To8888(pixels[ty * 256 + tx]), tint);
}
