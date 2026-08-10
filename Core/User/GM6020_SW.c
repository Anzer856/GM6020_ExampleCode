#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"
#include <sys/types.h>

typedef struct{
  double Kp,Ki,Kd;
  int32_t Inter,Inter_MAX,l_Er;
  double Output;
  
}PID_HandleTypeDef;
void PID_Caculate(PID_HandleTypeDef PID_Handle,uint32_t dT,int32_t Er)
{
  PID_Handle.Output=Kp*Er+Ki*PID_Handle.Inter+Kd*(Er-l_Er);
  if((-Inter_MAX<PID_Handle.Inter+dT*Er)&&(PID_Handle.Inter<Inter_MAX-dT*Er))
  {
    PID_Handle.Inter+=dT*Er;
  }
  return 1;
}

