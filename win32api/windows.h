#ifndef	__NP2_WIN32EMUL_H__
#define	__NP2_WIN32EMUL_H__

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

typedef	signed int	INT;
typedef	signed long	LONG;

typedef	unsigned int	UINT;

#ifdef HAVE_C68k
#include "/m68000/c68k/core.h"
typedef	u8	BYTE;
typedef	u16	WORD;
typedef	u32	DWORD;
#else
typedef	unsigned char	BYTE;
typedef	unsigned short	WORD;
typedef	unsigned int	DWORD;
#endif

typedef	int		BOOL;

typedef	void		VOID;
typedef	void		*PVOID;
typedef	void		*LPVOID;
typedef	const void	*PCVOID;
typedef	long		*PLONG;
typedef	DWORD		*PDWORD;
typedef char		*LPSTR;
typedef const char	*LPCSTR;

typedef	void *		LPSECURITY_ATTRIBUTES;
typedef	void *		LPOVERLAPPED;

typedef void *		HANDLE;
typedef	HANDLE		HLOCAL;
typedef	HANDLE		HGLOBAL;

#ifndef FASTCALL
#define FASTCALL
#endif

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	MAX_PATH
#define	MAX_PATH	MAXPATHLEN
#endif

/*
 * DUMMY DEFINITION
 */
#define	WINAPI
#define	CALLBACK

#ifdef __GNUC__
#ifndef UNUSED
#define UNUSED __attribute ((unused))
#endif
#else
#define	UNUSED(v)	((void)(v))
#endif

#ifndef	INLINE
#ifdef _WIN32
#define	INLINE	_inline
#else
#define	INLINE	inline
#endif
#endif

#define	RGB(r,g,b)	((DWORD)((uint8_t)(r))|((WORD)((uint8_t)(g)))|((DWORD)((uint8_t)(b))))

/* for BITMAP */
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#define	BI_BITFIELDS		3

/* for dosio.c */
#define	GENERIC_READ			1
#define	GENERIC_WRITE			2

#define	OPEN_EXISTING			1
#define	CREATE_ALWAYS			2
#define	CREATE_NEW			3

#define	FILE_SHARE_READ			0x00000001  
#define	FILE_SHARE_WRITE		0x00000002  
#define	FILE_SHARE_DELETE		0x00000004  

#define	FILE_ATTRIBUTE_READONLY		0x01
#define	FILE_ATTRIBUTE_HIDDEN		0x02
#define	FILE_ATTRIBUTE_SYSTEM		0x04
#define	FILE_ATTRIBUTE_VOLUME		0x08
#define	FILE_ATTRIBUTE_DIRECTORY	0x10
#define	FILE_ATTRIBUTE_ARCHIVE		0x20
#define	FILE_ATTRIBUTE_NORMAL		0x40

#define	INVALID_HANDLE_VALUE		(HANDLE)-1

#define	NO_ERROR			0
#define	ERROR_FILE_NOT_FOUND		2
#define	ERROR_SHARING_VIOLATION		32

#define	FILE_BEGIN			0
#define	FILE_CURRENT			1
#define	FILE_END			2


/*
 * replace
 */
#define	timeGetTime()		FAKE_GetTickCount()

/*
 * WIN32 structure
 */
typedef struct {
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct {
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} __attribute__ ((packed)) BITMAPINFOHEADER;

typedef struct {
	BYTE	rgbBlue;
	BYTE	rgbGreen;
	BYTE	rgbRed;
	BYTE	rgbReserved;
} __attribute__ ((packed)) RGBQUAD;

typedef struct {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD			bmiColors[1];
} __attribute__ ((packed)) BITMAPINFO;

typedef struct {
	DWORD	top;
	DWORD	left;
	DWORD	bottom;
	DWORD	right;
} RECT;

typedef struct {
	WORD	x;
	WORD	y;
} POINT;

/*
 * prototype
 */
#ifdef __cplusplus
extern "C" {
#endif
BOOL	WINAPI WritePrivateProfileString(LPCSTR, LPCSTR, LPCSTR, LPCSTR);

DWORD	WINAPI FAKE_GetLastError(void);
#ifdef __cplusplus
};
#endif

#include "peace.h"

#endif /* __NP2_WIN32EMUL_H__ */
