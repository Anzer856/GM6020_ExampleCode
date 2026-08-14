#include "PID.h"
#include "can.h"
#include "math.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"
#include "tim.h"

#define GM6020_ID_MAX 7
#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204

#define GM6020_EncoderAngle_MAX 8191

#define usTick (TIM4->CNT)
int32_t abs_int(int32_t N)
{
    return N>0?N:(-N);
}
typedef struct
{
    int16_t ActAngle;
    int16_t ActSpeed;
    int16_t ActCurrent;
    uint8_t Temperature;
    uint8_t IsUpdated;
} GM6020_FeedbackTypeDef;
typedef struct
{
    uint8_t ID;

    GM6020_FeedbackTypeDef MotorFeedback;
    PID_HandleTypeDef PidSpeed;
    PID_HandleTypeDef PidAngle;

    int32_t BigAngleNum, SumAngle;
    uint16_t LastEncoderAngle;
    int32_t TAngle, TAngle_MIN, TAngle_MAX;

    int32_t TSpeed, TSpeed_MIN, TSpeed_MAX;
    uint32_t SpeedLastTickus, AngleLastTickus;
} GM6020_TypeDef;
GM6020_TypeDef GM6020[7];

uint8_t GM6020_VoltageDatas_Group[2][7] = {};

uint8_t GM6020_CurrentDatas_Group[2][7] = {};

void GM6020_SetVoltage(uint8_t GM6020_ID, int16_t Voltage)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_VoltageDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2]     = (uint8_t)(Voltage & 0xFF00) >> 8;
    GM6020_VoltageDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2 + 1] = (uint8_t)(Voltage & 0x00FF);
    return;
}
void GM6020_SetCurrent(uint8_t GM6020_ID, int16_t Current)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_CurrentDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2]     = (uint8_t)(Current & 0xFF00) >> 8;
    GM6020_CurrentDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2 + 1] = (uint8_t)(Current & 0x00FF);

    return;
}
HAL_StatusTypeDef GM6020_SendVoltageConfig()
{

    CAN_TxHeaderTypeDef TxMail;
    uint8_t* pTxDATAs;
    uint32_t Mailbox;
    TxMail.IDE   = CAN_ID_STD;
    TxMail.RTR   = CAN_RTR_DATA;
    TxMail.DLC   = 8;
    TxMail.ExtId = 0x00; // Only use StdId

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) < 2)
    {
        return HAL_BUSY;
    }
    // Send VoltageDatas_Group
    TxMail.StdId = GM6020_CANID_Group1;
    pTxDATAs     = GM6020_VoltageDatas_Group[0];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);

    TxMail.StdId = GM6020_CANID_Group2;
    pTxDATAs     = GM6020_VoltageDatas_Group[1];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);
    return HAL_OK;
}
HAL_StatusTypeDef GM6020_SendCurrentConfig()
{

    CAN_TxHeaderTypeDef TxMail;
    uint8_t* pTxDATAs;
    uint32_t Mailbox;
    TxMail.IDE   = CAN_ID_STD;
    TxMail.RTR   = CAN_RTR_DATA;
    TxMail.DLC   = 8;
    TxMail.ExtId = 0x00; // Only use StdId

    HAL_CAN_Start(&hcan);

    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) < 2)
    {
        return HAL_BUSY;
    }
    // Send CurrentDatas_Group1
    TxMail.StdId = GM6020_CANID_Group1;
    pTxDATAs     = GM6020_CurrentDatas_Group[0];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);

    TxMail.StdId = GM6020_CANID_Group2;
    pTxDATAs     = GM6020_CurrentDatas_Group[1];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);
    return HAL_OK;
}
void GM6020_WriteInfo(uint8_t GM6020_ID, uint8_t* pDatas)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    // offset

    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];

    pGM6020->MotorFeedback.ActAngle = (uint16_t)pDatas[0] << 8;
    pGM6020->MotorFeedback.ActAngle |= (uint16_t)pDatas[1];

    pGM6020->MotorFeedback.ActSpeed = (uint16_t)pDatas[2] << 8;
    pGM6020->MotorFeedback.ActSpeed |= (uint16_t)pDatas[3];

    pGM6020->MotorFeedback.ActCurrent = (uint16_t)pDatas[4] << 8;
    pGM6020->MotorFeedback.ActCurrent |= (uint16_t)pDatas[5];

    pGM6020->MotorFeedback.Temperature = (uint16_t)pDatas[6];

    pGM6020->MotorFeedback.IsUpdated = 1;
    return;
}
void GM6020_Init(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020            = &GM6020[GM6020_ID - 1];
    pGM6020->MotorFeedback.ActAngle    = 0;
    pGM6020->MotorFeedback.ActCurrent  = 0;
    pGM6020->MotorFeedback.ActSpeed    = 0;
    pGM6020->MotorFeedback.IsUpdated   = 0;
    pGM6020->MotorFeedback.Temperature = 0;
    return;
}
//======================================================================================
//===============================================================================================

