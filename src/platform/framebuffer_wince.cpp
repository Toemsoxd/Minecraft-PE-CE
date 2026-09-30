#include "framebuffer_wince.h"
#include <string.h>

static const unsigned char font5x7[][5] = {
 {0,0,0,0,0},{0x1F,0x05,0x1F,0x14,0x14},{0x1E,0x15,0x1E,0x15,0x1E},
 {0x0E,0x11,0x10,0x11,0x0E},{0x1E,0x15,0x15,0x15,0x1E},{0x1F,0x10,0x1E,0x10,0x1F},
 {0x1F,0x10,0x1E,0x10,0x10},{0x0E,0x10,0x17,0x11,0x0F},{0x15,0x15,0x1F,0x15,0x15},
 {0x1F,0x04,0x04,0x04,0x1F},{0x07,0x02,0x02,0x12,0x0C},{0x14,0x12,0x1C,0x12,0x14},
 {0x10,0x10,0x10,0x10,0x1F},{0x11,0x1B,0x15,0x11,0x11},{0x11,0x19,0x15,0x13,0x11},
 {0x0E,0x11,0x11,0x11,0x0E},{0x1E,0x11,0x1E,0x10,0x10},{0x0E,0x11,0x11,0x15,0x0E},
 {0x1E,0x11,0x1E,0x12,0x11},{0x0F,0x10,0x0E,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04},
 {0x11,0x11,0x11,0x11,0x0E},{0x11,0x11,0x11,0x0A,0x04},{0x11,0x11,0x15,0x1B,0x11},
 {0x11,0x0A,0x04,0x0A,0x11},{0x11,0x0A,0x04,0x04,0x04},{0x1F,0x02,0x04,0x08,0x1F}
};
static int glyph(TCHAR c){ if(c>='A'&&c<='Z') return (int)(c-'A')+1; return 0; }
FramebufferWince::FramebufferWince():m_pixels(0){ZeroMemory(&m_bmi,sizeof(m_bmi));}
FramebufferWince::~FramebufferWince(){shutdown();}
bool FramebufferWince::init(){
 if(m_pixels)return true; m_pixels=new unsigned int[FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT]; if(!m_pixels)return false;
 ZeroMemory(&m_bmi,sizeof(m_bmi)); m_bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); m_bmi.bmiHeader.biWidth=FRAMEBUFFER_WIDTH; m_bmi.bmiHeader.biHeight=-FRAMEBUFFER_HEIGHT; m_bmi.bmiHeader.biPlanes=1; m_bmi.bmiHeader.biBitCount=32; m_bmi.bmiHeader.biCompression=BI_RGB; clear(0xFF000000u); return true;
}
void FramebufferWince::shutdown(){delete [] m_pixels;m_pixels=0;}
void FramebufferWince::clear(unsigned int c){if(!m_pixels)return;for(int i=0;i<FRAMEBUFFER_WIDTH*FRAMEBUFFER_HEIGHT;i++)m_pixels[i]=c;}
void FramebufferWince::fillRect(int x,int y,int w,int h,unsigned int c){
 if(!m_pixels||w<=0||h<=0)return; if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>FRAMEBUFFER_WIDTH)w=FRAMEBUFFER_WIDTH-x; if(y+h>FRAMEBUFFER_HEIGHT)h=FRAMEBUFFER_HEIGHT-y; if(w<=0||h<=0)return;
 for(int yy=y;yy<y+h;yy++){unsigned int* p=m_pixels+yy*FRAMEBUFFER_WIDTH+x;for(int xx=0;xx<w;xx++)p[xx]=c;}
}
void FramebufferWince::drawText(int x,int y,const TCHAR* s,unsigned int c,bool centered){
 if(!m_pixels||!s)return; int len=0;while(s[len])len++;int scale=2;int width=len*6*scale;int sx=centered?x-width/2:x;
 for(int n=0;n<len;n++){int g=glyph(s[n]);if(!g){sx+=6*scale;continue;}for(int col=0;col<5;col++)for(int row=0;row<7;row++)if(font5x7[g][col]&(1<<row))fillRect(sx+col*scale,y+row*scale,scale,scale,c);sx+=6*scale;}
}
void FramebufferWince::present(HDC hdc,const RECT* dirty){
 if(!m_pixels||!hdc)return;RECT r={0,0,FRAMEBUFFER_WIDTH,FRAMEBUFFER_HEIGHT};if(dirty)r=*dirty;int w=r.right-r.left,h=r.bottom-r.top;if(w<=0||h<=0)return;/* CE 4.2: no desktop StretchBlt mode dependency. */StretchDIBits(hdc,r.left,r.top,w,h,r.left,r.top,w,h,m_pixels,&m_bmi,DIB_RGB_COLORS,SRCCOPY);
}
void FramebufferWince::presentWindow(HWND hwnd){
 if(!hwnd)return;HDC dc=GetDC(hwnd);if(dc){RECT r;GetClientRect(hwnd,&r);StretchDIBits(dc,0,0,r.right,r.bottom,0,0,FRAMEBUFFER_WIDTH,FRAMEBUFFER_HEIGHT,m_pixels,&m_bmi,DIB_RGB_COLORS,SRCCOPY);ReleaseDC(hwnd,dc);}
}
