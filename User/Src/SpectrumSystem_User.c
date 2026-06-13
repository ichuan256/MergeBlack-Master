#include "SpectrumSystem_User.h"

#include "AD9226_User.h"
#include "ADF4351_User.h"
#include "BoardComm_User.h"

extern double adc_volt;

static SpectrumHostSnapshot spectrum_snapshot;
static uint16_t spectrum_values_mv[SPECTRUM_POINT_COUNT];
static uint32_t spectrum_sweep_time_ms = 3000UL;
static uint32_t spectrum_last_step_tick = 0;
static uint8_t spectrum_sweep_started = 0;

static void Spectrum_WriteU16(uint8_t *buf, uint8_t *pos, uint16_t value)
{
  buf[(*pos)++] = (uint8_t)(value & 0xFFU);
  buf[(*pos)++] = (uint8_t)((value >> 8) & 0xFFU);
}

static void Spectrum_WriteU32(uint8_t *buf, uint8_t *pos, uint32_t value)
{
  buf[(*pos)++] = (uint8_t)(value & 0xFFUL);
  buf[(*pos)++] = (uint8_t)((value >> 8) & 0xFFUL);
  buf[(*pos)++] = (uint8_t)((value >> 16) & 0xFFUL);
  buf[(*pos)++] = (uint8_t)((value >> 24) & 0xFFUL);
}

static uint32_t Spectrum_IndexToRfKHz(uint16_t index)
{
  return SPECTRUM_RF_START_KHZ + ((uint32_t)index * SPECTRUM_STEP_KHZ);
}

static uint32_t Spectrum_RfToLoKHz(uint32_t rf_khz)
{
  return rf_khz + SPECTRUM_IF_KHZ;
}

static uint16_t Spectrum_ReadAmplitudeMv(void)
{
  double mv = adc_volt * 1000.0;

  if (mv < 0.0)
  {
    mv = 0.0;
  }

  if (mv > 5000.0)
  {
    mv = 5000.0;
  }

  return (uint16_t)mv;
}

static void Spectrum_SendStatus(void)
{
  uint8_t payload[22];
  uint8_t pos = 0;

  payload[pos++] = (uint8_t)spectrum_snapshot.mode;
  payload[pos++] = (uint8_t)spectrum_snapshot.state;
  Spectrum_WriteU32(payload, &pos, spectrum_snapshot.rf_khz);
  Spectrum_WriteU32(payload, &pos, spectrum_snapshot.lo_khz);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.point_index);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.amplitude_mv);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.peak_index);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.peak_amplitude_mv);
  payload[pos++] = spectrum_snapshot.spur_count;
  payload[pos++] = spectrum_snapshot.pll_locked;

  (void)BoardComm_Send(BOARD_COMM_CMD_SYS_STATUS, payload, pos);
}

static void Spectrum_SendPoint(void)
{
  uint8_t payload[9];
  uint8_t pos = 0;

  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.point_index);
  Spectrum_WriteU32(payload, &pos, spectrum_snapshot.rf_khz);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.amplitude_mv);
  payload[pos++] = spectrum_snapshot.pll_locked;

  (void)BoardComm_Send(BOARD_COMM_CMD_SWEEP_POINT, payload, pos);
}

static void Spectrum_SendResult(void)
{
  uint8_t payload[5];
  uint8_t pos = 0;

  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.peak_index);
  Spectrum_WriteU16(payload, &pos, spectrum_snapshot.peak_amplitude_mv);
  payload[pos++] = spectrum_snapshot.spur_count;

  (void)BoardComm_Send(BOARD_COMM_CMD_SWEEP_RESULT, payload, pos);
}

static void Spectrum_UpdatePeakAndSpur(void)
{
  uint16_t peak = 0;
  uint16_t peak_value = 0;
  uint8_t spur_count = 0;
  uint16_t threshold;

  for (uint16_t i = 0; i < SPECTRUM_POINT_COUNT; i++)
  {
    if (spectrum_values_mv[i] > peak_value)
    {
      peak_value = spectrum_values_mv[i];
      peak = i;
    }
  }

  threshold = (uint16_t)((uint32_t)peak_value * 2UL / 100UL);
  for (uint16_t i = 0; i < SPECTRUM_POINT_COUNT; i++)
  {
    if ((i != peak) && (spectrum_values_mv[i] > threshold))
    {
      if (spur_count < 255U)
      {
        spur_count++;
      }
    }
  }

  spectrum_snapshot.peak_index = peak;
  spectrum_snapshot.peak_amplitude_mv = peak_value;
  spectrum_snapshot.spur_count = spur_count;
}

