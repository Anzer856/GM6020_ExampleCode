#ifndef GM6020_HW_DEFINED
#define GM6020_HW_DEFINED

#include "can.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"

#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204
typedef struct
{

    int16_t Angle;
    int16_t Speed;
    int16_t ActCurrent;
    uint8_t Temperature;
} GM6020_INFOTypeDef;

void GM6020_SetVoltage(uint8_t GM6020_ID, int16_t Voltage);
void GM6020_SetCurrent(uint8_t GM6020_ID, int16_t Current);
HAL_StatusTypeDef GM6020_SendVoltageConfig();
HAL_StatusTypeDef GM6020_SendCurrentConfig();
void GM6020_WriteInfo(uint8_t GM6020_ID, uint8_t* pDatas);
GM6020_INFOTypeDef GM6020_GetInfo(uint8_t GM6020_ID);

#endif