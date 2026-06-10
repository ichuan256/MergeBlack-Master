#include "AGC_User.h"

/*
 * 当前 AGC 控制电压，单位 mV。
 * 初始值放在最小值，避免系统上电后默认输出过高控制电压。
 */
uint16_t AGC_Voltage = AGC_MIN_Voltage;

/*
 * 当前 AGC 控制电压换算得到的 DAC 数字量。
 * 这里按常见 12 位 DAC 处理，所以有效范围为 0~4095。
 */
uint16_t AGC_Digital = 0;

/*
 * 功能：把 AGC 控制电压换算成 12 位 DAC 数字量。
 *
 * 换算关系：
 *   DAC 输出电压 = dac_code / 4095 * AGC_VREF
 *   dac_code = voltage_mv * 4095 / AGC_VREF
 *
 * 说明：
 *   1. 使用 uint32_t 做乘法，避免 uint16_t 中间结果溢出。
 *   2. 如果计算结果超过 4095，说明请求电压已经超过 DAC 满量程，
 *      此时强制限制到 4095。
 */
uint16_t AGC_VoltageToDacCode(uint16_t voltage_mv)
{
  uint32_t code = ((uint32_t)voltage_mv * 4095U) / AGC_VREF;

  if (code > 4095U)
  {
    code = 4095U;
  }

  return (uint16_t)code;
}

/*
 * 功能：初始化 AGC 软件控制状态。
 *
 * 当前工程尚未确定 AGC 的真实输出硬件，所以这里不初始化 DAC 外设，
 * 只把 AGC 电压和数字量设置到一个确定的初始值。
 */
void AGC_Init(void)
{
  (void)AGC_SetVoltageMv(AGC_MIN_Voltage);
}

/*
 * 功能：设置 AGC 控制电压。
 *
 * 执行流程：
 *   1. 检查输入电压是否低于最小值。
 *   2. 检查输入电压是否高于最大值。
 *   3. 保存钳位后的电压值。
 *   4. 将电压值换算成 DAC 数字量。
 *   5. 调用 AGC_OutputCode() 输出到实际硬件。
 *
 * 注意：
 *   当前 AGC_OutputCode() 默认是空函数，因此现在不会真正输出电压。
 *   后续接入 DAC 或 PWM 后，只需要重写 AGC_OutputCode()。
 */
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

/*
 * 功能：AGC 控制电压增加一个步进。
 * 如果当前已经到达上限，则不再增加，直接返回 AGC_LIMIT_HIGH。
 */
AGC_Status AGC_Voltage_Add(void)
{
  if (AGC_Voltage >= AGC_MAX_Voltage)
  {
    return AGC_LIMIT_HIGH;
  }

  return AGC_SetVoltageMv((uint16_t)(AGC_Voltage + AGC_SBS_Voltage));
}

/*
 * 功能：AGC 控制电压降低一个步进。
 * 如果当前已经到达下限，则不再降低，直接返回 AGC_LIMIT_LOW。
 */
AGC_Status AGC_Voltage_Sub(void)
{
  if (AGC_Voltage <= AGC_MIN_Voltage)
  {
    return AGC_LIMIT_LOW;
  }

  return AGC_SetVoltageMv((uint16_t)(AGC_Voltage - AGC_SBS_Voltage));
}

/* 返回当前 AGC 控制电压，单位 mV，可用于显示、调试或通讯上报。 */
uint16_t AGC_GetVoltageMv(void)
{
  return AGC_Voltage;
}

/* 返回当前 AGC 控制电压对应的 12 位 DAC 数字量。 */
uint16_t AGC_GetDacCode(void)
{
  return AGC_Digital;
}

/*
 * 默认 AGC 输出函数。
 *
 * __weak 表示弱定义：如果工程其他文件中实现了同名函数，链接器会优先
 * 使用用户自己的函数。这样可以先保留 AGC 软件接口，等硬件确定后再补
 * 实际输出，不影响当前工程编译。
 */
__weak void AGC_OutputCode(uint16_t dac_code)
{
  (void)dac_code;
}