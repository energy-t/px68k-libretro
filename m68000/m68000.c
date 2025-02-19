/******************************************************************************

	m68000.c

	M68000 CPUインタフェース関数

******************************************************************************/

#include "m68000.h"

#if defined (HAVE_CYCLONE)
struct Cyclone m68k;
typedef signed int s32;
#elif defined (HAVE_C68K)
#include "c68k/c68k.h"
#elif defined (HAVE_MUSASHI)
#include "musashi/m68k.h"
#include "musashi/m68kcpu.h"
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */

#include "../x68k/x68kmemory.h"

int m68000_ICountBk;
int ICount;

#if defined (HAVE_CYCLONE)

unsigned int read8(unsigned int a) {
	return (unsigned int) cpu_readmem24(a);
}

unsigned int read16(unsigned int a) {
	return (unsigned int) cpu_readmem24_word(a);
}

unsigned int MyCheckPc(unsigned int pc)
{
  pc-= m68k.membase; /* Get the real program counter */
  if (pc <= 0xbfffff) 			       					{ m68k.membase=(int) MEM; return m68k.membase+pc; }
  if ((pc >= 0xfc0000) && (pc <= 0xffffff))	{ m68k.membase=(int) IPL - 0xfc0000; return m68k.membase+pc; }
  if ((pc >= 0xc00000) && (pc <= 0xc7ffff)) m68k.membase=(int) GVRAM - 0xc00000;
  if ((pc >= 0xe00000) && (pc <= 0xe7ffff))	m68k.membase=(int) TVRAM - 0xe00000;
  if ((pc >= 0xea0000) && (pc <= 0xea1fff))	m68k.membase=(int) SCSIIPL - 0xea0000;
  if ((pc >= 0xed0000) && (pc <= 0xed3fff))	m68k.membase=(int) SRAM - 0xed0000;
  if ((pc >= 0xf00000) && (pc <= 0xfbffff))	m68k.membase=(int) FONT - 0xf00000;
  return m68k.membase+pc; /* New program counter */
}

#elif defined (HAVE_MUSASHI)
uint32_t m68k_read_memory_8(uint32_t address)
{
	return (uint32_t) cpu_readmem24(address);
}

void m68k_write_memory_8(uint32_t address, uint32_t data)
{
	cpu_writemem24(address, data);
}

uint32_t m68k_read_memory_16(uint32_t address)
{
	return (uint32_t) cpu_readmem24_word(address);
}

void m68k_write_memory_16(uint32_t address, uint32_t data)
{
	cpu_writemem24_word(address, data);
}

uint32_t m68k_read_memory_32(uint32_t address)
{
	return (uint32_t) cpu_readmem24_dword(address);
}

void m68k_write_memory_32(uint32_t address, uint32_t data)
{
	cpu_writemem24_dword(address, data);
}

#endif /* HAVE_CYCLONE */ /* HAVE_MUSASHI */


/******************************************************************************
	M68000インタフェース関数
******************************************************************************/

/*--------------------------------------------------------
	CPU初期化
--------------------------------------------------------*/
#if !defined (HAVE_MUSASHI)
s32 my_irqh_callback(s32 level);
#endif

void m68000_init(void)
{
#if defined (HAVE_CYCLONE)

	m68k.read8  = read8;
	m68k.read16 = read16;
	m68k.read32 = cpu_readmem24_dword;

	m68k.fetch8  = read8;
	m68k.fetch16 = read16;
	m68k.fetch32 = cpu_readmem24_dword;

	m68k.write8  = cpu_writemem24;
	m68k.write16 = cpu_writemem24_word;
	m68k.write32 = cpu_writemem24_dword;

	m68k.checkpc = MyCheckPc;

	m68k.IrqCallback = my_irqh_callback;

	CycloneInit();

#elif defined (HAVE_C68K)
    C68k_Init(&C68K, my_irqh_callback);
    C68k_Set_ReadB(&C68K, Memory_ReadB);
    C68k_Set_ReadW(&C68K, Memory_ReadW);
    C68k_Set_WriteB(&C68K, Memory_WriteB);
    C68k_Set_WriteW(&C68K, Memory_WriteW);
	C68k_Set_Fetch(&C68K, 0x000000, 0xbfffff, (pointer)MEM);
    C68k_Set_Fetch(&C68K, 0xc00000, 0xc7ffff, (pointer)GVRAM);
    C68k_Set_Fetch(&C68K, 0xe00000, 0xe7ffff, (pointer)TVRAM);
    C68k_Set_Fetch(&C68K, 0xea0000, 0xea1fff, (pointer)SCSIIPL);
    C68k_Set_Fetch(&C68K, 0xed0000, 0xed3fff, (pointer)SRAM);
    C68k_Set_Fetch(&C68K, 0xf00000, 0xfbffff, (pointer)FONT);
    C68k_Set_Fetch(&C68K, 0xfc0000, 0xffffff, (pointer)IPL);
#elif defined (HAVE_MUSASHI)
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_init();
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}


