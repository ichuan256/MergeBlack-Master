#ifndef _BOARD_COMM_USER_H_
#define _BOARD_COMM_USER_H_

#include "usart.h"

/*
 * MergeBlack 板间通讯主机模块
 *
 * CubeMX/Keil 配置:
 *   - 外设: USART3, Asynchronous
 *   - 引脚: PB10 = USART3_TX, PB11 = USART3_RX
 *   - 参数: 115200 baud, 8 data bits, no parity, 1 stop bit, no flow control
 *
 * 板间连接:
 *   - MergeBlack PB10(TX) -> 从机 RX
 *   - MergeBlack PB11(RX) <- 从机 TX
 *   - MergeBlack GND      -> 从机 GND
 *   - 两端电平均为 3.3V TTL UART，不可直接接 RS232 电平
 *
 * 使用方式:
 *   1. main.c 中先调用 MX_USART3_UART_Init()
 *   2. 再调用 BoardComm_Init()
 *   3. 主机发送: BoardComm_Send(cmd, data, len)
 *   4. 主机接收: BoardComm_Receive(&cmd, data, &len, timeout)
 *
 * 帧格式:
 *   0xA5 0x5A CMD LEN DATA... CHECKSUM
 *   CHECKSUM = CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[n-1]
 */

#define BOARD_COMM_HEAD1        0xA5
#define BOARD_COMM_HEAD2        0x5A
#define BOARD_COMM_MAX_PAYLOAD  32
#define BOARD_COMM_TIMEOUT_MS   20

typedef enum {
  BOARD_COMM_OK = 0,
  BOARD_COMM_ERROR,
  BOARD_COMM_TIMEOUT,
  BOARD_COMM_LENGTH_ERROR,
  BOARD_COMM_CHECKSUM_ERROR
} BoardComm_Status;

void BoardComm_Init(void);
BoardComm_Status BoardComm_Send(uint8_t cmd, const uint8_t *data, uint8_t len);
BoardComm_Status BoardComm_Receive(uint8_t *cmd, uint8_t *data, uint8_t *len, uint32_t timeout);
BoardComm_Status BoardComm_Ping(void);

#endif
