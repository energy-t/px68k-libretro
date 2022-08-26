#ifndef _WINX68K_WINCORE_H
#define _WINX68K_WINCORE_H

#include <stdint.h>
#include "common.h"

#ifdef RFMDRV
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int rfd_sock;
#endif

#define vline HOGEvline /* workaround for redefinition of 'vline' */

#define		SCREEN_WIDTH		768
#define		SCREEN_HEIGHT		512

#define		FULLSCREEN_WIDTH	800
#define		FULLSCREEN_HEIGHT	600
#define		FULLSCREEN_POSX		((FULLSCREEN_WIDTH - SCREEN_WIDTH) / 2)
#define		FULLSCREEN_POSY		((FULLSCREEN_HEIGHT - SCREEN_HEIGHT) / 2)

#define TOSTR(s) #s
#define _TOSTR(s) TOSTR(s)
#define PX68KVERSTR _TOSTR(PX68K_VERSION)

extern	uint8_t*	FONT;

extern	uint16_t	VLINE_TOTAL;
extern	uint32_t	VLINE;
extern	uint32_t	vline;

extern	char	winx68k_dir[MAX_PATH];
extern	char	winx68k_ini[MAX_PATH];
extern	int	BIOS030Flag;

int WinX68k_Reset(void);
int pmain(int argc, char *argv[]);
void end_loop_retro(void);
void exec_app_retro(void);

#ifndef	_WINX68K_GTKWRAPPER_H
#define	_WINX68K_GTKWRAPPER_H

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <signal.h>

#define	NELEMENTS(array)	((int)(sizeof(array) / sizeof(array[0])))

#endif /* _WINX68K_GTKWRAPPER_H */
#endif /* _WINX68K_WINCORE_H */
