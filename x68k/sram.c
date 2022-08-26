/*
 *  SRAM.C - SRAM (16kb) 領域
 */

#include	"common.h"
#include	"fileio.h"
#include	"prop.h"
#include	"winx68k.h"
#include	"sysport.h"
#include	"x68kmemory.h"
#include	"sram.h"

uint8_t	SRAM[0x4000];

#define SRAMFILE "sram.dat"

/*
 *   役に立たないうぃるすチェック
 */
void SRAM_VirusCheck(void)
{
	if ( (cpu_readmem24_dword(0xed3f60)==0x60000002)
	   &&(cpu_readmem24_dword(0xed0010)==0x00ed3f60) ) /* 特定うぃるすにしか効かないよ~ */
	{
		SRAM_Cleanup();
		SRAM_Init(); /* Virusクリーンアップ後のデータを書き込んでおく */
	}
}


/*
 *   初期化
 */
void SRAM_Init(void)
{
	int i;
	uint8_t tmp;
	FILEH fp;

	for (i=0; i<0x4000; i++)
		SRAM[i] = 0xFF;

	fp = file_open_c(SRAMFILE);
	if (fp)
	{
		File_Read(fp, SRAM, 0x4000);
		File_Close(fp);
		for (i=0; i<0x4000; i+=2)
		{
			tmp = SRAM[i];
			SRAM[i] = SRAM[i+1];
			SRAM[i+1] = tmp;
		}
	}
}


/*
 *  撤収~
 */
void SRAM_Cleanup(void)
{
	int i;
	uint8_t tmp;
	FILEH fp;

	for (i=0; i<0x4000; i+=2)
	{
		tmp = SRAM[i];
		SRAM[i] = SRAM[i+1];
		SRAM[i+1] = tmp;
	}

	fp = file_open_c(SRAMFILE);
	if (!fp)
		fp = file_create_c(SRAMFILE, FTYPE_SRAM);
	if (fp)
	{
		File_Write(fp, SRAM, 0x4000);
		File_Close(fp);
	}
}


/*
 *   りーど
 */
uint8_t FASTCALL SRAM_Read(uint32_t adr)
{
	adr &= 0xffff;
	adr ^= 1;
	if (adr<0x4000)
		return SRAM[adr];
	return 0xff;
}


/*
 *   らいと
 */
void FASTCALL SRAM_Write(uint32_t adr, uint8_t data)
{
	if ( (SysPort[5]==0x31)&&(adr<0xed4000) )
	{
		adr &= 0xffff;
		adr ^= 1;
		SRAM[adr] = data;
	}
}
