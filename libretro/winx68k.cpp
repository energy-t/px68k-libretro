#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "common.h"
#include "fileio.h"
#include "timer.h"
#include "keyboard.h"
#include "prop.h"
#include "status.h"
#include "joystick.h"
#include "winx68k.h"
#include "windraw.h"
#include "winui.h"
#include "../x68k/m68000.h" // xxx perhaps not needed
#include "../m68000/m68000.h"
#include "../x68k/x68kmemory.h"
#include "mfp.h"
#include "opm.h"
#include "bg.h"
#include "adpcm.h"
#include "mercury.h"
#include "crtc.h"
#include "mfp.h"
#include "fdc.h"
#include "fdd.h"
#include "dmac.h"
#include "irqh.h"
#include "ioc.h"
#include "rtc.h"
#include "sasi.h"
#include "scsi.h"
#include "sysport.h"
#include "bg.h"
#include "palette.h"
#include "crtc.h"
#include "pia.h"
#include "scc.h"
#include "midi.h"
#include "sram.h"
#include "gvram.h"
#include "tvram.h"
#include "mouse.h"

#include "dswin.h"
#include "fmg_wrap.h"

#ifdef RFMDRV
int rfd_sock;
#endif

char	winx68k_dir[MAX_PATH];
char	winx68k_ini[MAX_PATH];

uint16_t	VLINE_TOTAL = 567;
uint32_t	VLINE = 0;
uint32_t	vline = 0;

uint8_t DispFrame = 0;
static uint32_t SoundSampleRate;

static int ClkUsed = 0;
static int FrameSkipCount = 0;
static int FrameSkipQueue = 0;

static uint32_t old_ram_size = 0;
static int old_clkdiv = 0;

#ifdef __cplusplus
};
#endif

void WinX68k_SCSICheck(void)
{
	static const uint8_t SCSIIMG[] = {
		0x00, 0xfc, 0x00, 0x14,				// $fc0000 SCSI boot entry address
		0x00, 0xfc, 0x00, 0x16,				// $fc0004 IOCS vector setting entry address (always before "Human" 8 bytes)
		0x00, 0x00, 0x00, 0x00,				// $fc0008 ?
		0x48, 0x75, 0x6d, 0x61,				// $fc000c ↓
		0x6e, 0x36, 0x38, 0x6b,				// $fc0010 ID "Human68k"	(always just before start-up entry point)
		0x4e, 0x75,							// $fc0014 "rts"		(start-up entry point)
		0x23, 0xfc, 0x00, 0xfc, 0x00, 0x2a,	// $fc0016 ↓		(IOCS vector setting entry point)
		0x00, 0x00, 0x07, 0xd4,				// $fc001c "move.l #$fc002a, $7d4.l"
		0x74, 0xff,							// $fc0020 "moveq #-1, d2"
		0x4e, 0x75,							// $fc0022 "rts"
//		0x53, 0x43, 0x53, 0x49, 0x49, 0x4e,	// $fc0024 ID "SCSIIN"
// If internal SCSI is ON, it seems SASI is automatically switched OFF...
// Therefore, let's avoid ID conflict...
		0x44, 0x55, 0x4d, 0x4d, 0x59, 0x20,	// $fc0024 ID "DUMMY "
		0x70, 0xff,							// $fc002a "moveq #-1, d0"	(SCSI IOCS call entry point)
		0x4e, 0x75,							// $fc002c "rts"
	};

	uint16_t *p1, *p2;
	int scsi;
	int i;

	scsi = 0;
	for (i = 0x30600; i < 0x30c00; i += 2) {
		p1 = (uint16_t *)(&IPL[i]);
		p2 = p1 + 1;
		// xxx: works only for little endian guys
		if (*p1 == 0xfc00 && *p2 == 0x0000) {
			scsi = 1;
			break;
		}
	}

	// SCSI model time
	if (scsi) {
		memset(IPL, 0, 0x2000);				// main is 8kb
		memset(&IPL[0x2000], 0xff, 0x1e000);	// remaining is 0xff
		memcpy(IPL, SCSIIMG, sizeof(SCSIIMG));	// fake­SCSI BIOS
	} else {
		// SASI model sees the IPL as it is
		memcpy(IPL, &IPL[0x20000], 0x20000);
	}
}

