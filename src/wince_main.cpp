#include <windows.h>

// Windows CE bootstrap for the Minecraft-PE-CE port.
// This intentionally has no PSP dependencies: it is the first ARMV4
// executable used to verify the eVC++/Windows CE build environment.
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR    lpCmdLine,
                   int       nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    MessageBox(NULL,
               TEXT("Minecraft-PE-CE\n\nWindows CE ARMV4 bootstrap OK!"),
               TEXT("Minecraft-PE-CE"),
               MB_OK | MB_ICONINFORMATION);

    return 0;
}