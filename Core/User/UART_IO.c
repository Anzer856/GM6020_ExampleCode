
#include "stdio.h"
#include "stm32f1xx.h"
#include "usart.h"
#include <stdint.h>

#define Debug_Handle_UART huart1
#define Debug_PrintfTXBufferSize 64
uint8_t Debug_PrintfTXBuffer[Debug_PrintfTXBufferSize] = {}, Debug_PrintfTXBufferTop = 0, Debug_PrintfTXBufferLen = 0;
#include "stdio.h"
/**
 * @brief  清空printf发送缓存
 * @param 	无
 * @retval 无
 */
void Debug_PrintfTXBufferClear()
{
    if (HAL_UART_GetState(&Debug_Handle_UART) != HAL_UART_STATE_READY)
    {
        return;
    }
    if (Debug_PrintfTXBufferTop + Debug_PrintfTXBufferLen > Debug_PrintfTXBufferSize)
    {
        HAL_UART_Transmit(&Debug_Handle_UART, (uint8_t*)&Debug_PrintfTXBuffer[Debug_PrintfTXBufferTop], Debug_PrintfTXBufferSize - Debug_PrintfTXBufferTop, 1000);
        HAL_UART_Transmit(&Debug_Handle_UART, (uint8_t*)&Debug_PrintfTXBuffer[0], Debug_PrintfTXBufferLen - (Debug_PrintfTXBufferSize - Debug_PrintfTXBufferTop), 1000);
    }
    else
    {
        HAL_UART_Transmit(&Debug_Handle_UART, (uint8_t*)&Debug_PrintfTXBuffer[Debug_PrintfTXBufferTop], Debug_PrintfTXBufferLen, 1000);
        Debug_PrintfTXBufferTop = (Debug_PrintfTXBufferTop + Debug_PrintfTXBufferLen) % Debug_PrintfTXBufferSize;
        Debug_PrintfTXBufferLen = 0;
    }

    Debug_PrintfTXBufferTop = 0;
    return;
}
/**
 * @brief  fputc(printf)重定向
 * @param  ch 输入单字符
 * @retval 无
 */

int _write(int file, char* ptr, int len)
{

    if (Debug_PrintfTXBufferLen < Debug_PrintfTXBufferSize)
    {
        for (uint32_t cnt = 0; cnt < len; cnt++)
        {
            Debug_PrintfTXBuffer[(Debug_PrintfTXBufferTop + Debug_PrintfTXBufferLen) % Debug_PrintfTXBufferSize] = ptr[cnt];
            Debug_PrintfTXBufferLen++;
        }
    }
    if (Debug_PrintfTXBufferLen == Debug_PrintfTXBufferSize)
    {
        Debug_PrintfTXBufferClear();
    }
    return *ptr;
}
