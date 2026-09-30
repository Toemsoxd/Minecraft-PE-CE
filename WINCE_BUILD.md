# Windows CE / eVC++ bootstrap

This branch contains the Windows CE port work. The original PSP backend is still present because it is the reference implementation; it is not expected to compile with eMbedded Visual C++ yet.

## First executable build

The first milestone is deliberately small: build src/wince_main.cpp as a native Windows CE ARMV4 executable and run it on the HTC S730.

Use eMbedded Visual C++ 4.0 SP2:

1. Create a WCE Application project.
2. Select the Windows CE .NET 4.2 SDK.
3. Select the ARMV4 CPU target.
4. Add src/wince_main.cpp to the project.
5. Remove the template .cpp/.rc source if necessary so there is only one WinMain.
6. Build for Release or Debug.
7. Copy the resulting .exe to the S730 and launch it.

Expected result:

> Minecraft-PE-CE — Windows CE ARMV4 bootstrap OK!

## Why this file exists

Do not replace src/main.cpp yet. src/main.cpp is still the PSP entry point and will be replaced only after the CE platform layer is ready. Likewise, the existing PSP GPU/audio files remain useful as references while the CE backend is implemented.

The next porting stages are:

1. CE application/window + timing
2. CE framebuffer/renderer
3. CE input
4. CE filesystem/save paths
5. Disable/replace PSP-only audio and other services
6. Reconnect the portable Minecraft world/game code
7. Compile the complete tree as ARMV4

The final executable should remain an ARMV4-compatible Windows CE program, not an ARMv6 build.