#ifndef GM6020_HW_DEFINED
#define GM6020_HW_DEFINED

#include "PID.h"
#include "can.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"

#define GM6020_ID_MAX 7
#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204
#define GM6020_EncoderAngle_MAX 8191

#define GM6020_Current_BIN_MAX 16384
#define GM6020_Current_MAX 3
#define GM6020_Voltage_BIN_MAX 25000
#define GM6020_Voltage_MAX 24

#define usTickCNT (TIM4->CNT)

typedef struct
{
    int16_t ActEncoderAngle;
    int16_t ActSpeed;
    int16_t ActCurrent;
    uint8_t Temperature;
    uint8_t IsUpdated;
} GM6020_FeedbackTypeDef;
typedef struct
{
    uint8_t ID;
    uint8_t IsOK;

    GM6020_FeedbackTypeDef MotorFeedback;
    PID_HandleTypeDef PIDSpeed;
    PID_HandleTypeDef PIDAngle;

    uint8_t PIDAngleEnable;
    // 角度使用定点数，以编码器量程(GM6020_EncoderAngle_MAX)为一圈
    int32_t BigAngleNum; // 整圈数
    int32_t SumAngle;    // 总和角度
    int32_t TAngle, TAngle_MIN, TAngle_MAX;
    uint16_t LastEncoderAngle;
    float TSpeed, TSpeed_MIN, TSpeed_MAX;      //
    uint32_t SpeedLastTickus, AngleLastTickus,UpdateLastTickus; // 单位:us

} GM6020_TypeDef;

void GM6020_SetTragetAngle(uint8_t GM6020_ID, float TragetAngle);
void GM6020_SetTragetSpeed(uint8_t GM6020_ID, float TragetSpeed);
HAL_StatusTypeDef GM6020_SendVoltageConfig();
HAL_StatusTypeDef GM6020_SendCurrentConfig();
void GM6020_WriteInfo(uint8_t GM6020_ID, uint8_t* pDatas);
GM6020_TypeDef* GM6020_GetInfop(uint8_t GM6020_ID);
void GM6020_Init(uint8_t GM6020_ID);
void GM6020_Update(uint8_t GM6020_ID);
void GM6020_Update_PIDAngle(uint8_t GM6020_ID);
void GM6020_Update_PIDSpeed(uint8_t GM6020_ID);
#endif