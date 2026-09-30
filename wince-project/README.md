# eVC++ workspace

Open **Minecraft WCE Edition.vcw** in eMbedded Visual C++ 4.0 SP2.

The workspace contains a legacy `.vcp` project with the current CE source set. It keeps ARMV4 and ARMV4I configurations; the HTC S730 target is **ARMV4**.

The project adds `src` to the include path and references source files relative to the workspace directory.

The PSP-only entry point, PSP GPU sources, and PSP-only malloc/profiler/savedata/png backends are intentionally excluded. The CE replacements remain in the source tree.

## Textures

The CE software renderer now samples the existing `data/images/terrain_565.h` 256x256 RGB565 MCPE terrain atlas directly, so the first CE renderer no longer uses flat placeholder block colors.
