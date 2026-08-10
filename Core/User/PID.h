#ifndef PID_DEFINED
#define PID_DEFINED

#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"


typedef struct
{
    float Kp, Ki, Kd;
    float Er_Last, Er_MAX;
    float Inter, Inter_MAX, Inter_Step_Er_MAX;
    float Output, Output_Step_MAX, Output_MAX;
} PID_HandleTypeDef;
void PID_Caculate(PID_HandleTypeDef* pPid, double dT, double Er);
#endif