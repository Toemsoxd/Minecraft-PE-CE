#include <windows.h>
#include <math.h>
#include "platform/time_wince.h"
#include "platform/framebuffer_wince.h"

static HWND g_window = 0;
static FramebufferWince g_framebuffer;
static bool g_running = true;
static bool g_game = false;
static bool g_paused = false;
static int g_menuSelection = 0;
static float g_angle = 0.0f;

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

struct Vec3 { float x, y, z; };
struct ScreenPoint { int x, y; float z; };

static Vec3 rotatePoint(Vec3 v, float ax, float ay)
{
    float cx = (float)cos(ax), sx = (float)sin(ax);
    float cy = (float)cos(ay), sy = (float)sin(ay);
    float x = v.x * cy + v.z * sy;
    float z = -v.x * sy + v.z * cy;
    float y = v.y * cx - z * sx;
    z = v.y * sx + z * cx;
    Vec3 r = {x, y, z};
    return r;
}

static bool projectPoint(Vec3 v, ScreenPoint* out)
{
    float z = v.z + 5.0f;
    if (z <= 0.2f) return false;
    float scale = 125.0f / z;
    out->x = (int)(160.0f + v.x * scale);
    out->y = (int)(120.0f - v.y * scale);
    out->z = z;
    return true;
}

static void drawTriangle(const ScreenPoint& a, const ScreenPoint& b,
                         const ScreenPoint& c, unsigned int color)
{
    int minx = a.x, maxx = a.x, miny = a.y, maxy = a.y;
    if (b.x < minx) minx = b.x; if (c.x < minx) minx = c.x;
    if (b.x > maxx) maxx = b.x; if (c.x > maxx) maxx = c.x;
    if (b.y < miny) miny = b.y; if (c.y < miny) miny = c.y;
    if (b.y > maxy) maxy = b.y; if (c.y > maxy) maxy = c.y;
    if (minx < 0) minx = 0; if (miny < 0) miny = 0;
    if (maxx >= FRAMEBUFFER_WIDTH) maxx = FRAMEBUFFER_WIDTH - 1;
    if (maxy >= FRAMEBUFFER_HEIGHT) maxy = FRAMEBUFFER_HEIGHT - 1;

    long area = (long)(b.x-a.x)*(c.y-a.y) - (long)(b.y-a.y)*(c.x-a.x);
    if (area == 0) return;
    int y;
    int x;
    for (y=miny; y<=maxy; ++y) {
        for (x=minx; x<=maxx; ++x) {
            long w0=(long)(b.x-a.x)*(y-a.y)-(long)(b.y-a.y)*(x-a.x);
            long w1=(long)(c.x-b.x)*(y-b.y)-(long)(c.y-b.y)*(x-b.x);
            long w2=(long)(a.x-c.x)*(y-c.y)-(long)(a.y-c.y)*(x-c.x);
            if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0))
                g_framebuffer.drawPixel(x,y,color);
        }
    }
}

