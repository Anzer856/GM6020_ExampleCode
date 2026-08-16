#ifndef UART_IO_DEFINED
#define UART_IO_DEFINED

#include "stm32f1xx.h"
#include "usart.h"
void Debug_RxPutcToBuffer(uint8_t ch);
void Debug_PrintfTXBufferClear();
int __io_getchar(void);
double Debug_GetInt(void);
double Debug_GetFloat(void);
void Debug_WaitChar(uint8_t ch);
#endif