#include "BoardComm_User.h"

static UART_HandleTypeDef *board_comm_uart = &huart3;

static uint8_t BoardComm_Checksum(uint8_t cmd, const uint8_t *data, uint8_t len)
{
  uint8_t checksum = cmd ^ len;

  for (uint8_t i = 0; i < len; i++)
  {
    checksum ^= data[i];
  }

  return checksum;
}

void BoardComm_Init(void)
{
  board_comm_uart = &huart3;
}

BoardComm_Status BoardComm_Send(uint8_t cmd, const uint8_t *data, uint8_t len)
{
  uint8_t frame[BOARD_COMM_MAX_PAYLOAD + 5];

  if (len > BOARD_COMM_MAX_PAYLOAD)
  {
    return BOARD_COMM_LENGTH_ERROR;
  }

  if ((len != 0U) && (data == 0))
  {
    return BOARD_COMM_ERROR;
  }

  frame[0] = BOARD_COMM_HEAD1;
  frame[1] = BOARD_COMM_HEAD2;
  frame[2] = cmd;
  frame[3] = len;

  for (uint8_t i = 0; i < len; i++)
  {
    frame[4 + i] = data[i];
  }

  frame[4 + len] = BoardComm_Checksum(cmd, data, len);

  if (HAL_UART_Transmit(board_comm_uart, frame, (uint16_t)(len + 5U), BOARD_COMM_TIMEOUT_MS) != HAL_OK)
  {
    return BOARD_COMM_TIMEOUT;
  }

  return BOARD_COMM_OK;
}

BoardComm_Status BoardComm_Receive(uint8_t *cmd, uint8_t *data, uint8_t *len, uint32_t timeout)
{
  uint8_t header[4];
  uint8_t checksum;

  if ((cmd == 0) || (data == 0) || (len == 0))
  {
    return BOARD_COMM_ERROR;
  }

  if (HAL_UART_Receive(board_comm_uart, header, sizeof(header), timeout) != HAL_OK)
  {
    return BOARD_COMM_TIMEOUT;
  }

  if ((header[0] != BOARD_COMM_HEAD1) || (header[1] != BOARD_COMM_HEAD2))
  {
    return BOARD_COMM_ERROR;
  }

  if (header[3] > BOARD_COMM_MAX_PAYLOAD)
  {
    return BOARD_COMM_LENGTH_ERROR;
  }

  *cmd = header[2];
  *len = header[3];

  if (*len != 0U)
  {
    if (HAL_UART_Receive(board_comm_uart, data, *len, timeout) != HAL_OK)
    {
      return BOARD_COMM_TIMEOUT;
    }
  }

  if (HAL_UART_Receive(board_comm_uart, &checksum, 1, timeout) != HAL_OK)
  {
    return BOARD_COMM_TIMEOUT;
  }

  if (checksum != BoardComm_Checksum(*cmd, data, *len))
  {
    return BOARD_COMM_CHECKSUM_ERROR;
  }

  return BOARD_COMM_OK;
}

BoardComm_Status BoardComm_Ping(void)
{
  return BoardComm_Send(0x01, 0, 0);
}
