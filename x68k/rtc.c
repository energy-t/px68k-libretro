/*
 *  RTC.C - RTC (Real Time Clock / RICOH RP5C15)
 */

#include "common.h"
#include "mfp.h"

#include <time.h>

static uint8_t	RTC_Regs[2][16];
static uint8_t	RTC_Bank = 0;
static int RTC_Timer1    = 0;
static int RTC_Timer16   = 0;

/*
 *   初期化
 */
void RTC_Init(void)
{
	memset(&RTC_Regs[1][0], 0, 16);
	RTC_Regs[0][13] = 0;
	RTC_Regs[0][14] = 0;
	RTC_Regs[0][15] = 0x0c;
}

/*
 *   とけいのりーど
 */
uint8_t FASTCALL RTC_Read(uint32_t adr)
{
	uint8_t ret   = 0;
	time_t t      = time(NULL);
	struct tm *tm = localtime(&t);

	adr          &= 0x1f;
	if (!(adr&1)) return 0;

	if (RTC_Bank == 0)
	{
		switch(adr)
		{
		case 0x01: ret=(tm->tm_sec)%10; break;
		case 0x03: ret=(tm->tm_sec)/10; break;
		case 0x05: ret=(tm->tm_min)%10; break;
		case 0x07: ret=(tm->tm_min)/10; break;
		case 0x09: ret=(tm->tm_hour)%10; break;
		case 0x0b: ret=(tm->tm_hour)/10; break;
		case 0x0d: ret=(uint8_t)(tm->tm_wday); break;
		case 0x0f: ret=(tm->tm_mday)%10; break;
		case 0x11: ret=(tm->tm_mday)/10; break;
		case 0x13: ret=(tm->tm_mon+1)%10; break;
		case 0x15: ret=(tm->tm_mon+1)/10; break;
		case 0x17: ret=((tm->tm_year)-80)%10; break;
		case 0x19: ret=(((tm->tm_year)-80)/10)&0xf; break;
		case 0x1b: ret=RTC_Regs[0][13]; break;
		case 0x1d: ret=RTC_Regs[0][14]; break;
		case 0x1f: ret=RTC_Regs[0][15]; break;
		}
	}
	else
	{
		if (adr == 0x1b)
			ret = (RTC_Regs[1][13]|1);
		else if (adr == 0x17)
			ret = ((tm->tm_year)-80)%4;
		else
			ret = RTC_Regs[1][adr>>1];
	}
	return ret;
}


/*
 *   らいと
 */
void FASTCALL RTC_Write(uint32_t adr, uint8_t data)
{
	if ( adr==0xe8a001 )
          return;
	if ( adr==0xe8a01b )       /* Alarm/Timer Enable制御 */
		RTC_Regs[0][13] = RTC_Regs[1][13] = data & 0x0c;
	else if ( adr==0xe8a01f ) /* Alarm端子出力制御 */
		RTC_Regs[0][15] = RTC_Regs[1][15] = data & 0x0c; 
}

void RTC_Timer(int clock)
{
	RTC_Timer1  += clock;
	RTC_Timer16 += clock;
	if ( RTC_Timer1>=10000000 ) {
		if ( !(RTC_Regs[0][15]&8) ) MFP_Int(15);
		RTC_Timer1 -= 10000000;
	}
	if ( RTC_Timer16>=625000 ) {
		if ( !(RTC_Regs[0][15]&4) ) MFP_Int(15);
		RTC_Timer16 -= 625000;
	}
}
