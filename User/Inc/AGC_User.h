#ifndef _AGC_USER_H_
#define _AGC_USER_H_

#include "stm32h7xx_hal.h"

/*
 * AGC 用户模块说明
 * ============================================================
 * 1. 模块来源
 *    本模块参考 D:\CaoTX\电赛\Project\References\32_AGCAD603_V1
 *    例程中的 AGC 控制逻辑迁移而来。
 *
 * 2. 当前状态
 *    原 AGC 例程使用 DAC 输出控制电压，但 MergeBlack 工程里原 DAC
 *    常用引脚 PA5 已经被现有功能占用，所以这里没有强行配置 DAC 引脚，
 *    只先保留“控制电压设置、上下限保护、DAC 数字量换算”这些软件逻辑。
 *
 * 3. 后续接入真实 AGC 硬件的方法
 *    后续如果确定 AGC 控制电压由 DAC、PWM+RC 或外部 DAC 芯片输出，
 *    只需要重新实现 AGC_OutputCode(uint16_t dac_code) 这个函数即可。
 *    AGC_User.c 中的 AGC_OutputCode() 是 __weak 弱函数，用户自己写的
 *    同名函数会自动覆盖默认空实现。
 *
 * 4. 单位约定
 *    - 电压单位统一使用 mV。
 *    - DAC 数字量按 12 位处理，范围为 0~4095。
 */

/* AGC 控制电压下限，单位 mV；低于该值时自动钳位到该值。 */
#define AGC_MIN_Voltage  100

/* AGC 控制电压上限，单位 mV；高于该值时自动钳位到该值。 */
#define AGC_MAX_Voltage  1000

/* AGC 每次增加或减少的步进电压，单位 mV。 */
#define AGC_SBS_Voltage  100

/*
 * DAC 参考电压，单位 mV。
 * DAC 换算公式：dac_code = voltage_mv * 4095 / AGC_VREF。
 */
#define AGC_VREF         3290

/*
 * AGC 接口返回状态。
 * 上层可以根据返回值判断本次设置是否成功，或者是否已经碰到上下限。
 */
typedef enum {
  AGC_OK = 0,       /* 设置成功，输入电压在允许范围内。 */
  AGC_LIMIT_LOW,    /* 输入电压低于下限，已经自动钳位到 AGC_MIN_Voltage。 */
  AGC_LIMIT_HIGH    /* 输入电压高于上限，已经自动钳位到 AGC_MAX_Voltage。 */
} AGC_Status;

/*
 * 初始化 AGC 软件状态。
 * 默认把 AGC 控制电压设置到下限，并计算对应的 DAC 数字量。
 */
void AGC_Init(void);

/*
 * 设置 AGC 控制电压。
 * voltage_mv：期望输出的 AGC 控制电压，单位 mV。
 * 返回值：AGC_OK / AGC_LIMIT_LOW / AGC_LIMIT_HIGH。
 */
AGC_Status AGC_SetVoltageMv(uint16_t voltage_mv);

/* 按 AGC_SBS_Voltage 步进增加 AGC 控制电压。 */
AGC_Status AGC_Voltage_Add(void);

/* 按 AGC_SBS_Voltage 步进降低 AGC 控制电压。 */
AGC_Status AGC_Voltage_Sub(void);

/* 读取当前 AGC 控制电压，单位 mV。 */
uint16_t AGC_GetVoltageMv(void);

/* 读取当前 AGC 控制电压对应的 12 位 DAC 数字量。 */
uint16_t AGC_GetDacCode(void);

/* 将 mV 电压值换算成 12 位 DAC 数字量，结果范围限制为 0~4095。 */
uint16_t AGC_VoltageToDacCode(uint16_t voltage_mv);

/*
 * 实际硬件输出接口。
 * 默认实现为空函数；接入真实 AGC 硬件后，在其他用户文件中重写该函数。
 */
void AGC_OutputCode(uint16_t dac_code);

#endif