static void drawCube()
{
    g_framebuffer.clear(0xFF18202Au);
    g_framebuffer.fillRect(0, 178, FRAMEBUFFER_WIDTH, 62, 0xFF354A35u);

    const Vec3 verts[8] = {
        {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
        {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
    };
    const int faces[6][4] = {
        {0,1,2,3},{4,7,6,5},{0,4,5,1},
        {3,2,6,7},{0,3,7,4},{1,5,6,2}
    };
    const unsigned int colors[6] = {
        0xFF8B5A32u,0xFF704522u,0xFF99663Au,
        0xFFB07848u,0xFF76502Eu,0xFF5E3B1Eu
    };
    ScreenPoint p[8];
    int i;
    for (i=0;i<8;i++) {
        Vec3 r=rotatePoint(verts[i],g_angle*0.72f,g_angle);
        if (!projectPoint(r,&p[i])) return;
    }

    int order[6] = {0,1,2,3,4,5};
    int i;
    int j;
    for (i=0;i<5;i++) for (j=i+1;j<6;j++) {
        float zi=(p[faces[order[i]][0]].z+p[faces[order[i]][1]].z+p[faces[order[i]][2]].z+p[faces[order[i]][3]].z);
        float zj=(p[faces[order[j]][0]].z+p[faces[order[j]][1]].z+p[faces[order[j]][2]].z+p[faces[order[j]][3]].z);
        if (zi < zj) { int t=order[i]; order[i]=order[j]; order[j]=t; }
    }
    int oi;
    for (oi=0;oi<6;oi++) {
        int f=order[oi];
        ScreenPoint a=p[faces[f][0]], b=p[faces[f][1]];
        ScreenPoint c=p[faces[f][2]], d=p[faces[f][3]];
        drawTriangle(a,b,c,colors[f]);
        drawTriangle(a,c,d,colors[f]);
    }

    int f;
    for (f=0;f<6;f++) {
        int i0=faces[f][0],i1=faces[f][1],i2=faces[f][2],i3=faces[f][3];
        g_framebuffer.drawLine(p[i0].x,p[i0].y,p[i1].x,p[i1].y,0xFF2A1A10u);
        g_framebuffer.drawLine(p[i1].x,p[i1].y,p[i2].x,p[i2].y,0xFF2A1A10u);
        g_framebuffer.drawLine(p[i2].x,p[i2].y,p[i3].x,p[i3].y,0xFF2A1A10u);
        g_framebuffer.drawLine(p[i3].x,p[i3].y,p[i0].x,p[i0].y,0xFF2A1A10u);
    }

    g_framebuffer.drawText(160, 12, TEXT("3D RENDERER TEST"), 0xFFFFFFFFu, true);
    g_framebuffer.drawText(160, 28, TEXT("WASD MOVE  ARROWS LOOK"), 0xFFCCCCCCu, true);
    g_framebuffer.drawText(160, 44, TEXT("SPACE JUMP  P BREAK  L PLACE"), 0xFFCCCCCCu, true);

    if (g_paused) {
        g_framebuffer.fillRect(70,88,180,64,0xDD111111u);
        g_framebuffer.drawText(160,106,TEXT("PAUSED"),0xFFFFFFFFu,true);
        g_framebuffer.drawText(160,126,TEXT("ENTER TO RESUME"),0xFFE0E0E0u,true);
    }
}

static void updateMenu()
{
    static bool oldUp=false, oldDown=false, oldEnter=false;
    bool up=keyPressed(VK_UP), down=keyPressed(VK_DOWN), enter=keyPressed(VK_RETURN);
    if (up && !oldUp) g_menuSelection=0;
    if (down && !oldDown) g_menuSelection=1;
    if (enter && !oldEnter && g_menuSelection==0) { g_game=true; g_paused=false; }
    oldUp=up; oldDown=down; oldEnter=enter;
}

static void updateGame()
{
    static bool oldEnter=false;
    bool enter=keyPressed(VK_RETURN);
    if (enter && !oldEnter) g_paused=!g_paused;
    oldEnter=enter;

    if (!g_paused) {
        if (keyPressed(VK_LEFT)) g_angle -= 0.035f;
        if (keyPressed(VK_RIGHT)) g_angle += 0.035f;
        if (keyPressed('A')) g_angle -= 0.012f;
        if (keyPressed('D')) g_angle += 0.012f;
        if (keyPressed('W') || keyPressed('S')) g_angle += 0.004f;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
    ZeroMemory(g_keys,sizeof(g_keys));
    if (!g_framebuffer.init() || !createWindow(hInstance)) return 1;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    unsigned long nextFrame = timeMillis();

    while (g_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { g_running=false; break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_game) {
            updateGame();
            drawCube();
            if (!g_paused) g_angle += 0.018f;
        } else {
            updateMenu();
            drawTitleScreen();
        }

        g_framebuffer.presentWindow(g_window);

        nextFrame += 16;
        long wait=(long)(nextFrame-timeMillis());
        if (wait>0) Sleep((DWORD)wait);
        else nextFrame=timeMillis();
    }

    g_framebuffer.shutdown();
    return 0;
}
