#include "UART_IO.h"
#include "stm32f103xb.h"

uint8_t Debug_PrintfTXBuffer[Debug_PrintfTXBufferSize] = {}, Debug_PrintfTXBufferTop = 0, Debug_PrintfTXBufferLen = 0;

uint8_t Debug_PrintfRXBuffer[Debug_PrintfTXBufferSize] = {}, Debug_PrintfRXBufferTop = 0, Debug_PrintfRXBufferLen = 0;
#include "stdio.h"
/**
 * @brief  清空printf发送缓存
 * @param 	无
 * @retval 无
 */
void Debug_PrintfTXBufferClear(void)
{

    if (Debug_PrintfTXBufferLen > 0)
    {
        while (!(Debug_UART->SR & USART_SR_TXE))
        {
        }

        Debug_UART->DR          = Debug_PrintfTXBuffer[Debug_PrintfTXBufferTop];
        Debug_PrintfTXBufferTop = (Debug_PrintfTXBufferTop + 1) % Debug_PrintfTXBufferSize;
        Debug_PrintfTXBufferLen--;
        Debug_UART->CR1 |= USART_CR1_TXEIE_Msk;
    }
    else
    {
        Debug_UART->CR1 &= ~USART_CR1_TXEIE_Msk;
    }
    return;
}
/**
 * @brief  _write重定向
 * @param  file 输出方向(忽略)
 * @param  ch 输入字符串指针
 * @param  len 输入字符串长度
 * @retval 成功发送字符数量
 */

int _write(int file, char* ptr, int len)
{

    if (Debug_PrintfTXBufferLen  < Debug_PrintfTXBufferSize)
    {
        for (uint32_t cnt = 0; cnt < len&&Debug_PrintfTXBufferLen<Debug_PrintfTXBufferSize; cnt++)
        {
            
            Debug_PrintfTXBuffer[(Debug_PrintfTXBufferTop + Debug_PrintfTXBufferLen) % Debug_PrintfTXBufferSize] = ptr[cnt];
            Debug_PrintfTXBufferLen++;
            if (Debug_UART->SR & USART_SR_TXE_Msk)
            {
                Debug_PrintfTXBufferClear();
            }
        }
    }

    return len;
}

/**
 * @brief  __io_getchar重定向
 * @param  无
 * @retval 返回字符
 */
int __io_getchar(void)
{
    uint8_t data = -1;
    while (1)
    {
        if (Debug_PrintfRXBufferLen > 0)
        {
            data                    = Debug_PrintfRXBuffer[(Debug_PrintfRXBufferTop) % Debug_PrintfRXBufferSize];
            Debug_PrintfRXBufferTop = (Debug_PrintfRXBufferTop + 1) % Debug_PrintfRXBufferSize;
            Debug_PrintfRXBufferLen--;

            return data;
        }
        osDelay(1);
    }
}

/**
 * @brief Debug_RxPutcToBuffer将字符存入缓存
 * @param  无
 * @retval 返回字符
 */
void Debug_RxPutcToBuffer(uint8_t ch)
{
    if (Debug_PrintfRXBufferLen < Debug_PrintfRXBufferSize)
    {
        Debug_PrintfRXBuffer[(Debug_PrintfRXBufferTop + Debug_PrintfRXBufferLen) % Debug_PrintfRXBufferSize] = ch;
        Debug_PrintfRXBufferLen++;
    }
    return;
}
/**
 * @brief 接受数字
 * @param  无
 * @retval 返回字符
 */
double Debug_GetInt(void)
{
    int32_t num  = 0;
    uint8_t data = 0, polarity = 0;
    do
    {
        data = __io_getchar();
        if (data == '-')
        {
            polarity = 1;
        }
        else if (data >= '0' && data <= '9')
        {
            num *= 10;
            num += data - '0';
        }
        else
        {
            break;
        }

    } while (1);

    return polarity ? -num : num;
}
double Debug_GetFloat(void)
{
    float num = 0, nums = 1;
    uint8_t data = 0, polarity = 0, part = 0;
    do
    {
        data = __io_getchar();
        if (data == '-')
        {
            polarity = 1;
        }
        else if (data >= '0' && data <= '9')
        {
            num *= 10;
            num += data - '0';
        }
        else if (data == '.')
        {
            part = 1;
            break;
        }
        else
        {
            break;
        }
    } while (1);
    if (part == 1)
    {
        do
        {
            data = __io_getchar();
            if (data >= '0' && data <= '9')
            {
                nums /= 10;
                num += nums * (data - '0');
            }
            else
            {
                break;
            }
        } while (1);
    }
    return polarity ? -num : num;
}
void Debug_WaitChar(uint8_t ch)
{

    while (__io_getchar() != ch)
    {
    }
    return;
}