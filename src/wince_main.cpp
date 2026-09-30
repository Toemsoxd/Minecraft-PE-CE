#include <windows.h>
#include "platform/time_wince.h"
#include "platform/framebuffer_wince.h"

static HWND g_window = 0;
static FramebufferWince g_framebuffer;
static bool g_running = true;

static LRESULT CALLBACK WinceWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            g_framebuffer.present(hdc, &ps.rcPaint);
            EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_DESTROY:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool createWindow(HINSTANCE hInstance)
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WinceWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = TEXT("MinecraftPECEWindow");

    if (!RegisterClass(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    g_window = CreateWindow(TEXT("MinecraftPECEWindow"), TEXT("Minecraft PE CE"),
                            WS_VISIBLE, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT,
                            NULL, NULL, hInstance, NULL);
    return g_window != NULL;
}

static void drawTitleScreen()
{
    g_framebuffer.clear(0xFF202020u);
    for (int y = 0; y < FRAMEBUFFER_HEIGHT; y += 16) {
        for (int x = 0; x < FRAMEBUFFER_WIDTH; x += 16) {
            unsigned int c = (((x / 16) + (y / 16)) & 1) ? 0xFF59402Eu : 0xFF654A35u;
            g_framebuffer.fillRect(x, y, 16, 16, c);
        }
    }

    g_framebuffer.fillRect(28, 28, 264, 62, 0xFF111111u);
    g_framebuffer.drawText(160, 42, TEXT("MINECRAFT"), 0xFFFFFFFFu, true);
    g_framebuffer.drawText(160, 63, TEXT("POCKET EDITION"), 0xFFE0E0E0u, true);

    g_framebuffer.fillRect(65, 116, 190, 32, 0xFF5A5A5Au);
    g_framebuffer.fillRect(66, 117, 188, 30, 0xFF7A7A7Au);
    g_framebuffer.drawText(160, 132, TEXT("PLAY"), 0xFFFFFFFFu, true);

    g_framebuffer.fillRect(65, 158, 190, 32, 0xFF5A5A5Au);
    g_framebuffer.fillRect(66, 159, 188, 30, 0xFF7A7A7Au);
    g_framebuffer.drawText(160, 174, TEXT("OPTIONS"), 0xFFFFFFFFu, true);

    g_framebuffer.drawText(160, 214, TEXT("WINDOWS CE ARMV4"), 0xFFDDDDDDu, true);
    g_framebuffer.drawText(160, 230, TEXT("HTC S730"), 0xFFBBBBBBu, true);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    if (!g_framebuffer.init() || !createWindow(hInstance)) return 1;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    unsigned long nextFrame = timeMillis();

    while (g_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        drawTitleScreen();
        g_framebuffer.presentWindow(g_window);

        nextFrame += 16;
        long wait = (long)(nextFrame - timeMillis());
        if (wait > 0) Sleep((DWORD)wait);
        else nextFrame = timeMillis();
    }

    g_framebuffer.shutdown();
    return 0;
}
