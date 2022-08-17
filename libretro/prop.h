#ifndef _WINX68K_CONFIG_H
#define _WINX68K_CONFIG_H

#include <stdint.h>
#include "common.h"

typedef struct
{
	DWORD SampleRate;
	DWORD BufferSize;
	int OPM_VOL;
	int PCM_VOL;
	int MCR_VOL;
	int MouseSpeed;
	int WindowFDDStat;
	int FullScrFDDStat;
	int MIDI_SW;
	int MIDI_Type;
	int MIDI_Reset;
	int JoyKey;
	int JoyKeyReverse;
	int JoyKeyJoy2;
	char HDImage[16][MAX_PATH];
	int ToneMap;
	char ToneMapFile[MAX_PATH];
	int XVIMode;
	int WinDrvFD;
	int WinStretch;
	int CDROM_ASPI;
	int CDROM_ASPI_Drive;
	int CDROM_IOCTRL_Drive;
	int CDROM_SCSIID;
	int CDROM_Enable;
	int SSTP_Enable;
	int SSTP_Port;
	int Sound_LPF;
	int SoundROMEO;
	int MIDIDelay;
	int MIDIAutoDelay;
	char FDDImage[2][MAX_PATH];
	int VkeyScale;
	int VbtnSwap;
	int JoyOrMouse;
	int HwJoyBtn[8];
	int NoWaitMode;
	uint8_t FrameRate;
	int PushVideoBeforeAudio;
	int AdjustFrameRates;
	int AudioDesyncHack;
	int MenuFontSize; /* font size of menu, 0 = normal, 1 = large */
	int joy1_select_mapping; /* used for keyboard to joypad map for P1 Select */
	int save_fdd_path;
	int save_hdd_path;
	/* Cpu clock in MHz */
	int clockmhz;
	/* RAM Size = size * 1024 * 1024 */
	int ram_size;
	/* Set controller type for each player to use
	 * 0 = Standard 2-buttons gamepad
	 * 1 = CPSF-MD (8 Buttons
	 * 2 = CPSF-SFC (8 Buttons) */
	int JOY_TYPE[2];
} Win68Conf;

extern Win68Conf Config;

void LoadConfig(void);
void SaveConfig(void);
void PropPage_Init(void);

int set_modulepath(char *path, size_t len);

#endif /* _WINX68K_CONFIG_H */
