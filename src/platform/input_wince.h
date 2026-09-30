#ifndef MCPECE_INPUT_WINCE_H
#define MCPECE_INPUT_WINCE_H

/*
 * Compatibility input packet used by the portable player/game code.
 * These names intentionally keep the old PSP-facing interfaces stable while
 * the actual input source is Windows CE keyboard/touch hardware.
 */
struct SceCtrlData
{
    unsigned int Buttons;
    unsigned char Lx;
    unsigned char Ly;
    unsigned char Rsrv[6];
};

#define PSP_CTRL_SELECT   0x00000001u
#define PSP_CTRL_START    0x00000002u
#define PSP_CTRL_UP       0x00000004u
#define PSP_CTRL_RIGHT    0x00000008u
#define PSP_CTRL_DOWN     0x00000010u
#define PSP_CTRL_LEFT     0x00000020u
#define PSP_CTRL_LTRIGGER 0x00000040u
#define PSP_CTRL_RTRIGGER 0x00000080u
#define PSP_CTRL_TRIANGLE 0x00000100u
#define PSP_CTRL_CIRCLE   0x00000200u
#define PSP_CTRL_CROSS    0x00000400u
#define PSP_CTRL_SQUARE   0x00000800u

#endif