int
WinX68k_LoadROMs(void)
{
	static const char *BIOSFILE[] = {
		"iplrom.dat", "iplrom30.dat", "iplromco.dat", "iplromxv.dat"
	};
	static const char FONTFILE[] = "cgrom.dat";
	static const char FONTFILETMP[] = "cgrom.tmp";
	FILEH fp;
	int i;
	uint8_t tmp;

	for (fp = 0, i = 0; fp == 0 && i < NELEMENTS(BIOSFILE); ++i)
		fp = file_open_c((char *)BIOSFILE[i]);

	if (fp == 0) {
		Error("BIOS ROM image can't be found.");
		return FALSE;
	}

	File_Read(fp, &IPL[0x20000], 0x20000);
	File_Close(fp);

	WinX68k_SCSICheck();	// if SCSI IPL, SCSI BIOS is established around $fc0000

	for (i = 0; i < 0x40000; i += 2) {
		tmp = IPL[i];
		IPL[i] = IPL[i + 1];
		IPL[i + 1] = tmp;
	}

	fp = file_open_c((char *)FONTFILE);
	if (fp == 0) {
		// cgrom.tmp present?
		fp = file_open_c((char *)FONTFILETMP);
		// font creation XXX - Font ROM image can't be found
		if (fp == 0)
			return FALSE;
	}
	File_Read(fp, FONT, 0xc0000);
	File_Close(fp);

	return TRUE;
}

