#ifndef _WINX68K_WINDRAW_H
#define _WINX68K_WINDRAW_H

#include <stdint.h>

extern int winx, winy;
extern uint16_t WinDraw_Pal16B, WinDraw_Pal16R, WinDraw_Pal16G;

extern	int	WindowX;
extern	int	WindowY;
extern	int	kbd_x, kbd_y, kbd_w, kbd_h;

void WinDraw_InitWindowSize(uint16_t width, uint16_t height);
int WinDraw_Init(void);
void WinDraw_Cleanup(void);
void FASTCALL WinDraw_Draw(void);
void WinDraw_ShowMenu(int flag);
void WinDraw_DrawLine(void);
void WinDraw_ChangeSize(void);

void WinDraw_StartupScreen(void);
void WinDraw_CleanupScreen(void);

int WinDraw_MenuInit(void);
void WinDraw_DrawMenu(int menu_state, int mkey_pos, int mkey_y, int *mval_y);

extern struct menu_flist mfl;

void WinDraw_DrawMenufile(struct menu_flist *mfl);
void WinDraw_ClearMenuBuffer(void);

#endif /* _WINX68K_WINDRAW_H */
