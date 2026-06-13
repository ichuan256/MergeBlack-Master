#ifndef _SPECTRUM_SYSTEM_USER_H_
#define _SPECTRUM_SYSTEM_USER_H_

#include "stm32h7xx_hal.h"

/*
 * MergeBlack spectrum-system host module.
 *
 * Role:
 *   MergeBlack is the measurement/control host. It drives the ADF4351 LO,
 *   samples the current amplitude placeholder, analyzes peak/spur data, and
 *   sends compact display packets to MergeBlue over BoardComm UART.
 *
 * Current small-system scope:
 *   RF input 80.0~100.0 MHz is represented by a 201-point sweep.
 *   LO frequency is RF + 10.0 MHz, so ADF4351 sweeps 90.0~110.0 MHz.
 *   The analog front-end can be connected later; for now the sampling hook
 *   reads the latest converted AD9226 voltage if available.
 */

#define SPECTRUM_RF_START_KHZ       80000UL
#define SPECTRUM_RF_STOP_KHZ        100000UL
#define SPECTRUM_IF_KHZ             10000UL
#define SPECTRUM_STEP_KHZ           100UL
#define SPECTRUM_POINT_COUNT        201U
#define SPECTRUM_SWEEP_TIME_MIN_MS  1000UL
#define SPECTRUM_SWEEP_TIME_MAX_MS  5000UL

/* BoardComm command words shared by MergeBlack and MergeBlue. */
#define BOARD_COMM_CMD_SYS_STATUS      0x20U
#define BOARD_COMM_CMD_SWEEP_POINT     0x30U
#define BOARD_COMM_CMD_SWEEP_RESULT    0x32U

typedef enum {
  SPECTRUM_MODE_LO_MANUAL = 0,
  SPECTRUM_MODE_SWEEP = 1
} SpectrumMode;

typedef enum {
  SPECTRUM_HOST_IDLE = 0,
  SPECTRUM_HOST_SWEEPING = 1,
  SPECTRUM_HOST_DONE = 2
} SpectrumHostState;

typedef struct {
  SpectrumMode mode;
  SpectrumHostState state;
  uint32_t rf_khz;
  uint32_t lo_khz;
  uint16_t point_index;
  uint16_t amplitude_mv;
  uint16_t peak_index;
  uint16_t peak_amplitude_mv;
  uint8_t spur_count;
  uint8_t pll_locked;
} SpectrumHostSnapshot;

void SpectrumSystem_Init(void);
void SpectrumSystem_Task(void);
void SpectrumSystem_OnKey(char key);
SpectrumHostSnapshot SpectrumSystem_GetSnapshot(void);

#endif
