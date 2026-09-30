# Windows CE / eVC++ port

This branch contains the Windows CE port work. The original PSP backend remains as the reference implementation while platform-specific pieces are replaced incrementally.

## Current milestone: first visible title screen

The CE entry point is now src/wince_main.cpp. It creates a native Windows CE window, initializes a 320x240 software framebuffer, runs a timed render loop, and presents a procedural Minecraft-style title screen.

The first title screen is intentionally procedural: it does not yet depend on the PSP PNG/texture/GU stack. The real Minecraft title assets will be connected after the CE texture/asset path is ready.

### eVC++ setup

Use eMbedded Visual C++ 4.0 SP2:

1. Create a WCE Application project.
2. Select the Windows CE .NET 4.2 SDK.
3. Select ARMV4.
4. Prefer an Empty Project.
5. Add these CE source files:
   - src/wince_main.cpp
   - src/platform/framebuffer_wince.cpp
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

## Next stages

1. Replace the procedural title with the real title/texture asset path.
2. Build the CE 2D renderer and texture abstraction.
3. Port the world/chunk software rendering path.
4. Add CE input after the screen is stable.
5. Replace PSP-only filesystem/audio/services.
6. Reconnect portable Minecraft world/game code.
7. Compile the complete tree as ARMV4.

The final executable remains an ARMV4-compatible Windows CE program, even though the S730 CPU itself is an ARM11/ARMv6-class processor.