void Motor_SetTragetAngle(uint8_t GM6020_ID, double TragetAngle)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];

    if (TragetAngle > pGM6020->TAngle_MAX)
    {
        pGM6020->TAngle = pGM6020->TAngle_MAX;
    }
    else if (TragetAngle < pGM6020->TAngle_MIN)
    {
        pGM6020->TAngle = pGM6020->TAngle_MIN;
    }
    else
    {
        pGM6020->TAngle = TragetAngle;
    }
    return;
}

void Motor_Update(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    // 判断角度是否突变
    if (abs_int(pGM6020->LastEncoderAngle
            - pGM6020->MotorFeedback.ActAngle)
        > GM6020_EncoderAngle_MAX / 2)
    {
        if (pGM6020->MotorFeedback.ActAngle < GM6020_EncoderAngle_MAX / 2 && pGM6020->MotorFeedback.ActSpeed > 0)
        {
            pGM6020->BigAngleNum += 1;
        }
        else if (pGM6020->MotorFeedback.ActAngle > GM6020_EncoderAngle_MAX / 2 && pGM6020->MotorFeedback.ActSpeed < 0)
        {
            pGM6020->BigAngleNum -= 1;
        }
    }
    // 计算dT,单位s,考虑溢出
    double dTSpeed, dTAngle;
    if (pGM6020->SpeedLastTickus < TIM4->CNT)
    {
        dTSpeed = TIM4->CNT - pGM6020->SpeedLastTickus;
    }
    else
    {

        dTSpeed = 0xFFFF - pGM6020->SpeedLastTickus + TIM4->CNT;
    }
    dTSpeed /= 1000000;
    // 计算求和角度
    pGM6020->SumAngle = pGM6020->BigAngleNum * GM6020_EncoderAngle_MAX + pGM6020->MotorFeedback.ActAngle;
    // 保存微秒数
    pGM6020->LastEncoderAngle = pGM6020->MotorFeedback.ActAngle;
    // 计算速度环(使用电流模式)
    double MotoOutput = PID_Caculate(&pGM6020->PidSpeed, dTSpeed, pGM6020->TSpeed - pGM6020->MotorFeedback.ActSpeed);

    if (pGM6020->AngleLastTickus < TIM4->CNT)
    {
        dTAngle = TIM4->CNT - pGM6020->AngleLastTickus;
    }
    else
    {
        dTAngle = 0xFFFF - pGM6020->AngleLastTickus + TIM4->CNT;
    }
    dTAngle /= 1000000;
    pGM6020->AngleLastTickus = TIM4->CNT;
    GM6020_SetCurrent(pGM6020->ID, MotoOutput);
    // 计算位置环
    pGM6020->TSpeed = PID_Caculate(&pGM6020->PidAngle, dTAngle, pGM6020->TAngle - pGM6020->SumAngle);
    // 发送应用
    GM6020_SendCurrentConfig();

    pGM6020->MotorFeedback.IsUpdated = 0;
    return;
}
void Motor_Init(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];

    GM6020_Init(pGM6020->ID);
    pGM6020->LastEncoderAngle = 0;
    pGM6020->BigAngleNum      = 0;
    pGM6020->TAngle           = 0;
    pGM6020->AngleLastTickus  = 0;
    pGM6020->LastEncoderAngle = 0;
    return;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    CAN_RxHeaderTypeDef RxMail;
    uint8_t RxDATAs[8] = {};
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxMail, RxDATAs);
    // Mail from GM6020
    if ((GM6020_BackMailBaseID + 1) <= RxMail.StdId && RxMail.StdId <= (GM6020_BackMailBaseID + 7))
    {
        GM6020_WriteInfo(RxMail.StdId - GM6020_BackMailBaseID, RxDATAs);
    }
    // 更新对应电机PID
    Motor_Update(RxMail.StdId - GM6020_BackMailBaseID);
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    CAN_RxHeaderTypeDef RxMail;
    uint8_t RxDATAs[8] = {};
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxMail, RxDATAs);
    // Mail from GM6020
    if ((GM6020_BackMailBaseID + 1) <= RxMail.StdId && RxMail.StdId <= (GM6020_BackMailBaseID + 7))
    {
        GM6020_WriteInfo(RxMail.StdId - GM6020_BackMailBaseID, RxDATAs);
    }
    // 更新对应电机PID
    Motor_Update(RxMail.StdId - GM6020_BackMailBaseID);
}