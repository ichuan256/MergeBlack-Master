#ifndef _BOARD_COMM_USER_H_
#define _BOARD_COMM_USER_H_

#include "usart.h"

/*
 * MergeBlack 板间 UART 通信模块
 * ============================================================
 * 1. 模块用途
 *    用 USART3 实现两块板之间的简单通信。MergeBlack 当前按“主机”使用，
 *    可以主动发送命令，也可以接收另一块板返回的数据。
 *
 * 2. 硬件连接
 *    MergeBlack PB10 / USART3_TX  ->  对方 RX
 *    MergeBlack PB11 / USART3_RX  <-  对方 TX
 *    MergeBlack GND               <-> 对方 GND
 *    注意：两端必须是 3.3V TTL UART 电平，不能直接接 RS232 电平。
 *
 * 3. 串口参数
 *    USART3，115200 bps，8 数据位，1 停止位，无校验，无硬件流控。
 *
 * 4. 接收方式
 *    本模块已经加入“接收到空闲中断”方式：
 *      HAL_UARTEx_ReceiveToIdle_IT()
 *    当串口接收到一段数据，并且总线空闲超过约 1 帧时间后，HAL 会触发
 *    HAL_UARTEx_RxEventCallback()，本模块会在回调里解析完整数据帧。
 *
 * 5. main.c 中推荐调用顺序
 *      MX_USART3_UART_Init();
 *      BoardComm_Init();
 *      BoardComm_StartReceiveToIdleIT();
 *
 * 6. 收到数据后的使用方式
 *    用户只需要在自己的 .c 文件中重写 BoardComm_RxFrameCallback()：
 *      void BoardComm_RxFrameCallback(uint8_t cmd, const uint8_t *data,
 *                                     uint8_t len, BoardComm_Status status)
 *      {
 *          if (status == BOARD_COMM_OK) {
 *              // 根据 cmd 和 data 处理业务
 *          }
 *      }
 *
 * 7. 协议帧格式
 *      Byte0      0xA5，帧头 1
 *      Byte1      0x5A，帧头 2
 *      Byte2      CMD，命令字
 *      Byte3      LEN，数据区长度
 *      Byte4..N   DATA，数据区，可为空
 *      Last       CHECKSUM，校验字节
 *
 * 8. 校验算法
 *      CHECKSUM = CMD ^ LEN ^ DATA[0] ^ DATA[1] ^ ...
 */

/* 帧头第 1 字节。接收端用它判断一帧数据的开始。 */
#define BOARD_COMM_HEAD1        0xA5

/* 帧头第 2 字节。双字节帧头可以降低误判概率。 */
#define BOARD_COMM_HEAD2        0x5A

/* 单帧最大数据区长度，单位字节。 */
#define BOARD_COMM_MAX_PAYLOAD  32

/* 接收缓冲区长度：2 字节帧头 + CMD + LEN + 最大 DATA + CHECKSUM。 */
#define BOARD_COMM_RX_BUF_SIZE  (BOARD_COMM_MAX_PAYLOAD + 5U)

/* 阻塞式发送使用的默认超时时间，单位 ms。 */
#define BOARD_COMM_TIMEOUT_MS   20

/*
 * 板间通信函数返回值。
 * 这些状态既用于阻塞式收发，也用于中断接收回调中的解析结果。
 */
typedef enum {
  BOARD_COMM_OK = 0,          /* 操作成功，或收到的数据帧合法。 */
  BOARD_COMM_ERROR,           /* 参数错误、串口句柄错误或帧头错误。 */
  BOARD_COMM_TIMEOUT,         /* 阻塞式收发等待超时。 */
  BOARD_COMM_LENGTH_ERROR,    /* 数据长度超出协议范围，或收到半包/粘包。 */
  BOARD_COMM_CHECKSUM_ERROR   /* 校验失败，数据可能被干扰或协议不一致。 */
} BoardComm_Status;

/*
 * 初始化板间通信模块。
 * 当前内部绑定 USART3，也就是 CubeMX 生成的 huart3。
 */
void BoardComm_Init(void);

/*
 * 启动“接收到空闲中断”接收。
 * 调用成功后，USART3 收到数据并检测到 IDLE 空闲时，会进入 HAL 回调。
 */
BoardComm_Status BoardComm_StartReceiveToIdleIT(void);

/*
 * 停止 USART3 的中断接收。
 * 一般不需要调用，只有临时关闭通信或切换接收方式时使用。
 */
BoardComm_Status BoardComm_StopReceiveIT(void);

/*
 * 空闲中断事件处理函数。
 * 本函数由 HAL_UARTEx_RxEventCallback() 调用，用于解析接收到的数据帧。
 * 如果以后别的文件也要实现 HAL_UARTEx_RxEventCallback()，记得在里面
 * 调用 BoardComm_HandleRxIdleEvent(huart, size)，否则本模块收不到数据。
 */
void BoardComm_HandleRxIdleEvent(UART_HandleTypeDef *huart, uint16_t size);

/*
 * 用户接收回调接口。
 *
 * 参数说明：
 *   cmd    - 收到的命令字。
 *   data   - 收到的数据区指针。注意它指向模块内部接收缓冲区。
 *   len    - 数据区长度。
 *   status - 本帧解析状态。只有 BOARD_COMM_OK 时 cmd/data/len 才可靠。
 *
 * 重要说明：
 *   该函数在串口中断上下文中执行，不要在里面做耗时操作；推荐只复制数据、
 *   设置标志位，然后在主循环里处理真正的业务。
 */
void BoardComm_RxFrameCallback(uint8_t cmd, const uint8_t *data, uint8_t len, BoardComm_Status status);

/*
 * 发送一帧数据。
 * cmd  为命令字，data 为数据区，len 为数据长度。
 * 当 len 为 0 时，data 可以传 0。
 */
BoardComm_Status BoardComm_Send(uint8_t cmd, const uint8_t *data, uint8_t len);

/*
 * 阻塞式接收一帧数据。
 * 该接口主要保留给早期调试使用；正式连续接收推荐使用空闲中断方式。
 */
BoardComm_Status BoardComm_Receive(uint8_t *cmd, uint8_t *data, uint8_t *len, uint32_t timeout);

/* 发送一帧 PING 命令，常用于测试两块板串口连线是否正常。 */
BoardComm_Status BoardComm_Ping(void);

#endif