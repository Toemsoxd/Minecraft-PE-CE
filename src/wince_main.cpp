#include <windows.h>
#include <math.h>
#include "platform/time_wince.h"
#include "platform/framebuffer_wince.h"
#include "platform/wince_world.h"
#include "platform/wince_renderer.h"

unsigned long g_timeBootMs = 0;
float g_gameSeconds = 0.0f;
bool g_gameFrozen = true;

static HWND g_window = 0;
static FramebufferWince g_framebuffer;
static bool g_running = true;
static bool g_game = false;
static bool g_worldLoadAttempted = false;
static bool g_worldLoadOK = false;
static bool g_paused = false;
static int g_menuSelection = 0;
static WinceRenderer g_renderer;

static bool g_keys[256];

static LRESULT CALLBACK WinceWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_KEYDOWN:
        if (wParam < 256) g_keys[(int)wParam] = true;
        return 0;
    case WM_KEYUP:
        if (wParam < 256) g_keys[(int)wParam] = false;
        return 0;
    case WM_KILLFOCUS:
        ZeroMemory(g_keys, sizeof(g_keys));
        return 0;
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

static bool keyPressed(int key)
{
    return key >= 0 && key < 256 && g_keys[key];
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
    int y;
    int x;
    for (y = 0; y < FRAMEBUFFER_HEIGHT; y += 16) {
        for (x = 0; x < FRAMEBUFFER_WIDTH; x += 16) {
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

    if (g_menuSelection == 0)
        g_framebuffer.drawText(43, 126, TEXT(">"), 0xFFFFFFFFu, false);
    else
        g_framebuffer.drawText(43, 168, TEXT(">"), 0xFFFFFFFFu, false);

    g_framebuffer.drawText(160, 214, TEXT("ENTER: SELECT"), 0xFFDDDDDDu, true);
    g_framebuffer.drawText(160, 230, TEXT("HTC S730 / ARMV4"), 0xFFBBBBBBu, true);
}

static void drawWorldStatus()
{
    g_framebuffer.clear(0xFF315B78u);
    g_framebuffer.fillRect(0, 0, FRAMEBUFFER_WIDTH, 28, 0xFF1B2B38u);
    if (!g_worldLoadAttempted) {
        g_framebuffer.drawText(160, 72, TEXT("LOADING WORLD..."), 0xFFFFFFFFu, true);
        return;
    }
    if (!g_worldLoadOK) {
        g_framebuffer.drawText(160, 72, TEXT("WORLD LOAD FAILED"), 0xFFFF7070u, true);
        g_framebuffer.drawText(160, 104, TEXT("CHECK MEMORY / SAVES"), 0xFFFFFFFFu, true);
        return;
    }
    g_framebuffer.drawText(160, 14, TEXT("MINECRAFT PE CE"), 0xFFFFFFFFu, true);
    g_framebuffer.drawText(160, 62, TEXT("WORLD READY"), 0xFF80FF80u, true);
    g_framebuffer.drawText(160, 88, TEXT("STARTING VOXEL RENDERER"), 0xFFE0E0E0u, true);
    TCHAR buf[64];
    wsprintf(buf, TEXT("TERRAIN %d%%"), winceWorldProgress());
    g_framebuffer.drawText(160, 120, buf, 0xFFFFFFFFu, true);
}

static void drawPauseOverlay()
{
    g_framebuffer.fillRect(58, 82, 204, 76, 0xDD111111u);
    g_framebuffer.drawText(160, 99, TEXT("PAUSED"), 0xFFFFFFFFu, true);
    g_framebuffer.drawText(160, 121, TEXT("ENTER TO RESUME"), 0xFFE0E0E0u, true);
    g_framebuffer.drawText(160, 143, TEXT("WASD / ARROWS"), 0xFFBBBBBBu, true);
}

static void updateMenu()
{
    static bool oldUp=false, oldDown=false, oldEnter=false;
    bool up=keyPressed(VK_UP), down=keyPressed(VK_DOWN), enter=keyPressed(VK_RETURN);
    if (up && !oldUp) g_menuSelection=0;
    if (down && !oldDown) g_menuSelection=1;
    if (enter && !oldEnter && g_menuSelection==0) { g_game=true; g_paused=false; g_worldLoadAttempted=true; g_worldLoadOK=winceWorldStart(); }
    oldUp=up; oldDown=down; oldEnter=enter;
}

static void updateGame(float dt)
{
    static bool oldEnter = false;
    bool enter = keyPressed(VK_RETURN);
    if (enter && !oldEnter) g_paused = !g_paused;
    oldEnter = enter;
    if (!g_paused && g_worldLoadOK && g_worldBuilt)
        g_renderer.update(g_keys, dt);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    ZeroMemorint WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    ZeroMemory(g_keys, sizeof(g_keys));
    if (!g_framebuffer.init() || !createWindow(hInstance)) return 1;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    unsigned long lastFrame = timeMillis();
    unsigned long nextFrame = lastFrame;

    while (g_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running = false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        unsigned long now = timeMillis();
        float dt = (float)(now - lastFrame) / 1000.0f;
        lastFrame = now;
        if (dt > 0.10f) dt = 0.10f;
        g_gameSeconds += dt;

        if (!g_game) {
            updateMenu();
            drawTitleScreen();
        } else if (!g_worldLoadOK || !g_worldBuilt) {
            drawWorldStatus();
        } else {
            updateGame(dt);
            g_renderer.render(&g_framebuffer);
            if (g_paused) drawPauseOverlay();
        }

        g_framebuffer.presentWindow(g_window);

        nextFrame += 33;
        {
            long wait = (long)(nextFrame - timeMillis());
            if (wait > 0) Sleep((DWORD)wait);
            else nextFrame = timeMillis();
        }
    }

    winceWorldShutdown();
    g_framebuffer.shutdown();
    return 0;
}
