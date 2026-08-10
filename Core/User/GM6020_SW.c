#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"
#include <sys/types.h>

inline double ABS(double N) 
{
  return ((N)>=0?(N):(-(N)));
}

typedef struct{
  float Kp,Ki,Kd;
  float Er_Last,Er_MAX;

  float Inter,Inter_MAX,Inter_Step_Er_MAX;

  float Output,Output_Step_MAX,Output_MAX;
}PID_HandleTypeDef;
void PID_Caculate(PID_HandleTypeDef *pPid,double dT,double Er)
{
  //阻止输入误差过大
  if(Er<-pPid->Er_MAX)
  {
    Er=-pPid->Er_MAX;
  }
  else if(Er>pPid->Er_MAX)
  {
    Er=pPid->Er_MAX;
  }
  double Output=pPid->Kp*Er
        +pPid->Ki*pPid->Inter
        +pPid->Kd*(Er-pPid->Er_Last)/dT;
  
  //误差大时候禁用I
  if(ABS(Er)>=pPid->Inter_Step_Er_MAX)
  {
    pPid->Inter+=0;
  }
  //执行器满时禁用I
  else if(ABS(Output)>=pPid->Output_MAX)
  {
    pPid->Inter+=0;
  }
  else 
  {
    pPid->Inter+=Er*dT;
  }
  //积分上限
  if(-pPid->Inter_MAX>pPid->Inter)
  {
    pPid->Inter=-pPid->Inter_MAX;
  }
  else if(pPid->Inter>pPid->Inter_MAX)
  {
    pPid->Inter=pPid->Inter_MAX;
  }
  
  
  
  
  
  
  
  //输出限幅
  if(Output>pPid->Output_MAX)
  {
    Output=pPid->Output_MAX;
  }
  else if(Output<-pPid->Output_MAX)
  {
    Output=-pPid->Output_MAX;
  }
  //输出
  pPid->Er_Last=Er;
  pPid->Output=Output;
  return ;
}
void PID_Init(PID_HandleTypeDef *pPid)
{
  pPid->Kp = 2.2;
	pPid->Ki = 1.6;
	pPid->Kd = 0.011;
	pPid->Er_Last = 0;
	pPid->Er_MAX = 1000.0f;
	pPid->Inter = 0;
	pPid->Inter_MAX = 40.0f;
	pPid->Output = 0;
	pPid->Output_Step_MAX = 100.0f;
	pPid->Output_MAX = 200.0f;
  return;
}