static void Spectrum_StartSweep(void)
{
  for (uint16_t i = 0; i < SPECTRUM_POINT_COUNT; i++)
  {
    spectrum_values_mv[i] = 0;
  }

  spectrum_snapshot.mode = SPECTRUM_MODE_SWEEP;
  spectrum_snapshot.state = SPECTRUM_HOST_SWEEPING;
  spectrum_snapshot.point_index = 0;
  spectrum_snapshot.peak_index = 0;
  spectrum_snapshot.peak_amplitude_mv = 0;
  spectrum_snapshot.spur_count = 0;
  spectrum_sweep_started = 1;
  spectrum_last_step_tick = 0;
}

static void Spectrum_SetCurrentPoint(uint16_t index)
{
  uint32_t rf_khz = Spectrum_IndexToRfKHz(index);
  uint32_t lo_khz = Spectrum_RfToLoKHz(rf_khz);

  spectrum_snapshot.point_index = index;
  spectrum_snapshot.rf_khz = rf_khz;
  spectrum_snapshot.lo_khz = lo_khz;
  spectrum_snapshot.pll_locked = 1;

  /*
   * The existing ADF4351 driver uses 0.1 MHz units: 900 means 90.0 MHz.
   * RF 80.0 MHz maps to LO 90.0 MHz, so 900 is the first sweep command.
   */
  ADF4351_SetFreq(lo_khz / 100UL);
}

void SpectrumSystem_Init(void)
{
  spectrum_snapshot.mode = SPECTRUM_MODE_SWEEP;
  spectrum_snapshot.state = SPECTRUM_HOST_IDLE;
  spectrum_snapshot.rf_khz = SPECTRUM_RF_START_KHZ;
  spectrum_snapshot.lo_khz = Spectrum_RfToLoKHz(SPECTRUM_RF_START_KHZ);
  spectrum_snapshot.point_index = 0;
  spectrum_snapshot.amplitude_mv = 0;
  spectrum_snapshot.peak_index = 0;
  spectrum_snapshot.peak_amplitude_mv = 0;
  spectrum_snapshot.spur_count = 0;
  spectrum_snapshot.pll_locked = 0;

  Spectrum_StartSweep();
}

void SpectrumSystem_Task(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t dwell_ms = spectrum_sweep_time_ms / SPECTRUM_POINT_COUNT;

  if (dwell_ms == 0UL)
  {
    dwell_ms = 1UL;
  }

  if (spectrum_snapshot.state != SPECTRUM_HOST_SWEEPING)
  {
    return;
  }

  if ((spectrum_last_step_tick != 0UL) && ((now - spectrum_last_step_tick) < dwell_ms))
  {
    return;
  }

  spectrum_last_step_tick = now;
  Spectrum_SetCurrentPoint(spectrum_snapshot.point_index);
  spectrum_snapshot.amplitude_mv = Spectrum_ReadAmplitudeMv();
  spectrum_values_mv[spectrum_snapshot.point_index] = spectrum_snapshot.amplitude_mv;

  if (spectrum_snapshot.amplitude_mv > spectrum_snapshot.peak_amplitude_mv)
  {
    spectrum_snapshot.peak_amplitude_mv = spectrum_snapshot.amplitude_mv;
    spectrum_snapshot.peak_index = spectrum_snapshot.point_index;
  }

  Spectrum_SendPoint();

  if ((spectrum_snapshot.point_index % 10U) == 0U)
  {
    Spectrum_SendStatus();
  }

  if (spectrum_snapshot.point_index >= (SPECTRUM_POINT_COUNT - 1U))
  {
    Spectrum_UpdatePeakAndSpur();
    spectrum_snapshot.state = SPECTRUM_HOST_DONE;
    spectrum_sweep_started = 0;
    Spectrum_SendStatus();
    Spectrum_SendResult();
  }
  else
  {
    spectrum_snapshot.point_index++;
  }
}

void SpectrumSystem_OnKey(char key)
{
  if (key == 'A')
  {
    spectrum_snapshot.mode = SPECTRUM_MODE_LO_MANUAL;
    spectrum_snapshot.state = SPECTRUM_HOST_IDLE;
  }
  else if ((key == 'B') || (key == 'D'))
  {
    Spectrum_StartSweep();
  }
  else if (key == '*')
  {
    if (spectrum_sweep_time_ms > SPECTRUM_SWEEP_TIME_MIN_MS)
    {
      spectrum_sweep_time_ms -= 1000UL;
    }
  }
  else if (key == '#')
  {
    if (spectrum_sweep_time_ms < SPECTRUM_SWEEP_TIME_MAX_MS)
    {
      spectrum_sweep_time_ms += 1000UL;
    }
  }

  if (spectrum_sweep_started == 0U)
  {
    Spectrum_SendStatus();
  }
}

SpectrumHostSnapshot SpectrumSystem_GetSnapshot(void)
{
  return spectrum_snapshot;
}
