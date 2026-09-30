#ifndef MCPECE_FRAMEBUFFER_WINCE_H
#define MCPECE_FRAMEBUFFER_WINCE_H
#include <windows.h>
#define FRAMEBUFFER_WIDTH 320
#define FRAMEBUFFER_HEIGHT 240
class FramebufferWince {
public:
    FramebufferWince(); ~FramebufferWince();
    bool init(); void shutdown();
    void clear(unsigned int argb);
    void fillRect(int x,int y,int w,int h,unsigned int argb);
    void drawText(int x,int y,const TCHAR* text,unsigned int argb,bool centered);
    void present(HDC hdc,const RECT* dirty);
    void presentWindow(HWND hwnd);
private:
    unsigned int* m_pixels; BITMAPINFO m_bmi;
};
#endif
