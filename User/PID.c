#include "PID.h"
static float inline abs_Float(float N)
{
    return ((N) >= 0 ? (N) : (-(N)));
}

// 误差Er=Traget-ActualData,Output与ActualData正相关
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
    // 积分
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

    //计算输出
    float Output = pPid->Kp * Er + pPid->Ki * pPid->Inter + pPid->Kd * (Er - pPid->Er_Last) / dT;

    // 输出限幅
    if (Output > pPid->Output_MAX)
    {
        Output = pPid->Output_MAX;
    }
    else if (Output < -pPid->Output_MAX)
    {
        Output = -pPid->Output_MAX;
    }
    float Step = Output - pPid->Output;
    // 限制步幅
    if ((Step) > pPid->Step_MAX && Step >= 0)
    {
        Output = pPid->Output + pPid->Step_MAX;
    }
    else if ((Step) < -pPid->Step_MAX && Step < 0)
    {
        Output = pPid->Output - pPid->Step_MAX;
    }
    // 输出
    pPid->Er_Last = Er;
    pPid->Output  = Output;
    return Output;
}
void PID_Init(PID_HandleTypeDef* pPid)
{
    pPid->Kp             = 0;
    pPid->Ki             = 0;
    pPid->Kd             = 0;
    pPid->Er_Last        = 0;
    pPid->Er_MAX         = 0;
    pPid->Inter          = 0;
    pPid->Inter_MAX      = 0;
    pPid->Output         = 0;
    pPid->Step_Precision = 0;
    pPid->Step_MAX       = 1000;
    pPid->Output_MAX     = 1000;
    return;
}
