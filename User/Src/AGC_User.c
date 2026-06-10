#include "AGC_User.h"

uint16_t AGC_Voltage = AGC_MIN_Voltage;
uint16_t AGC_Digital = 0;

uint16_t AGC_VoltageToDacCode(uint16_t voltage_mv)
{
  uint32_t code = ((uint32_t)voltage_mv * 4095U) / AGC_VREF;

  if (code > 4095U)
  {
    code = 4095U;
  }

  return (uint16_t)code;
}

void AGC_Init(void)
{
  (void)AGC_SetVoltageMv(AGC_MIN_Voltage);
}

AGC_Status AGC_SetVoltageMv(uint16_t voltage_mv)
{
  AGC_Status status = AGC_OK;

  if (voltage_mv < AGC_MIN_Voltage)
  {
    voltage_mv = AGC_MIN_Voltage;
    status = AGC_LIMIT_LOW;
  }
  else if (voltage_mv > AGC_MAX_Voltage)
  {
    voltage_mv = AGC_MAX_Voltage;
    status = AGC_LIMIT_HIGH;
  }

  AGC_Voltage = voltage_mv;
  AGC_Digital = AGC_VoltageToDacCode(voltage_mv);
  AGC_OutputCode(AGC_Digital);

  return status;
}

AGC_Status AGC_Voltage_Add(void)
{
  if (AGC_Voltage >= AGC_MAX_Voltage)
  {
    return AGC_LIMIT_HIGH;
  }

  return AGC_SetVoltageMv((uint16_t)(AGC_Voltage + AGC_SBS_Voltage));
}

AGC_Status AGC_Voltage_Sub(void)
{
  if (AGC_Voltage <= AGC_MIN_Voltage)
  {
    return AGC_LIMIT_LOW;
  }

  return AGC_SetVoltageMv((uint16_t)(AGC_Voltage - AGC_SBS_Voltage));
}

uint16_t AGC_GetVoltageMv(void)
{
  return AGC_Voltage;
}

uint16_t AGC_GetDacCode(void)
{
  return AGC_Digital;
}

__weak void AGC_OutputCode(uint16_t dac_code)
{
  (void)dac_code;
}
