#include "PID.h"
#include "can.h"
#include "math.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"
#include "tim.h"
#include <stdint.h>

#define GM6020_ID_MAX 7
#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204
#define GM6020_EncoderAngle_MAX 8191

#define GM6020_Current_BIN_MAX 16384
#define GM6020_Current_MAX 3
#define GM6020_Voltage_BIN_MAX 25000
#define GM6020_Voltage_MAX 24

#define PID_SPEED_ANGLE_HANDLERatio 10

#define usTickCNT (TIM4->CNT)
int32_t abs_int(int32_t N)
{
    return N > 0 ? N : (-N);
}
typedef struct
{
    int16_t ActEncoderAngle;
    int16_t ActSpeed;
    int16_t ActCurrent;
    uint8_t Temperature;
} GM6020_FeedbackTypeDef;
typedef struct
{
    uint8_t ID;
    uint8_t IsOK;
    GM6020_FeedbackTypeDef MotorFeedback;
    PID_HandleTypeDef PidSpeed;
    PID_HandleTypeDef PidAngle;
    uint32_t PidSpeedHandleTimes;
    uint8_t PidAngleEnable;
    // 角度使用定点数，以编码器量程(GM6020_EncoderAngle_MAX)为一圈
    int32_t BigAngleNum; // 整圈数
    int32_t SumAngle;    // 总和角度
    int32_t TAngle, TAngle_MIN, TAngle_MAX;
    uint16_t LastEncoderAngle;

    float TSpeed, TSpeed_MIN, TSpeed_MAX;      //
    uint32_t SpeedLastTickus, AngleLastTickus; // 单位:us
} GM6020_TypeDef;
GM6020_TypeDef GM6020[7];
uint8_t GM6020_VoltageDatas_Group[2][4] = {};
uint8_t GM6020_CurrentDatas_Group[2][4] = {};
// 输入单位V
void GM6020_SetVoltage(uint8_t GM6020_ID, float Voltage)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    int16_t VoltageBIN                                                          = (int16_t)Voltage * GM6020_Voltage_BIN_MAX / GM6020_Voltage_MAX;
    GM6020_VoltageDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2]     = (uint8_t)(VoltageBIN & 0xFF00) >> 8;
    GM6020_VoltageDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2 + 1] = (uint8_t)(VoltageBIN & 0x00FF);
    return;
}
// 输入单位A
void GM6020_SetCurrent(uint8_t GM6020_ID, float Current)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    int16_t CurrentBIN                                                          = (int16_t)Current * GM6020_Current_BIN_MAX / GM6020_Current_MAX;
    GM6020_CurrentDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2]     = (uint8_t)(CurrentBIN & 0xFF00) >> 8;
    GM6020_CurrentDatas_Group[(GM6020_ID - 1) / 4][(GM6020_ID - 1) % 4 * 2 + 1] = (uint8_t)(CurrentBIN & 0x00FF);

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

    pGM6020->MotorFeedback.ActEncoderAngle = (uint16_t)pDatas[0] << 8;
    pGM6020->MotorFeedback.ActEncoderAngle |= (uint16_t)pDatas[1];

    pGM6020->MotorFeedback.ActSpeed = (uint16_t)pDatas[2] << 8;
    pGM6020->MotorFeedback.ActSpeed |= (uint16_t)pDatas[3];

    pGM6020->MotorFeedback.ActCurrent = (uint16_t)pDatas[4] << 8;
    pGM6020->MotorFeedback.ActCurrent |= (uint16_t)pDatas[5];

    pGM6020->MotorFeedback.Temperature = (uint16_t)pDatas[6];

    return;
}
void GM6020_Reinit(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    //====================
    pGM6020->AngleLastTickus     = usTickCNT;
    pGM6020->SpeedLastTickus     = usTickCNT;
    pGM6020->PidSpeedHandleTimes = 0;
    pGM6020->BigAngleNum         = 0;
    pGM6020->TSpeed              = 0;
    pGM6020->SumAngle            = 0;
    pGM6020->IsOK                = 1;
    return;
}
void GM6020_Init(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020                = &GM6020[GM6020_ID - 1];
    pGM6020->MotorFeedback.ActEncoderAngle = 0;
    pGM6020->MotorFeedback.ActCurrent      = 0;
    pGM6020->MotorFeedback.ActSpeed        = 0;
    pGM6020->MotorFeedback.Temperature     = 0;
    GM6020_Reinit(pGM6020->ID);
    pGM6020->IsOK = 1;
    return;
}

