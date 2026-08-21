#include "GM6020_HW.h"

uint8_t GM6020_VoltageDatas_Group[2][4] = {};
uint8_t GM6020_CurrentDatas_Group[2][4] = {};

GM6020_TypeDef GM6020[7];
static int32_t abs_int(int32_t N)
{
    return N > 0 ? N : (-N);
}
static float abs_float(float N)
{
    return N > 0 ? N : (-N);
}
// 输入单位V
void GM6020_TypeDef::SetVoltage(float Voltage)
{
    int16_t VoltageBIN = 0;
    if (abs_float(Voltage) < GM6020_Voltage_BIN_MAX)
    {
        VoltageBIN = (int16_t)Voltage * GM6020_Voltage_BIN_MAX / GM6020_Voltage_MAX;
    }
    else
    {
        VoltageBIN = GM6020_Voltage_BIN_MAX;
    }
    uint8_t IDX                                                     = this->ID;
    GM6020_VoltageDatas_Group[(IDX - 1) / 4][(IDX - 1) % 4 * 2]     = (uint8_t)(VoltageBIN & 0xFF00) >> 8;
    GM6020_VoltageDatas_Group[(IDX - 1) / 4][(IDX - 1) % 4 * 2 + 1] = (uint8_t)(VoltageBIN & 0x00FF);
    return;
}
// 输入单位A
void GM6020_TypeDef::SetCurrent(float Current)
{
    int16_t CurrentBIN = 0;
    if (abs_float(Current) < GM6020_Current_MAX)
    {
        CurrentBIN = (int16_t)Current * GM6020_Current_BIN_MAX / GM6020_Current_MAX;
    }
    else
    {
        CurrentBIN = GM6020_Current_BIN_MAX;
    }
    uint8_t IDX                                                     = this->ID;
    GM6020_CurrentDatas_Group[(IDX - 1) / 4][(IDX - 1) % 4 * 2]     = (uint8_t)(CurrentBIN & 0xFF00) >> 8;
    GM6020_CurrentDatas_Group[(IDX - 1) / 4][(IDX - 1) % 4 * 2 + 1] = (uint8_t)(CurrentBIN & 0x00FF);

    return;
}
uint8_t GM6020_SendVoltageConfig()
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
        return 0;
    }
    // Send VoltageDatas_Group
    TxMail.StdId = GM6020_CANID_Group1;
    pTxDATAs     = GM6020_VoltageDatas_Group[0];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);

    TxMail.StdId = GM6020_CANID_Group2;
    pTxDATAs     = GM6020_VoltageDatas_Group[1];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);
    return 1;
}
uint8_t GM6020_SendCurrentConfig()
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
        return 0;
    }
    // Send CurrentDatas_Group1
    TxMail.StdId = GM6020_CANID_Group1;
    pTxDATAs     = GM6020_CurrentDatas_Group[0];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);

    TxMail.StdId = GM6020_CANID_Group2;
    pTxDATAs     = GM6020_CurrentDatas_Group[1];
    HAL_CAN_AddTxMessage(&hcan, &TxMail, pTxDATAs, &Mailbox);
    return 1;
}
void GM6020_TypeDef::WriteInfo(uint8_t* pDatas)
{

    this->MotorFeedback.ActEncoderAngle = (uint16_t)pDatas[0] << 8;
    this->MotorFeedback.ActEncoderAngle |= (uint16_t)pDatas[1];

    this->MotorFeedback.ActSpeed = (uint16_t)pDatas[2] << 8;
    this->MotorFeedback.ActSpeed |= (uint16_t)pDatas[3];

    this->MotorFeedback.ActCurrent = (uint16_t)pDatas[4] << 8;
    this->MotorFeedback.ActCurrent |= (uint16_t)pDatas[5];

    this->MotorFeedback.Temperature = (uint16_t)pDatas[6];
    this->MotorFeedback.IsUpdated   = 1;
    this->UpdateLastTickus          = usTickCNT;
    return;
}
void GM6020_TypeDef::Reinit()
{
    // 判断ID是否合法

    //====================
    this->AngleLastTickus = usTickCNT;
    this->SpeedLastTickus = usTickCNT;

    this->BigAngleNum      = 0;
    this->TSpeed           = 0;
    this->SumAngle         = 0;
    this->IsOK             = 1;
    this->UpdateLastTickus = usTickCNT;

    return;
}
GM6020_TypeDef::GM6020_TypeDef()
{
    PID_Init(&this->PIDAngle);
    PID_Init(&this->PIDSpeed);
    this->MotorFeedback.ActEncoderAngle = 0;
    this->MotorFeedback.ActCurrent      = 0;
    this->MotorFeedback.ActSpeed        = 0;
    this->MotorFeedback.Temperature     = 0;
    this->MotorFeedback.IsUpdated       = 0;

    this->PIDAngleEnable = 0;
    this->Reinit();

    return;
}

