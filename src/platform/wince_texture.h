#ifndef MCPSP_PLATFORM_WINCE_TEXTURE_H
#define MCPSP_PLATFORM_WINCE_TEXTURE_H

/*
 * CE texture sampler for the MCPE terrain atlas.
 *
 * The atlas is the existing 256x256 16-bit RGB565 conversion of the
 * MCPE terrain.png.  Keeping the atlas as a static C array avoids requiring
 * libpng on the HTC S730 and keeps the renderer deterministic on CE 4.2.
 */
bool winceTextureInit();
unsigned int winceTextureSample(int col, int row, float u, float v,
                                unsigned int tint);

#endif