//======================================================================================
//===============================================================================================

void GM6020_SetTragetAngle(uint8_t GM6020_ID, float TragetAngle)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    TragetAngle *= GM6020_EncoderAngle_MAX;
    // 转换单位圈->定点数
    //  判断角度是否超限
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
void GM6020_SetTragetSpeed(uint8_t GM6020_ID, float TragetSpeed)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    pGM6020->TSpeed         = TragetSpeed * GM6020_EncoderAngle_MAX;
}
void GM6020_Update(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    if (pGM6020->IsOK == 0)
    {
        GM6020_Reinit(pGM6020->ID);
        return;
    }
    // 判断角度是否突变
    if (abs_int(pGM6020->LastEncoderAngle
                - pGM6020->MotorFeedback.ActEncoderAngle)
        > GM6020_EncoderAngle_MAX / 2)
    {
        if (pGM6020->MotorFeedback.ActEncoderAngle < GM6020_EncoderAngle_MAX / 2 && pGM6020->MotorFeedback.ActSpeed > 0)
        {
            pGM6020->BigAngleNum += 1;
        }
        else if (pGM6020->MotorFeedback.ActEncoderAngle > GM6020_EncoderAngle_MAX / 2 && pGM6020->MotorFeedback.ActSpeed < 0)
        {
            pGM6020->BigAngleNum -= 1;
        }
    }
    // 计算求和角度
    pGM6020->SumAngle = pGM6020->BigAngleNum * GM6020_EncoderAngle_MAX + pGM6020->MotorFeedback.ActEncoderAngle;
    // 保存角度
    pGM6020->LastEncoderAngle = pGM6020->MotorFeedback.ActEncoderAngle;
    //================================================AnglePID============================
    // 计算位置环,处理频率为速度环的PID_SPEED_ANGLE_HANDLERatio分之一，
    // Er_Angle单位使用圈
    if (pGM6020->PidAngleEnable && pGM6020->PidSpeedHandleTimes % PID_SPEED_ANGLE_HANDLERatio == 0)
    {
        // 计算dT,单位s,考虑溢出
        float dT_Angle;
        if (pGM6020->AngleLastTickus < usTickCNT)
        {
            dT_Angle = usTickCNT - pGM6020->AngleLastTickus;
        }
        else
        {
            // TIM4 CNT Reg 16bit，最大计数60ms
            dT_Angle = UINT16_MAX - pGM6020->AngleLastTickus + usTickCNT;
        }
        dT_Angle /= 1000000;
        // 单位转换
        float Er_Angle = (pGM6020->TAngle - pGM6020->SumAngle) / GM6020_EncoderAngle_MAX;
        PID_Caculate(&pGM6020->PidAngle, dT_Angle, Er_Angle);
        GM6020_SetTragetSpeed(pGM6020->ID, pGM6020->PidAngle.Output);
    }
    //================================================SpeedPID============================
    // 计算dT,单位s,考虑溢出
    float dT_Speed;
    if (pGM6020->SpeedLastTickus < usTickCNT)
    {
        dT_Speed = usTickCNT - pGM6020->SpeedLastTickus;
    }
    else
    {
        // TIM4 CNT Reg 16bit，最大计数60ms
        dT_Speed = UINT16_MAX - pGM6020->SpeedLastTickus + usTickCNT;
    }
    dT_Speed /= 1000000;
    pGM6020->SpeedLastTickus = usTickCNT;
    // 计算速度环(使用电流模式)
    // Er_Speed单位圈每秒
    float Er_Speed = (float)(pGM6020->TSpeed / GM6020_EncoderAngle_MAX - pGM6020->MotorFeedback.ActSpeed / 60);
    PID_Caculate(&pGM6020->PidSpeed, dT_Speed, Er_Speed);

    // 发送应用
    GM6020_SetCurrent(pGM6020->ID, pGM6020->PidSpeed.Output);
    GM6020_SendCurrentConfig();

    return;
}
void GM6020_CheckIsOK()
{
}

// CAN报文处理
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
    GM6020_Update(RxMail.StdId - GM6020_BackMailBaseID);
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
    GM6020_Update(RxMail.StdId - GM6020_BackMailBaseID);
}