/*--------------------------------------------------------
	CPUリセット
--------------------------------------------------------*/

void m68000_reset(void)
{
#if defined (HAVE_CYCLONE)
	CycloneReset(&m68k);
	m68k.state_flags = 0; /* Go to default state (not stopped, halted, etc.) */
	m68k.srh = 0x27; /* Set supervisor mode */
#elif defined (HAVE_C68K)
	C68k_Reset(&C68K);
#elif defined (HAVE_MUSASHI)
	m68k_pulse_reset();
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}


/*--------------------------------------------------------
	CPU停止
--------------------------------------------------------*/

void m68000_exit(void)
{
}


/*--------------------------------------------------------
	CPU実行
--------------------------------------------------------*/

int m68000_execute(int cycles)
{	
#if defined (HAVE_CYCLONE)
	m68k.cycles = cycles;
	CycloneRun(&m68k);
	return m68k.cycles ;
#elif defined (HAVE_C68K)
	return C68k_Exec(&C68K, cycles);
#elif defined (HAVE_MUSASHI)
        return m68k_execute(cycles);
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}



/*--------------------------------------------------------
	割り込み処理
--------------------------------------------------------*/

void m68000_set_irq_line(int irqline, int state)
{
#if defined (HAVE_CYCLONE)
	m68k.irq = irqline;
#elif defined (HAVE_C68K)
	C68k_Set_IRQ(&C68K, irqline);
#elif defined (HAVE_MUSASHI)
	m68k_set_irq(irqline);
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}


/*--------------------------------------------------------
	割り込みコールバック関数設定
--------------------------------------------------------*/

void m68000_set_irq_callback(int (*callback)(int line)) { }

/*--------------------------------------------------------
	レジスタ取得
--------------------------------------------------------*/