//======================================================================================
//===============================================================================================
/**
 * @brief 设置目标角度
 * @param uint8_t IDX 电机ID
 * @param float TragetAngle 目标角度，单位圈
 * @retval None
 */
void GM6020_TypeDef::SetTragetAngle(float TragetAngle)
{

    TragetAngle *= GM6020_EncoderAngle_MAX;
    // 转换单位圈->定点数
    //  判断角度是否超限
    if (TragetAngle > this->TAngle_MAX)
    {
        this->TAngle = this->TAngle_MAX;
    }
    else if (TragetAngle < this->TAngle_MIN)
    {
        this->TAngle = this->TAngle_MIN;
    }
    else
    {
        this->TAngle = TragetAngle;
    }
    return;
}
/**
 * @brief 设置目标速度
 * @param uint8_t IDX 电机ID
 * @param float TragetSpeed 目标速度，单位圈/s
 * @retval None
 */
void GM6020_TypeDef::SetTragetSpeed(float TragetSpeed)
{

    this->TSpeed = TragetSpeed;
}
/**
 * @brief 更新角度环
 * @param uint8_t IDX 电机ID
 * @retval None
 */
void GM6020_TypeDef::Update_PIDAngle()
{

    //================================================AnglePID============================
    // Er_Angle单位使用圈
    if (this->PIDAngleEnable)
    {
        // 计算dT,单位s,考虑溢出
        float dT_Angle;
        if (this->AngleLastTickus < usTickCNT)
        {
            dT_Angle = usTickCNT - this->AngleLastTickus;
        }
        else
        {
            // TIM4 CNT Reg 16bit，最大计数60ms
            dT_Angle = UINT16_MAX - this->AngleLastTickus + usTickCNT;
        }
        dT_Angle /= 1000000;
        // 单位转换
        float Er_Angle = (this->TAngle - this->SumAngle) / GM6020_EncoderAngle_MAX;
        PID_Caculate(&this->PIDAngle, dT_Angle, Er_Angle);
        this->SetTragetSpeed(this->PIDAngle.Output);
    }
    return;
}
/**
 * @brief 更新速度环
 * @param uint8_t IDX 电机ID
 * @retval None
 */
void GM6020_TypeDef::Update_PIDSpeed()
{
    //================================================SpeedPID============================
    // 计算dT,单位s,考虑一次溢出
    // TIM4 CNT Reg 16bit，最大计数60ms
    float dT_Speed = 0;
    if (this->SpeedLastTickus < usTickCNT)
    {
        dT_Speed = usTickCNT - this->SpeedLastTickus;
    }
    else
    {

        dT_Speed = UINT16_MAX - this->SpeedLastTickus + usTickCNT;
    }
    dT_Speed /= 1000000;
    this->SpeedLastTickus = usTickCNT;
    // 计算速度环(使用电流模式)
    // Er_Speed单位圈每秒
    float Er_Speed = (float)(this->TSpeed / GM6020_EncoderAngle_MAX - this->MotorFeedback.ActSpeed / 60);
    PID_Caculate(&this->PIDSpeed, dT_Speed, Er_Speed);
    return;
}
/**
 * @brief 处理电机报文数据
 * @param uint8_t IDX 电机ID
 * @retval None
 */
void GM6020_TypeDef::Update()
{

    // 判断角度是否突变
    if (abs_int(this->LastEncoderAngle
                - this->MotorFeedback.ActEncoderAngle)
        > GM6020_EncoderAngle_MAX / 2)
    {
        if (this->MotorFeedback.ActEncoderAngle < GM6020_EncoderAngle_MAX / 2 && this->MotorFeedback.ActSpeed > 0)
        {
            this->BigAngleNum += 1;
        }
        else if (this->MotorFeedback.ActEncoderAngle > GM6020_EncoderAngle_MAX / 2 && this->MotorFeedback.ActSpeed < 0)
        {
            this->BigAngleNum -= 1;
        }
    }
    // 计算求和角度
    this->SumAngle = this->BigAngleNum * GM6020_EncoderAngle_MAX + this->MotorFeedback.ActEncoderAngle;
    // 保存角度
    this->LastEncoderAngle = this->MotorFeedback.ActEncoderAngle;

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
        GM6020[RxMail.StdId - GM6020_BackMailBaseID - 1].WriteInfo(RxDATAs);
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
        GM6020[RxMail.StdId - GM6020_BackMailBaseID - 1].WriteInfo(RxDATAs);
    }
    // 更新对应电机PID
}
/**
 * @brief 获取电机结构体指针
 * @param uint8_t IDX 电机ID
 * @retval None
 */

GM6020_TypeDef* GM6020_GetInfop(uint8_t IDX)
{
    return &GM6020[(IDX - 1) % IDX_MAX];
}