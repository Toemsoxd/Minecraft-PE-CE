# Windows CE / Visual Studio 2008 D3DM port

This branch contains the Windows CE port work. The original PSP backend remains as the reference implementation while platform-specific pieces are replaced incrementally.

## Current milestone: first playable terrain view

The CE entry point is src/wince_main.cpp. It creates a native Windows CE window, initializes a 320x240 software framebuffer, runs a timed render loop, starts the real MCPE world generator/storage path, and renders the loaded terrain through a new low-resolution software voxel raycaster. The camera supports WASD movement, arrow-key look, gravity, jumping, simple collision, block colors, face shading, and world lighting.

The title screen remains procedural, but PLAY now enters the actual CE world engine. The voxel renderer deliberately avoids the PSP GU stack and renders directly into the CE framebuffer. It is a compatibility-first milestone, not the final optimized renderer.

#
## Direct3D Mobile backend

The `wince-d3dm` branch adds a native Direct3D Mobile backend while retaining the software voxel renderer as a fallback.

The primary path is:

    WinMain
       |
       +-- WinceD3DMRenderer
              |
              +-- Direct3DMobileCreate
              +-- 320x240 R5G6B5 backbuffer
              +-- D16 depth buffer when the driver accepts it
              +-- fixed-function view/projection transforms
              +-- persistent D3DM vertex buffer
              +-- existing MCPE chunk meshes

The renderer consumes the existing `ChunkMesh` / `DrawVertex` data rather than rebuilding voxel geometry. Chunk positions stay in the existing packed 1/256-unit representation and are converted to D3DM float vertices when batches are uploaded.

The first D3DM stage intentionally renders the opaque terrain mesh with vertex colors. Water, leaves, no-mip materials, terrain atlas textures, entities and HUD are still separate stages. This keeps the first hardware path small enough to diagnose on the HTC S730 before adding more GPU state.

D3DM is attempted at startup. If `Direct3DMobileCreate` or device/vertex-buffer creation fails, the existing software renderer remains active.

### Visual Studio 2008

A native VS2008 Smart Device project and solution are included:

1. Open `wince-project/MinecraftPECE-WS2008.sln` in Visual Studio 2008.
2. Select `Release|Windows Mobile 6 Standard SDK (ARMV4I)`.
3. The project links `D3dm.lib` and `D3dmguid.lib` in addition to the existing CE libraries.
4. The project uses the Windows Mobile 6 Standard ARMV4I target and the existing `src/` tree.
5. Deploy the resulting `MinecraftPECE.exe` to the S730.

The project files are prepared from the current CE source list; a physical S730 build/deployment has **not** been performed by this repository change.

### D3DM API note

Direct3D Mobile is not desktop Direct3D 9. In particular, the backend uses vertex buffers and `DrawPrimitive`; it does not rely on `DrawPrimitiveUP`. Microsoft documents `IDirect3DMobileDevice` as providing vertex-buffer creation, stream binding, transforms, drawing and presentation, and the Windows CE D3DM SDK supplies `D3dm.lib` / `D3dmguid.lib`.

## Legacy eVC++ setup

Use eMbedded Visual C++ 4.0 SP2 with the Windows CE .NET 4.2 SDK and the **ARMV4** target.

The repository now includes a legacy eVC++ workspace under `wince-project/`:

1. Open `wince-project/Minecraft WCE Edition.vcw`.
2. Select **Win32 (WCE ARMV4) Release** for the HTC S730.
3. The `.vcp` already contains the current CE source set and uses `..\src` as its include path.
4. Do not add the PSP `src/main.cpp`; it remains the PSP entry point and is intentionally not part of the CE project.
5. Build the project.
6. Copy the resulting `.exe` to the HTC S730 and launch it.

The project also keeps ARMV4I configurations for compatibility/reference, but the S730 build target remains ARMV4.

### CE texture path

The first CE renderer now samples the existing `data/images/terrain_565.h` 256x256 RGB565 MCPE terrain atlas instead of using flat procedural block colors. The sampler uses the existing `Tile::getTexture()` mapping, including block metadata and tint values.

The atlas is embedded as C data, so this renderer does not require a libpng runtime dependency on the S730.

### Current architecture

    WinMain
       |
       +-- time_wince.h (GetTickCount)
       |
       +-- framebuffer_wince
              |
              +-- 320x240 ARGB software buffer
              +-- software rectangles/text
              +-- GDI StretchDIBits presentation

The render loop targets roughly 60 Hz using a millisecond Windows CE timer. No PSP headers, PSP GPU APIs, PSP input APIs, or PSP CPU assembly are required by this first visible milestone.

## PSP dependency policy

PSP files are not being deleted merely because the CE path no longer uses them. A PSP file is retained while it is still useful as reference code or is still required by another build. CE replacements are introduced at platform boundaries instead: timing, filesystem, synchronization, audio, input compatibility, mesh drawing, and framebuffer presentation.

When the CE tree is complete enough to build as a standalone eVC++ project, a final dependency audit will remove only PSP files with zero remaining consumers and will document any PSP reference files intentionally retained.

## Next stages

1. Replace the procedural title/voxel block colors with the CE texture/asset path.
2. Add block targeting, breaking, placing, and a proper hotbar.
3. Replace the compatibility voxel renderer with the optimized chunk-mesh software renderer.
4. Port CE input beyond keyboard emulation and connect the S730 QWERTY controls.
5. Finish save/load/player serialization validation on-device.
6. Audit every remaining PSP symbol/header and remove only dead PSP dependencies.
7. Produce the final eVC++ ARMV4 source-file manifest and build project instructions.

