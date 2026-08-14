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
} PID_HandleTypeDef;
double PID_Caculate(PID_HandleTypeDef* pPid, double dT, double Er);
void PID_Init(PID_HandleTypeDef* pPid);
#endif