int WinX68k_Reset(void)
{
	OPM_Reset();

#if defined (HAVE_CYCLONE)
	m68000_reset();
	m68000_set_reg(M68K_A7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	m68000_set_reg(M68K_PC, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
#elif defined (HAVE_C68K)
	C68k_Reset(&C68K);
/*
	C68k_Set_Reg(&C68K, C68K_A7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	C68k_Set_Reg(&C68K, C68K_PC, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
*/
	C68k_Set_AReg(&C68K, 7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	C68k_Set_PC(&C68K, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
#elif defined (HAVE_MUSASHI)
	m68k_pulse_reset();

	m68k_set_reg(M68K_REG_A7, (IPL[0x30001]<<24)|(IPL[0x30000]<<16)|(IPL[0x30003]<<8)|IPL[0x30002]);
	m68k_set_reg(M68K_REG_PC, (IPL[0x30005]<<24)|(IPL[0x30004]<<16)|(IPL[0x30007]<<8)|IPL[0x30006]);
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */

	Memory_Init();
	CRTC_Init();
	DMA_Init();
	MFP_Init();
	FDC_Init();
	FDD_Reset();
	SASI_Init();
	SCSI_Init();
	IOC_Init();
	SCC_Init();
	PIA_Init();
	RTC_Init();
	TVRAM_Init();
	GVRAM_Init();
	BG_Init();
	Pal_Init();
	IRQH_Init();
	MIDI_Init();
	//WinDrv_Init();

	m68000_ICountBk = 0;
	ICount = 0;

	DSound_Stop();
	SRAM_VirusCheck();
	//CDROM_Init();
	DSound_Play();
	return TRUE;
}

int WinX68k_Init(void)
{

#define MEM_SIZE 0xc00000

	IPL = (uint8_t*)malloc(0x40000);
	MEM = (uint8_t*)malloc(MEM_SIZE);
	FONT = (uint8_t*)malloc(0xc0000);

	if (MEM)
		memset(MEM, 0, MEM_SIZE);

	if (MEM && FONT && IPL)
	{
	  	m68000_init();
		return TRUE;
	}
	return FALSE;
}

void
WinX68k_Cleanup(void)
{

	if (IPL) {
		free(IPL);
		IPL = 0;
	}
	if (MEM) {
		free(MEM);
		MEM = 0;
	}
	if (FONT) {
		free(FONT);
		FONT = 0;
	}
}

#define CLOCK_SLICE 200
// -----------------------------------------------------------------------------------
//  Core Main Loop
// -----------------------------------------------------------------------------------
void WinX68k_Exec(void)
{
	//char *test = NULL;
	int clk_total, clkdiv, usedclk, hsync, clk_next, clk_count, clk_line=0;
	int KeyIntCnt = 0, MouseIntCnt = 0;
	uint32_t t_start = timeGetTime(), t_end;

	if(!(Memory_ReadD(0xed0008)==Config.ram_size)){
		Memory_WriteB(0xe8e00d, 0x31);             // SRAM write permission
		Memory_WriteD(0xed0008, Config.ram_size);         // Define RAM amount
	}

	Joystick_Update(FALSE, -1, 0);
	Joystick_Update(FALSE, -1, 1);

	if ( Config.FrameRate != 7 ) {
		DispFrame = (DispFrame+1)%Config.FrameRate;
	} else {				// Auto Frame Skip
		if ( FrameSkipQueue ) {
			if ( FrameSkipCount>15 ) {
				FrameSkipCount = 0;
				FrameSkipQueue++;
				DispFrame = 0;
			} else {
				FrameSkipCount++;
				FrameSkipQueue--;
				DispFrame = 1;
			}
		} else {
			FrameSkipCount = 0;
			DispFrame = 0;
		}
	}

	vline = 0;
	clk_count = -ICount;
	clk_total = (CRTC_Regs[0x29] & 0x10) ? VSYNC_HIGH : VSYNC_NORM;

	clk_total = (clk_total*Config.clockmhz)/10;
	clkdiv = Config.clockmhz;

#if 0
	if (Config.XVIMode == 1) {
		clk_total = (clk_total*16)/10;
		clkdiv = 16;
	} else if (Config.XVIMode == 2) {
		clk_total = (clk_total*24)/10;
		clkdiv = 24;
	} else {
		clkdiv = 10;
	}
#endif

	if(clkdiv != old_clkdiv || Config.ram_size != old_ram_size){
		old_clkdiv = clkdiv;
		old_ram_size = Config.ram_size;
	}

	ICount += clk_total;
	clk_next = (clk_total/VLINE_TOTAL);
	hsync = 1;

	do {
		int m, n = (ICount>CLOCK_SLICE)?CLOCK_SLICE:ICount;

		if ( hsync ) {
			hsync = 0;
			clk_line = 0;
			MFP_Int(0);
			if ( (vline>=CRTC_VSTART)&&(vline<CRTC_VEND) )
				VLINE = ((vline-CRTC_VSTART)*CRTC_VStep)/2;
			else
				VLINE = (uint32_t)-1;
			if ( (!(MFP[MFP_AER]&0x40))&&(vline==CRTC_IntLine) )
				MFP_Int(1);
			if ( MFP[MFP_AER]&0x10 ) {
				if ( vline==CRTC_VSTART )
					MFP_Int(9);
			} else {
				if ( CRTC_VEND>=VLINE_TOTAL ) {
					if ( (long)vline==(CRTC_VEND-VLINE_TOTAL) ) MFP_Int(9);		// Is it Exciting Hour? （TOTAL<VEND）
				} else {
					if ( (long)vline==(VLINE_TOTAL-1) ) MFP_Int(9);				// Is it Crazy Climber?
				}
			}
		}

		{
#if defined (HAVE_CYCLONE)
			m68000_execute(n);
#elif defined (HAVE_C68K)
			C68k_Exec(&C68K, n);
#elif defined (HAVE_MUSASHI)
			m68k_execute(n);
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
			m = (n-m68000_ICountBk);
			ClkUsed += m*10;
			usedclk = ClkUsed/clkdiv;
			clk_line += usedclk;
			ClkUsed -= usedclk*clkdiv;
			ICount -= m;
			clk_count += m;
		}

		MFP_Timer(usedclk);
		RTC_Timer(usedclk);
		DMA_Exec(0);
		DMA_Exec(1);
		DMA_Exec(2);

		if ( clk_count>=clk_next ) {
			//OPM_RomeoOut(Config.BufferSize*5);
			//MIDI_DelayOut((Config.MIDIAutoDelay)?(Config.BufferSize*5):Config.MIDIDelay);
			MFP_TimerA();
			if ( (MFP[MFP_AER]&0x40)&&(vline==CRTC_IntLine) )
				MFP_Int(1);
			if ( (!DispFrame)&&(vline>=CRTC_VSTART)&&(vline<CRTC_VEND) ) {
				if ( CRTC_VStep==1 ) {				// HighReso 256dot (read twice)
					if ( vline%2 )
						WinDraw_DrawLine();
				} else if ( CRTC_VStep==4 ) {		// LowReso 512dot
					WinDraw_DrawLine();				// draw twice per scanline (interlace)
					VLINE++;
					WinDraw_DrawLine();
				} else {							// High 512dot / Low 256dot
					WinDraw_DrawLine();
				}
			}

			ADPCM_PreUpdate(clk_line);
			OPM_Timer(clk_line);
			MIDI_Timer(clk_line);
#ifndef	NO_MERCURY
			Mcry_PreUpdate(clk_line);
#endif

			KeyIntCnt++;
			if ( KeyIntCnt>(VLINE_TOTAL/4) ) {
				KeyIntCnt = 0;
				Keyboard_Int();
			}
			MouseIntCnt++;
			if ( MouseIntCnt>(VLINE_TOTAL/8) ) {
				MouseIntCnt = 0;
				SCC_IntCheck();
			}
			DSound_Send0(clk_line);

			vline++;
			clk_next  = (clk_total*(vline+1))/VLINE_TOTAL;
			hsync = 1;
		}
	} while ( vline<VLINE_TOTAL );

	if ( CRTC_Mode&2 ) {		// FastClr byte adjustment (PITAPAT)
		if ( CRTC_FastClr ) {	// if FastClr=1 and CRTC_Mode&2 then end
			CRTC_FastClr--;
			if ( !CRTC_FastClr )
				CRTC_Mode &= 0xfd;
		} else {				// FastClr start
			if ( CRTC_Regs[0x29]&0x10 )
				CRTC_FastClr = 1;
			else
				CRTC_FastClr = 2;
			TVRAM_SetAllDirty();
			GVRAM_FastClear();
		}
	}

	FDD_SetFDInt();
	if ( !DispFrame )
		WinDraw_Draw();

	t_end = timeGetTime();
	if ( (int)(t_end-t_start)>((CRTC_Regs[0x29]&0x10)?14:16) ) {
		FrameSkipQueue += ((t_end-t_start)/((CRTC_Regs[0x29]&0x10)?14:16))+1;
		if ( FrameSkipQueue>100 )
			FrameSkipQueue = 100;
	}
}


#ifdef  __cplusplus
extern "C" {
#endif

#include "libretro.h"

extern retro_input_state_t input_state_cb;
extern char Core_Key_State[512];
extern char Core_old_Key_State[512];

int mb1=0,mb2=0;
extern int retrow,retroh,CHANGEAV;
static int oldrw=0,oldrh=0;
int end_loop=0;

enum {menu_out, menu_enter, menu_in};
int menu_mode = menu_out;
#ifdef  __cplusplus
};
#endif
extern "C" int pmain(int argc, char *argv[])
{
#ifdef RFMDRV
	struct sockaddr_in dest;

	memset(&dest, 0, sizeof(dest));
	dest.sin_port = htons(2151);
	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr("127.0.0.1");

	rfd_sock = socket(AF_INET, SOCK_STREAM, 0);
	connect (rfd_sock, (struct sockaddr *)&dest, sizeof(dest));
#endif

	if (set_modulepath(winx68k_dir, sizeof(winx68k_dir)))
		return 1;

	file_setcd(winx68k_dir);

	LoadConfig();

	if (!WinDraw_MenuInit()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	SoundSampleRate = Config.SampleRate;

	StatBar_Show(Config.WindowFDDStat);
	WinDraw_ChangeSize();

	WinUI_Init();
	WinDraw_StartupScreen();

	if (!WinX68k_Init()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		return 1;
	}

	if (!WinX68k_LoadROMs()) {
		WinX68k_Cleanup();
		WinDraw_Cleanup();
		exit (1);
	}

	Keyboard_Init(); //before moving to WinDraw_Init()

	if (!WinDraw_Init()) {
		WinDraw_Cleanup();
		Error("Error: Can't init screen.\n");
		return 1;
	}

	if ( SoundSampleRate ) {
		ADPCM_Init(SoundSampleRate);
		OPM_Init(4000000/*3579545*/, SoundSampleRate);
#ifndef	NO_MERCURY
		Mcry_Init(SoundSampleRate, winx68k_dir);
#endif
	} else {
		ADPCM_Init(100);
		OPM_Init(4000000/*3579545*/, 100);
#ifndef	NO_MERCURY
		Mcry_Init(100, winx68k_dir);
#endif
	}

	FDD_Init();
	SysPort_Init();
	Mouse_Init();
	Joystick_Init();
	SRAM_Init();
	WinX68k_Reset();
	Timer_Init();

	MIDI_Init();
	MIDI_SetMimpiMap(Config.ToneMapFile);	// ToneMap file usage
	MIDI_EnableMimpiDef(Config.ToneMap);

	if (!DSound_Init(Config.SampleRate, Config.BufferSize)) { }

	ADPCM_SetVolume((uint8_t)Config.PCM_VOL);
	OPM_SetVolume((uint8_t)Config.OPM_VOL);
#ifndef	NO_MERCURY
	Mcry_SetVolume((uint8_t)Config.MCR_VOL);
#endif
	DSound_Play();

	// apply defined command line settings
	if(argc==3 && argv[1][0]=='-' && argv[1][1]=='h')
		strcpy(Config.HDImage[0], argv[2]);
	else {
		switch (argc) {
		case 3:
			strcpy(Config.FDDImage[1], argv[2]);
		case 2:
			strcpy(Config.FDDImage[0], argv[1]);
			break;
		case 0:
			// start menu when running without content
			menu_mode = menu_enter;
		}
	}

	FDD_SetFD(0, Config.FDDImage[0], 0);
	FDD_SetFD(1, Config.FDDImage[1], 0);

	return 1;

}

#define KEYP(a,b) {\
	if(Core_Key_State[a] && Core_Key_State[a]!=Core_old_Key_State[a]  )\
		send_keycode(b, 2);\
	else if ( !Core_Key_State[a] && Core_Key_State[a]!=Core_old_Key_State[a]  )\
		send_keycode(b, 1);\
}

extern "C" void handle_retrok(void)
{
	if(Core_Key_State[RETROK_F12] && Core_Key_State[RETROK_F12]!=Core_old_Key_State[RETROK_F12]  )
	{
		if (menu_mode == menu_out) {
			oldrw=retrow;oldrh=retroh;
			retroh=600;retrow=800;
			CHANGEAV=1;
			menu_mode = menu_enter;
			DSound_Stop();
		} else {
			CHANGEAV=1;
			retrow=oldrw;retroh=oldrh;
			DSound_Play();
			menu_mode = menu_out;
		}
	}

	KEYP(RETROK_ESCAPE,0x1);
        int i;
	for(i=1;i<10;i++)
		KEYP(RETROK_0+i,0x1+i);
	KEYP(RETROK_0,0xb);
	KEYP(RETROK_MINUS,0xc);
	KEYP(RETROK_QUOTE,0x28); // colon :
	KEYP(RETROK_BACKSPACE,0xf);

	KEYP(RETROK_TAB,0x10);
	KEYP(RETROK_q,0x11);
	KEYP(RETROK_w,0x12);
	KEYP(RETROK_e,0x13);
	KEYP(RETROK_r,0x14);
	KEYP(RETROK_t,0x15);
	KEYP(RETROK_y,0x16);
	KEYP(RETROK_u,0x17);
	KEYP(RETROK_i,0x18);
	KEYP(RETROK_o,0x19);
	KEYP(RETROK_p,0x1A);
	KEYP(RETROK_BACKQUOTE,0x1B);
	KEYP(RETROK_LEFTBRACKET,0x1C);
	KEYP(RETROK_RETURN,0x1d);
	KEYP(RETROK_EQUALS,0xd); // caret ^

	KEYP(RETROK_a,0x1e);
	KEYP(RETROK_s,0x1f);
	KEYP(RETROK_d,0x20);
	KEYP(RETROK_f,0x21);
	KEYP(RETROK_g,0x22);
	KEYP(RETROK_h,0x23);
	KEYP(RETROK_j,0x24);
	KEYP(RETROK_k,0x25);
	KEYP(RETROK_l,0x26);
	KEYP(RETROK_SEMICOLON,0x27);
	KEYP(RETROK_BACKSLASH,0xe); // Yen symbol ¥
	KEYP(RETROK_RIGHTBRACKET,0x29);

	KEYP(RETROK_z,0x2a);
	KEYP(RETROK_x,0x2b);
	KEYP(RETROK_c,0x2c);
	KEYP(RETROK_v,0x2d);
	KEYP(RETROK_b,0x2e);
	KEYP(RETROK_n,0x2f);
	KEYP(RETROK_m,0x30);
	KEYP(RETROK_COMMA,0x31);
	KEYP(RETROK_PERIOD,0x32);
	KEYP(RETROK_SLASH,0x33);
	KEYP(RETROK_0,0x34); // underquote _ as shift+0 which was empty, Japanese chars can't overlap as we're not using them

	KEYP(RETROK_SPACE,0x35);
	KEYP(RETROK_HOME,0x36);
	KEYP(RETROK_DELETE,0x37);
	KEYP(RETROK_PAGEDOWN,0x38);
	KEYP(RETROK_PAGEUP,0x39);
	KEYP(RETROK_END,0x3a);
	KEYP(RETROK_LEFT,0x3b);
	KEYP(RETROK_UP,0x3c);
	KEYP(RETROK_RIGHT,0x3d);
	KEYP(RETROK_DOWN,0x3e);

	KEYP(RETROK_CLEAR,0x3f);
	KEYP(RETROK_KP_DIVIDE,0x40);
	KEYP(RETROK_KP_MULTIPLY,0x41);
	KEYP(RETROK_KP_MINUS,0x42);
	KEYP(RETROK_KP7,0x43);
	KEYP(RETROK_KP8,0x44);
	KEYP(RETROK_KP9,0x45);
	KEYP(RETROK_KP_PLUS,0x46);
	KEYP(RETROK_KP4,0x47);
	KEYP(RETROK_KP5,0x48);
	KEYP(RETROK_KP6,0x49);
	KEYP(RETROK_KP_EQUALS,0x4a);
	KEYP(RETROK_KP1,0x4b);
	KEYP(RETROK_KP2,0x4c);
	KEYP(RETROK_KP3,0x4d);
	KEYP(RETROK_KP_ENTER,0x4e);
	KEYP(RETROK_KP0,0x4f);
	//KEYP(RETROK_COMMA,0x50);
	KEYP(RETROK_KP_PERIOD,0x51);

	KEYP(RETROK_PRINT,0x52); //symbol input (kigou)
	KEYP(RETROK_SCROLLOCK,0x53); //registration (touroku)
	KEYP(RETROK_F11,0x54); //help

	// only process kb_to_joypad map when its not zero, else button is used as joypad select mode
	if (Config.joy1_select_mapping)
		KEYP(RETROK_XFX, Config.joy1_select_mapping);

	KEYP(RETROK_CAPSLOCK,0x5d);
	KEYP(RETROK_INSERT,0x5e);
	KEYP(RETROK_BREAK,0x61); //break
	KEYP(RETROK_PAUSE,0x61); //break (allow shift+break combo)

	for(i=0;i<10;i++)
		KEYP(RETROK_F1+i,0x63+i);

	KEYP(RETROK_LSHIFT,0x70);
	KEYP(RETROK_RSHIFT,0x70);
	KEYP(RETROK_LCTRL,0x71);
	KEYP(RETROK_RCTRL,0x71);
	KEYP(RETROK_LSUPER,0x72);
	KEYP(RETROK_RSUPER,0x73);
	KEYP(RETROK_LALT,0x72);
	KEYP(RETROK_RALT,0x73);

}

extern "C" void exec_app_retro(void
){
	int menu_key_down;
	//while (1) {
		// OPM_RomeoOut(Config.BufferSize * 5);
		if (menu_mode == menu_out
		    && (Config.AudioDesyncHack ||
                        Config.NoWaitMode || Timer_GetCount()))
			WinX68k_Exec();

		menu_key_down = -1;
 		//end_loop=1;

		static int mbL = 0, mbR = 0;

	      	int mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
		int mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

     		Mouse_Event(0, mouse_x, mouse_y);

		int mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
		int mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

  	        if(mbL==0 && mouse_l){
      			mbL=1;
			Mouse_Event(1,1.0,0);
		}
   		else if(mbL==1 && !mouse_l)
   		{
   			mbL=0;
			Mouse_Event(1,0,0);
		}
  	        if(mbR==0 && mouse_r){
      			mbR=1;
			Mouse_Event(2,1.0,0);
		}
   		else if(mbR==1 && !mouse_r)
   		{
   			mbR=0;
			Mouse_Event(2,0,0);
		}

  		int i;

   		for(i=0;i<320;i++)
      			Core_Key_State[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i) ? 0x80: 0;

      	Core_Key_State[RETROK_XFX] = 0;

   		if (input_state_cb(0, RETRO_DEVICE_JOYPAD,0, RETRO_DEVICE_ID_JOYPAD_L2))	//Joypad Key for Menu
				Core_Key_State[RETROK_F12] = 0x80;

		if (Config.joy1_select_mapping)
			if (input_state_cb(0, RETRO_DEVICE_JOYPAD,0, RETRO_DEVICE_ID_JOYPAD_SELECT))	//Joypad Key for Mapping
				Core_Key_State[RETROK_XFX] = 0x80;

		if(memcmp( Core_Key_State,Core_old_Key_State , sizeof(Core_Key_State) ) )
			handle_retrok();

   		memcpy(Core_old_Key_State,Core_Key_State , sizeof(Core_Key_State) );

		if (menu_mode != menu_out) {
			int ret;

			keyb_in = 0;
			if (Core_Key_State[RETROK_RIGHT] || Core_Key_State[RETROK_PAGEDOWN])
				keyb_in |= JOY_RIGHT;
			if (Core_Key_State[RETROK_LEFT] || Core_Key_State[RETROK_PAGEUP])
				keyb_in |= JOY_LEFT;
			if (Core_Key_State[RETROK_UP])
				keyb_in |= JOY_UP;
			if (Core_Key_State[RETROK_DOWN])
				keyb_in |= JOY_DOWN;
			if (Core_Key_State[RETROK_z] || Core_Key_State[RETROK_RETURN])
				keyb_in |= JOY_TRG1;
			if (Core_Key_State[RETROK_x] || Core_Key_State[RETROK_BACKSPACE])
				keyb_in |= JOY_TRG2;

			Joystick_Update(TRUE, menu_key_down, 0);

			ret = WinUI_Menu(menu_mode == menu_enter);
			menu_mode = menu_in;
			if (ret == WUM_MENU_END) {
				DSound_Play();
				menu_mode = menu_out;
			} else if (ret == WUM_EMU_QUIT) {
				 end_loop=1;
			}
		}

	//}
}

extern "C" void end_loop_retro(void)
{
   //end_loop:
   Memory_WriteB(0xe8e00d, 0x31);                     // SRAM write permission
   Memory_WriteD(0xed0040, Memory_ReadD(0xed0040)+1); // Estimated operation time(min.)
   Memory_WriteD(0xed0044, Memory_ReadD(0xed0044)+1); // Estimated booting times

   OPM_Cleanup();
#ifndef	NO_MERCURY
   Mcry_Cleanup();
#endif

   Joystick_Cleanup();
   SRAM_Cleanup();
   FDD_Cleanup();
   //CDROM_Cleanup();
   MIDI_Cleanup();
   DSound_Cleanup();
   WinX68k_Cleanup();
   WinDraw_Cleanup();
   WinDraw_CleanupScreen();

   SaveConfig();
}
