#ifndef _WINX68K_ADPCM_H
#define _WINX68K_ADPCM_H

#include <stdint.h>

void FASTCALL ADPCM_PreUpdate(uint32_t clock);
void FASTCALL ADPCM_Update(int16_t *buffer, uint32_t length, int rate, uint8_t *pbsp, uint8_t *pbep);

void FASTCALL ADPCM_Write(uint32_t adr, uint8_t data);
uint8_t FASTCALL ADPCM_Read(uint32_t adr);

void ADPCM_SetVolume(uint8_t vol);
void ADPCM_SetPan(int n);
void ADPCM_SetClock(int n);

void ADPCM_Init(uint32_t samplerate);
int ADPCM_IsReady(void);

#endif /* _WINX68K_ADPCM_H */
