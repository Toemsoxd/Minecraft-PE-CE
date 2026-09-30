# Windows CE / eVC++ port

This branch contains the Windows CE port work. The original PSP backend remains as the reference implementation while platform-specific pieces are replaced incrementally.

## Current milestone: first playable terrain view

The CE entry point is src/wince_main.cpp. It creates a native Windows CE window, initializes a 320x240 software framebuffer, runs a timed render loop, starts the real MCPE world generator/storage path, and renders the loaded terrain through a new low-resolution software voxel raycaster. The camera supports WASD movement, arrow-key look, gravity, jumping, simple collision, block colors, face shading, and world lighting.

The title screen remains procedural, but PLAY now enters the actual CE world engine. The voxel renderer deliberately avoids the PSP GU stack and renders directly into the CE framebuffer. It is a compatibility-first milestone, not the final optimized renderer.

### eVC++ setup

Use eMbedded Visual C++ 4.0 SP2:

1. Create a WCE Application project.
2. Select the Windows CE .NET 4.2 SDK.
3. Select ARMV4.
4. Prefer an Empty Project.
5. Add these CE source files:
   - src/wince_main.cpp
   - src/platform/framebuffer_wince.cpp\n   - src/platform/wince_renderer.cpp\n   - src/platform/wince_world.cpp
6. Add src to the compiler include path so headers such as platform/framebuffer_wince.h and platform/time_wince.h resolve.
7. Do not add the PSP src/main.cpp to the build; it is still the PSP entry point.
8. Build for Release or Debug.
9. Copy the resulting .exe to the HTC S730 and launch it.

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

