#ifndef GM6020_SW_DEFINED
#define GM6020_SW_DEFINED

#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"

typedef struct{
  double Kp,Ki,Kd;
  int32_t Inter,Inter_MAX,l_Er;
  double Output;
  
}PID_HandleTypeDef;
void PID_Caculate(PID_HandleTypeDef PID_Handle,uint32_t dT,int32_t Er);
#endif