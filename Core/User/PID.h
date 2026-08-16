#ifndef PID_DEFINED
#define PID_DEFINED

#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"


typedef struct
{
    float Kp, Ki, Kd;
    float Er_Last, Er_MAX;
    float Inter, Inter_MAX;
    float Output, Output_MAX;
    float Step_MAX,Step_Precision;
} PID_HandleTypeDef;
float PID_Caculate(PID_HandleTypeDef* pPid, float dT, float Er);
void PID_Init(PID_HandleTypeDef* pPid);
#endif