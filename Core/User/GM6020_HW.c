#include "stm32f1xx.h"
#include "can.h"
#include "stm32f1xx_hal_can.h"


#define GM6020_CANID_Group1 0x1FF
#define GM6020_CANID_Group2 0x2FF
#define GM6020_BackMailBaseID 0x204
typedef  struct{

  int16_t Angle;
  int16_t Speed;
  int16_t ActCurrent;
  uint8_t Temperature;
}GM6020_INFOTypeDef;

GM6020_INFOTypeDef GM6020_Infos[7];
uint8_t GM6020_VoltageDatas_Group1[8]={};
uint8_t GM6020_VoltageDatas_Group2[8]={};
uint8_t GM6020_CurrentDatas_Group1[8]={};
uint8_t GM6020_CurrentDatas_Group2[8]={};

void GM6020_SetVoltage(uint8_t GM6020_ID,int16_t Voltage)
{
  if(1<=GM6020_ID&&GM6020_ID<=4)
  {
    GM6020_VoltageDatas_Group1[(GM6020_ID-1)*2]=(uint8_t)(Voltage&0xFF00)>>8;
    GM6020_VoltageDatas_Group1[(GM6020_ID-1)*2+1]=(uint8_t)(Voltage&0x00FF);
  }
  else if (5<=GM6020_ID&&GM6020_ID<=7)
  {
    GM6020_VoltageDatas_Group2[(GM6020_ID-5)*2]=(uint8_t)(Voltage&0xFF00)>>8;
    GM6020_VoltageDatas_Group2[(GM6020_ID-5)*2+1]=(uint8_t)(Voltage&0x00FF);
  }
  return;
}
void GM6020_SetCurrent(uint8_t GM6020_ID,int16_t Current)
{
  if(1<=GM6020_ID&&GM6020_ID<=4)
  {
    GM6020_CurrentDatas_Group1[(GM6020_ID-1)*2]=(uint8_t)(Current&0xFF00)>>8;
    GM6020_CurrentDatas_Group1[(GM6020_ID-1)*2+1]=(uint8_t)(Current&0x00FF);
  }
  else if (5<=GM6020_ID&&GM6020_ID<=7)
  {
    GM6020_CurrentDatas_Group2[(GM6020_ID-5)*2]=(uint8_t)(Current&0xFF00)>>8;
    GM6020_CurrentDatas_Group2[(GM6020_ID-5)*2+1]=(uint8_t)(Current&0x00FF);
  }
  return;
}
HAL_StatusTypeDef GM6020_SendVoltageConfig()
{
  
  CAN_TxHeaderTypeDef TxMail;
  uint8_t *pTxDATAs;
  uint32_t Mailbox;
  TxMail.IDE=CAN_ID_STD;
  TxMail.RTR=CAN_RTR_DATA;
  TxMail.DLC=8;
  TxMail.ExtId=0x00;//Only use StdId

  
  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)<2)
  {
    return HAL_BUSY;
  }
  //Send VoltageDatas_Group
  TxMail.StdId=GM6020_CANID_Group1;
  pTxDATAs=GM6020_VoltageDatas_Group1;
  HAL_CAN_AddTxMessage(&hcan, &TxMail,pTxDATAs, &Mailbox);

  TxMail.StdId=GM6020_CANID_Group2;
  pTxDATAs=GM6020_VoltageDatas_Group2;
  HAL_CAN_AddTxMessage(&hcan, &TxMail,pTxDATAs, &Mailbox);
  return HAL_OK;
}
HAL_StatusTypeDef GM6020_SendCurrentConfig()
{
  
  CAN_TxHeaderTypeDef TxMail;
  uint8_t *pTxDATAs;
  uint32_t Mailbox;
  TxMail.IDE=CAN_ID_STD;
  TxMail.RTR=CAN_RTR_DATA;
  TxMail.DLC=8;
  TxMail.ExtId=0x00;//Only use StdId

  
  

  HAL_CAN_Start(&hcan);
  
  if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan)<2)
  {
    return HAL_BUSY;
  }
  //Send CurrentDatas_Group1
  TxMail.StdId=GM6020_CANID_Group1;
  pTxDATAs=GM6020_CurrentDatas_Group1;
  HAL_CAN_AddTxMessage(&hcan, &TxMail,pTxDATAs, &Mailbox);

  TxMail.StdId=GM6020_CANID_Group2;
  pTxDATAs=GM6020_CurrentDatas_Group2;
  HAL_CAN_AddTxMessage(&hcan, &TxMail,pTxDATAs, &Mailbox);
  return HAL_OK;
}
void GM6020_WriteInfo(uint8_t GM6020_ID,uint8_t *pDatas)
{
    
    if(GM6020_ID>7)
    {
        return;
    }
    //offset
    GM6020_ID-=1;
    GM6020_Infos[GM6020_ID].Angle=(uint16_t)pDatas[0]<<8;
    GM6020_Infos[GM6020_ID].Angle|=(uint16_t)pDatas[1];

    GM6020_Infos[GM6020_ID].Speed=(uint16_t)pDatas[2]<<8;
    GM6020_Infos[GM6020_ID].Speed|=(uint16_t)pDatas[3];

    GM6020_Infos[GM6020_ID].ActCurrent=(uint16_t)pDatas[4]<<8;
    GM6020_Infos[GM6020_ID].ActCurrent|=(uint16_t)pDatas[5];

    GM6020_Infos[GM6020_ID].Temperature=(uint16_t)pDatas[6];
    return ;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxMail;
    uint8_t RxDATAs[8]={};
    HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&RxMail, RxDATAs);
    //Mail from GM6020
    if((GM6020_BackMailBaseID+1)<=RxMail.StdId&&RxMail.StdId<=(GM6020_BackMailBaseID+7))
    {
        GM6020_WriteInfo(RxMail.StdId-GM6020_BackMailBaseID,RxDATAs);
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef RxMail;
    uint8_t RxDATAs[8]={};
    HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO1,&RxMail, RxDATAs);
    //Mail from GM6020
    if((GM6020_BackMailBaseID+1)<=RxMail.StdId&&RxMail.StdId<=(GM6020_BackMailBaseID+7))
    {
        GM6020_WriteInfo(RxMail.StdId-GM6020_BackMailBaseID,RxDATAs);
    }
}
GM6020_INFOTypeDef GM6020_GetInfo(uint8_t GM6020_ID)
{
    return GM6020_Infos[(GM6020_ID-1)%7];
}