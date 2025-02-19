#ifndef _LIBRETRO_WINX68K_COMMON_H
#define _LIBRETRO_WINX68K_COMMON_H

#ifdef _WIN32
#include "windows.h"
#endif

#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include "../win32api/windows.h"
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#undef FASTCALL
#define FASTCALL

#define	__stdcall

#ifdef PSP
#ifdef MAX_PATH
#undef MAX_PATH
#endif
#define MAX_PATH 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

void Error(const char* s);

#ifdef __cplusplus
}
#endif

#endif
