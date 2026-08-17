#ifndef UART_IO_DEFINED
#define UART_IO_DEFINED

#include "stm32f1xx.h"
#include "usart.h"
#include "cmsis_os.h"

#define Debug_Handle_UART huart1
#define Debug_UART USART1

#define Debug_PrintfTXBufferSize 64
#define Debug_PrintfRXBufferSize 32

void Debug_RXPutBuffer_IT(uint8_t ch);
void Debug_TXBufferClear_IT(void);
int __io_getchar(void);
double Debug_GetInt(void);
double Debug_GetFloat(void);
void Debug_WaitChar(uint8_t ch);
#endif