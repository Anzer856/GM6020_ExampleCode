#ifndef UART_IO_DEFINED
#define UART_IO_DEFINED

#include "cmsis_os.h"
#include "stm32f1xx.h"
#include "usart.h"

#ifdef __cplusplus
 extern "C" {
#endif
#define Debug_Handle_UART huart1
#define Debug_UART USART1

#define Debug_TXBufferSize 64
#define Debug_RXBufferSize 32

void Debug_RXPutBuffer_IT(uint8_t ch);
void Debug_TXBufferClear_IT(void);
int __io_getchar(void);
double Debug_GetInt(void);
double Debug_GetFloat(void);
void Debug_WaitChar(uint8_t ch);
#ifdef __cplusplus
  }
#endif
#endif