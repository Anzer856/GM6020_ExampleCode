#include "can.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_can.h"
#include <sys/types.h>
#define abs_float(N) ((N) >= 0 ? (N) : (-(N)))


typedef struct
{
    float Kp, Ki, Kd;
    float Er_Last, Er_MAX;
    float Inter, Inter_MAX;
    float Output, Output_MAX;
} PID_HandleTypeDef;
//误差Er=Traget-ActualData,Output与ActualData正相关
float PID_Caculate(PID_HandleTypeDef* pPid, float dT, float Er)
{

    // 阻止输入误差过大
    if (Er < -pPid->Er_MAX)
    {
        Er = -pPid->Er_MAX;
    }
    else if (Er > pPid->Er_MAX)
    {
        Er = pPid->Er_MAX;
    }
    float Output = pPid->Kp * Er + pPid->Ki * pPid->Inter + pPid->Kd * (Er - pPid->Er_Last) / dT;
    // 误差大时候禁用I
    
    pPid->Inter += Er * dT;
    
    // 积分上限
    if (-pPid->Inter_MAX > pPid->Inter)
    {
        pPid->Inter = -pPid->Inter_MAX;
    }
    else if (pPid->Inter > pPid->Inter_MAX)
    {
        pPid->Inter = pPid->Inter_MAX;
    }

    // 输出限幅
    if (Output > pPid->Output_MAX)
    {
        Output = pPid->Output_MAX;
    }
    else if (Output < -pPid->Output_MAX)
    {
        Output = -pPid->Output_MAX;
    }
    // 输出
    pPid->Er_Last = Er;
    pPid->Output  = Output;
    return Output;
}
void PID_Init(PID_HandleTypeDef* pPid)
{
    pPid->Kp              = 2.2;
    pPid->Ki              = 1.6;
    pPid->Kd              = 0.011;
    pPid->Er_Last         = 0;
    pPid->Er_MAX          = 1000.0f;
    pPid->Inter           = 0;
    pPid->Inter_MAX       = 40.0f;
    pPid->Output          = 0;

    pPid->Output_MAX      = 200.0f;
    return;
}
