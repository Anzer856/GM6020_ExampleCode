#include "TrueMain.h"
#include "GM6020_HW.h"
#include "UART_IO.h"
#include "stdio.h"
extern GM6020_TypeDef GM6020[GM6020_ID_MAX];

void MainSetup()
{
    TIM4->CR1 |= TIM_CR1_CEN_Msk;
    // HAL_TIM_Base_Start();

    HAL_Delay(50);
    //=======================CAN=====================
    CAN_FilterTypeDef FilterConfig;
    FilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    FilterConfig.FilterScale      = CAN_FILTERSCALE_16BIT; // 16位模式
    FilterConfig.FilterMode       = CAN_FILTERMODE_IDMASK; // 掩码

    FilterConfig.FilterBank           = 0;                                   // 配置Bank0
    FilterConfig.FilterIdHigh         = (GM6020_BackMailBaseID & 0xFF) << 5; // 左对齐，只接受GM6020报文
    FilterConfig.FilterMaskIdHigh     = 0xFC00;
    FilterConfig.FilterFIFOAssignment = CAN_FilterFIFO0;

    HAL_CAN_ConfigFilter(&hcan, &FilterConfig);
    HAL_CAN_Start(&hcan);
    CAN1->IER |= CAN_IER_FMPIE0;
    //======================USART1=====================

    Debug_TXBufferClear_IT();
    USART1->CR1 |= USART_CR1_RXNEIE_Msk;
    USART1->CR1 |= USART_CR1_TXEIE_Msk;
    //======================
    for (uint8_t cnt = 1; cnt <= GM6020_ID_MAX; cnt++)
    {
        GM6020[cnt - 1].ID = cnt;
    }
    HAL_Delay(1000);
    return;
}
void MainLoop1()
{
    while (1)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        osDelay(18);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        osDelay(2);

        
        printf("RM2027\n");

        CAN_FilterTypeDef FilterConfig;
        FilterConfig.FilterActivation = CAN_FILTER_ENABLE;
        FilterConfig.FilterScale      = CAN_FILTERSCALE_16BIT; // 16位模式
        FilterConfig.FilterMode       = CAN_FILTERMODE_IDMASK; // 掩码

        FilterConfig.FilterBank           = 0;                            // 配置Bank0
        FilterConfig.FilterIdHigh         = (GM6020_BackMailBaseID) << 5; // 左对齐，只接受GM6020报文
        FilterConfig.FilterMaskIdHigh     = 0xFC00;
        FilterConfig.FilterFIFOAssignment = CAN_FilterFIFO0;

        HAL_CAN_ConfigFilter(&hcan, &FilterConfig);
        HAL_CAN_Start(&hcan);
        uint8_t idata[]  = {0, 10, 0, 20, 0, 30, 25, 0};
        uint32_t MailBox = 0;
        CAN_TxHeaderTypeDef TxHeader;
        TxHeader.DLC                = 8;
        TxHeader.ExtId              = 0x00;
        TxHeader.IDE                = CAN_ID_STD;
        TxHeader.RTR                = CAN_RTR_DATA;
        TxHeader.StdId              = 0x205;
        TxHeader.TransmitGlobalTime = DISABLE;
        HAL_CAN_AddTxMessage(&hcan, &TxHeader, idata, &MailBox);
        osDelay(1);
    }
    return;
}
void MainLoop2()
{
    while (1)
    {
        osDelay(1);
    }
    return;
}
void MainLoop3()
{
    while (1)
    {
        const uint8_t Timeoutus = 10, Ratio = 10;
        uint8_t Counter[GM6020_ID_MAX] = {};
        for (uint8_t ID = 1; ID <= GM6020_ID_MAX; ID++)
        {
            GM6020_TypeDef* pGM6020 = &GM6020[ID];
            // 当数据更新时计算
            if (pGM6020->MotorFeedback.IsUpdated)
            {
                pGM6020->Update();
                // 控制pid刷新比例
                if (Counter[ID] % Ratio == 0 && pGM6020->PIDAngleEnable)
                {
                    pGM6020->Update_PIDAngle();
                }
                pGM6020->Update_PIDSpeed();
                Counter[ID]++;

                pGM6020->MotorFeedback.IsUpdated = 0;
            }
            else if ((uint16_t)((UINT16_MAX - usTickCNT) + pGM6020->UpdateLastTickus) > Timeoutus && pGM6020->IsOK == 1)
            {
                // 断联检测
                pGM6020->IsOK = 0;
            }
        }
        GM6020_SendCurrentConfig();
        osDelay(1);
    }
    return;
}