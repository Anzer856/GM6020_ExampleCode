#ifndef GM6020_HW_DEFINED
#define GM6020_HW_DEFINED

#include "can.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"
#include "PID.h"
#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204
#define  GM6020_Angle_MAX 8191
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

void GM6020_SetVoltage(uint8_t GM6020_ID, int16_t Voltage);
void GM6020_SetCurrent(uint8_t GM6020_ID, int16_t Current);
HAL_StatusTypeDef GM6020_SendVoltageConfig();
HAL_StatusTypeDef GM6020_SendCurrentConfig();
void GM6020_WriteInfo(uint8_t GM6020_ID, uint8_t* pDatas);
GM6020_TypeDef *GM6020_GetInfop(uint8_t GM6020_ID);
void GM6020_Init(uint8_t GM6020_ID);
#endif