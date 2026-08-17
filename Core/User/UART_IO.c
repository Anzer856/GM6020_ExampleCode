#include "UART_IO.h"

#include <stdint.h>

volatile uint8_t Debug_TXBuffer[Debug_TXBufferSize] = {}, Debug_TXBufferTop = 0, Debug_TXBufferLen = 0;
volatile uint8_t Debug_RXBuffer[Debug_TXBufferSize] = {}, Debug_RXBufferTop = 0, Debug_RXBufferLen = 0;

/**
 * @brief  清空printf发送缓存
 * @param 	无
 * @retval 无
 */
void Debug_TXBufferClear_IT(void)
{

    if (Debug_TXBufferLen > 0)
    {

        if ((Debug_UART->SR & USART_SR_TXE))
        {
            Debug_UART->DR = Debug_TXBuffer[Debug_TXBufferTop];

            Debug_TXBufferTop = (Debug_TXBufferTop + 1) % Debug_TXBufferSize;
            Debug_TXBufferLen--;
        }
        if (Debug_TXBufferLen > 0)
        {
            Debug_UART->CR1 |= USART_CR1_TXEIE_Msk;
        }
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

    if (Debug_TXBufferLen < Debug_TXBufferSize)
    {
        
        for (uint32_t cnt = 0; cnt < len; cnt++)
        {
            if (Debug_TXBufferLen < Debug_TXBufferSize)
            {
                //禁用中断防止失序错误导致数据丢失
                Debug_UART->CR1 &= ~USART_CR1_TXEIE_Msk;
                Debug_TXBuffer[(Debug_TXBufferTop + Debug_TXBufferLen) % Debug_TXBufferSize] = ptr[cnt];
                Debug_TXBufferLen++;
                Debug_TXBufferClear_IT();
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
        if (Debug_RXBufferLen > 0)
        {
            data              = Debug_RXBuffer[(Debug_RXBufferTop) % Debug_RXBufferSize];
            Debug_RXBufferTop = (Debug_RXBufferTop + 1) % Debug_RXBufferSize;
            Debug_RXBufferLen--;

            return data;
        }
        osDelay(1);
    }
}

/**
 * @brief Debug_RXPutBuffer_IT将字符存入缓存
 * @param  无
 * @retval 返回字符
 */
void Debug_RXPutBuffer_IT(uint8_t ch)
{
    if (Debug_RXBufferLen < Debug_RXBufferSize)
    {
        Debug_RXBuffer[(Debug_RXBufferTop + Debug_RXBufferLen) % Debug_RXBufferSize] = ch;
        Debug_RXBufferLen++;
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