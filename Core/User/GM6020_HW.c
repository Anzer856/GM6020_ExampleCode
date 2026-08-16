#include "GM6020_HW.h"

GM6020_TypeDef GM6020[7];
uint8_t GM6020_VoltageDatas_Group[2][4] = {};
uint8_t GM6020_CurrentDatas_Group[2][4] = {};
static int32_t abs_int(int32_t N)
{
    return N > 0 ? N : (-N);
}
static float abs_float(float N)
{
    return N > 0 ? N : (-N);
}
// 输入单位V
void GM6020_SetVoltage(uint8_t GM6020_ID, float Voltage)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    int16_t VoltageBIN = 0;
    if (abs_float(Voltage) < GM6020_Voltage_BIN_MAX)
    {
        VoltageBIN = (int16_t)Voltage * GM6020_Voltage_BIN_MAX / GM6020_Voltage_MAX;
    }
    else
    {
        VoltageBIN = GM6020_Voltage_BIN_MAX;
    }

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
    int16_t CurrentBIN = 0;
    if (abs_float(Current) < GM6020_Current_MAX)
    {
        CurrentBIN = (int16_t)Current * GM6020_Current_BIN_MAX / GM6020_Current_MAX;
    }
    else
    {
        CurrentBIN = GM6020_Current_BIN_MAX;
    }

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
    pGM6020->MotorFeedback.IsUpdated   = 1;
    pGM6020->UpdateLastTickus          = usTickCNT;
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
    pGM6020->ID=GM6020_ID;
    pGM6020->AngleLastTickus = usTickCNT;
    pGM6020->SpeedLastTickus = usTickCNT;

    pGM6020->BigAngleNum      = 0;
    pGM6020->TSpeed           = 0;
    pGM6020->SumAngle         = 0;
    pGM6020->IsOK             = 1;
    pGM6020->UpdateLastTickus = usTickCNT;
    pGM6020->PIDAngleEnable=0;
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
    pGM6020->MotorFeedback.IsUpdated       = 0;
    GM6020_Reinit(pGM6020->ID);

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
GM6020_TypeDef* GM6020_GetInfop(uint8_t GM6020_ID)
{
    return &GM6020[(GM6020_ID - 1) % GM6020_ID_MAX];
}
void GM6020_SetTragetSpeed(uint8_t GM6020_ID, float TragetSpeed)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    pGM6020->TSpeed         = TragetSpeed;
}
void GM6020_Update_PIDAngle(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
    //================================================AnglePID============================
    // Er_Angle单位使用圈
    if (pGM6020->PIDAngleEnable)
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
        PID_Caculate(&pGM6020->PIDAngle, dT_Angle, Er_Angle);
        GM6020_SetTragetSpeed(pGM6020->ID, pGM6020->PIDAngle.Output);
    }
    return;
}
void GM6020_Update_PIDSpeed(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
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
    PID_Caculate(&pGM6020->PIDSpeed, dT_Speed, Er_Speed);
    return;
}
void GM6020_Update(uint8_t GM6020_ID)
{
    // 判断ID是否合法
    if (GM6020_ID > GM6020_ID_MAX || GM6020_ID < 1)
    {
        return;
    }
    GM6020_TypeDef* pGM6020 = &GM6020[GM6020_ID - 1];
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

    return;
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
}