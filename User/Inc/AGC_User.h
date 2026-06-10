#ifndef _AGC_USER_H_
#define _AGC_USER_H_

#include "stm32h7xx_hal.h"

#define AGC_MIN_Voltage  100
#define AGC_MAX_Voltage  1000
#define AGC_SBS_Voltage  100
#define AGC_VREF         3290

typedef enum {
  AGC_OK = 0,
  AGC_LIMIT_LOW,
  AGC_LIMIT_HIGH
} AGC_Status;

void AGC_Init(void);
AGC_Status AGC_SetVoltageMv(uint16_t voltage_mv);
AGC_Status AGC_Voltage_Add(void);
AGC_Status AGC_Voltage_Sub(void);
uint16_t AGC_GetVoltageMv(void);
uint16_t AGC_GetDacCode(void);
uint16_t AGC_VoltageToDacCode(uint16_t voltage_mv);
void AGC_OutputCode(uint16_t dac_code);

#endif