uint32_t m68000_get_reg(int regnum)
{
#if defined (HAVE_CYCLONE)
	switch (regnum)
	{
		case M68K_PC: return m68k.pc - m68k.membase;
		case M68K_SR: return CycloneGetSr(&m68k);
		case M68K_D0: return m68k.d[0];
		case M68K_D1: return m68k.d[1];
		case M68K_D2: return m68k.d[2];
		case M68K_D3: return m68k.d[3];
		case M68K_D4: return m68k.d[4];
		case M68K_D5: return m68k.d[5];
		case M68K_D6: return m68k.d[6];
		case M68K_D7: return m68k.d[7];
		case M68K_A0: return m68k.a[0];
		case M68K_A1: return m68k.a[1];
		case M68K_A2: return m68k.a[2];
		case M68K_A3: return m68k.a[3];
		case M68K_A4: return m68k.a[4];
		case M68K_A5: return m68k.a[5];
		case M68K_A6: return m68k.a[6];
		case M68K_A7: return m68k.a[7];
		default:
			break;
	}
	return 0x0BADC0DE;
#elif defined (HAVE_C68K)
	switch (regnum)
	{
	case M68K_PC:  return C68k_Get_PC(&C68K);
	case M68K_USP: return C68k_Get_USP(&C68K);
	case M68K_MSP: return C68k_Get_MSP(&C68K);
	case M68K_SR:  return C68k_Get_SR(&C68K);
	case M68K_D0:  return C68k_Get_DReg(&C68K, 0);
	case M68K_D1:  return C68k_Get_DReg(&C68K, 1);
	case M68K_D2:  return C68k_Get_DReg(&C68K, 2);
	case M68K_D3:  return C68k_Get_DReg(&C68K, 3);
	case M68K_D4:  return C68k_Get_DReg(&C68K, 4);
	case M68K_D5:  return C68k_Get_DReg(&C68K, 5);
	case M68K_D6:  return C68k_Get_DReg(&C68K, 6);
	case M68K_D7:  return C68k_Get_DReg(&C68K, 7);
	case M68K_A0:  return C68k_Get_AReg(&C68K, 0);
	case M68K_A1:  return C68k_Get_AReg(&C68K, 1);
	case M68K_A2:  return C68k_Get_AReg(&C68K, 2);
	case M68K_A3:  return C68k_Get_AReg(&C68K, 3);
	case M68K_A4:  return C68k_Get_AReg(&C68K, 4);
	case M68K_A5:  return C68k_Get_AReg(&C68K, 5);
	case M68K_A6:  return C68k_Get_AReg(&C68K, 6);
	case M68K_A7:  return C68k_Get_AReg(&C68K, 7);

	default: return 0;
	}
#elif defined (HAVE_MUSASHI)
	switch (regnum)
	{
	case M68K_PC:  return m68k_get_reg(NULL, M68K_REG_PC);
	case M68K_USP: return m68k_get_reg(NULL, M68K_REG_USP);
	case M68K_MSP: return m68k_get_reg(NULL, M68K_REG_MSP);
	case M68K_SR:  return m68k_get_reg(NULL, M68K_REG_SR);
	case M68K_D0:  return m68k_get_reg(NULL, M68K_REG_D0);
	case M68K_D1:  return m68k_get_reg(NULL, M68K_REG_D1);
	case M68K_D2:  return m68k_get_reg(NULL, M68K_REG_D2);
	case M68K_D3:  return m68k_get_reg(NULL, M68K_REG_D3);
	case M68K_D4:  return m68k_get_reg(NULL, M68K_REG_D4);
	case M68K_D5:  return m68k_get_reg(NULL, M68K_REG_D5);
	case M68K_D6:  return m68k_get_reg(NULL, M68K_REG_D6);
	case M68K_D7:  return m68k_get_reg(NULL, M68K_REG_D7);
	case M68K_A0:  return m68k_get_reg(NULL, M68K_REG_A0);
	case M68K_A1:  return m68k_get_reg(NULL, M68K_REG_A1);
	case M68K_A2:  return m68k_get_reg(NULL, M68K_REG_A2);
	case M68K_A3:  return m68k_get_reg(NULL, M68K_REG_A3);
	case M68K_A4:  return m68k_get_reg(NULL, M68K_REG_A4);
	case M68K_A5:  return m68k_get_reg(NULL, M68K_REG_A5);
	case M68K_A6:  return m68k_get_reg(NULL, M68K_REG_A6);
	case M68K_A7:  return m68k_get_reg(NULL, M68K_REG_A7);

	default: return 0;
	}
#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}


/*--------------------------------------------------------
	レジスタ設定
--------------------------------------------------------*/

void m68000_set_reg(int regnum, uint32_t val)
{
#if defined (HAVE_CYCLONE)
	switch (regnum)
	{
		case M68K_PC:
		  	m68k.pc = m68k.checkpc(val+m68k.membase);
			break;
		case M68K_SR:
		  	CycloneSetSr(&m68k, val);
			break;
		case M68K_D0: m68k.d[0] = val; break;
		case M68K_D1: m68k.d[1] = val; break;
		case M68K_D2: m68k.d[2] = val; break;
		case M68K_D3: m68k.d[3] = val; break;
		case M68K_D4: m68k.d[4] = val; break;
		case M68K_D5: m68k.d[5] = val; break;
		case M68K_D6: m68k.d[6] = val; break;
		case M68K_D7: m68k.d[7] = val; break;
		case M68K_A0: m68k.a[0] = val; break;
		case M68K_A1: m68k.a[1] = val; break;
		case M68K_A2: m68k.a[2] = val; break;
		case M68K_A3: m68k.a[3] = val; break;
		case M68K_A4: m68k.a[4] = val; break;
		case M68K_A5: m68k.a[5] = val; break;
		case M68K_A6: m68k.a[6] = val; break;
		case M68K_A7: m68k.a[7] = val; break;

		
		default: break;
	}	
#elif defined (HAVE_C68k)
	switch (regnum)
	{
	case M68K_PC:  C68k_Set_PC(&C68K, val); break;
	case M68K_USP: C68k_Set_USP(&C68K, val); break;
	case M68K_MSP: C68k_Set_MSP(&C68K, val); break;
	case M68K_SR:  C68k_Set_SR(&C68K, val); break;
	case M68K_D0:  C68k_Set_DReg(&C68K, 0, val); break;
	case M68K_D1:  C68k_Set_DReg(&C68K, 1, val); break;
	case M68K_D2:  C68k_Set_DReg(&C68K, 2, val); break;
	case M68K_D3:  C68k_Set_DReg(&C68K, 3, val); break;
	case M68K_D4:  C68k_Set_DReg(&C68K, 4, val); break;
	case M68K_D5:  C68k_Set_DReg(&C68K, 5, val); break;
	case M68K_D6:  C68k_Set_DReg(&C68K, 6, val); break;
	case M68K_D7:  C68k_Set_DReg(&C68K, 7, val); break;
	case M68K_A0:  C68k_Set_AReg(&C68K, 0, val); break;
	case M68K_A1:  C68k_Set_AReg(&C68K, 1, val); break;
	case M68K_A2:  C68k_Set_AReg(&C68K, 2, val); break;
	case M68K_A3:  C68k_Set_AReg(&C68K, 3, val); break;
	case M68K_A4:  C68k_Set_AReg(&C68K, 4, val); break;
	case M68K_A5:  C68k_Set_AReg(&C68K, 5, val); break;
	case M68K_A6:  C68k_Set_AReg(&C68K, 6, val); break;
	case M68K_A7:  C68k_Set_AReg(&C68K, 7, val); break;
	default: break;
	}
#elif defined (HAVE_MUSASHI)
	switch (regnum)
	{
	case M68K_PC:  m68k_set_reg(M68K_REG_PC, val); break;
	case M68K_USP: m68k_set_reg(M68K_REG_USP, val); break;
	case M68K_MSP: m68k_set_reg(M68K_REG_MSP, val); break;
	case M68K_SR:  m68k_set_reg(M68K_REG_SR, val); break;
	case M68K_D0:  m68k_set_reg(M68K_REG_D0, val); break;
	case M68K_D1:  m68k_set_reg(M68K_REG_D1, val); break;
	case M68K_D2:  m68k_set_reg(M68K_REG_D2, val); break;
	case M68K_D3:  m68k_set_reg(M68K_REG_D3, val); break;
	case M68K_D4:  m68k_set_reg(M68K_REG_D4, val); break;
	case M68K_D5:  m68k_set_reg(M68K_REG_D5, val); break;
	case M68K_D6:  m68k_set_reg(M68K_REG_D6, val); break;
	case M68K_D7:  m68k_set_reg(M68K_REG_D7, val); break;
	case M68K_A0:  m68k_set_reg(M68K_REG_A0, val); break;
	case M68K_A1:  m68k_set_reg(M68K_REG_A1, val); break;
	case M68K_A2:  m68k_set_reg(M68K_REG_A2, val); break;
	case M68K_A3:  m68k_set_reg(M68K_REG_A3, val); break;
	case M68K_A4:  m68k_set_reg(M68K_REG_A4, val); break;
	case M68K_A5:  m68k_set_reg(M68K_REG_A5, val); break;
	case M68K_A6:  m68k_set_reg(M68K_REG_A6, val); break;
	case M68K_A7:  m68k_set_reg(M68K_REG_A7, val); break;
	default: break;
	}

#endif /* HAVE_C68K */ /* HAVE_MUSASHI */
}


/*------------------------------------------------------
	セーブ/ロード ステート
------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( m68000 )
{
#if defined (HAVE_CYCLONE)
/* empty */
#elif defined (HAVE_C68K)
	int i;
	uint32_t pc = C68k_Get_Reg(&C68K, C68K_PC);

	for (i = 0; i < 8; i++)
		state_save_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_save_long(&C68K.A[i], 1);

	state_save_long(&C68K.flag_C, 1);
	state_save_long(&C68K.flag_V, 1);
	state_save_long(&C68K.flag_Z, 1);
	state_save_long(&C68K.flag_N, 1);
	state_save_long(&C68K.flag_X, 1);
	state_save_long(&C68K.flag_I, 1);
	state_save_long(&C68K.flag_S, 1);
	state_save_long(&C68K.USP, 1);
	state_save_long(&pc, 1);
	state_save_long(&C68K.HaltState, 1);
	state_save_long(&C68K.IRQLine, 1);
	state_save_long(&C68K.IRQState, 1);
#endif /* HAVE_C68K */
}

STATE_LOAD( m68000 )
{
#if defined (HAVE_CYCLONE)
/* empty */
#elif defined (HAVE_C68K)
	int i;
	uint32_t pc;

	for (i = 0; i < 8; i++)
		state_load_long(&C68K.D[i], 1);
	for (i = 0; i < 8; i++)
		state_load_long(&C68K.A[i], 1);

	state_load_long(&C68K.flag_C, 1);
	state_load_long(&C68K.flag_V, 1);
	state_load_long(&C68K.flag_Z, 1);
	state_load_long(&C68K.flag_N, 1);
	state_load_long(&C68K.flag_X, 1);
	state_load_long(&C68K.flag_I, 1);
	state_load_long(&C68K.flag_S, 1);
	state_load_long(&C68K.USP, 1);
	state_load_long(&pc, 1);
	state_load_long(&C68K.HaltState, 1);
	state_load_long(&C68K.IRQLine, 1);
	state_load_long(&C68K.IRQState, 1);

	C68k_Set_Reg(&C68K, C68K_PC, pc);
#endif /* HAVE_C68K */
}

#endif /* SAVE_STATE */
