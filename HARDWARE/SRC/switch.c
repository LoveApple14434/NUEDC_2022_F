#include "switch.h"

const Switch switches[3]={
    {.P1_Port=S1_P1_Port, .P1_Pin =S1_P1_Pin, .P2_Port=S1_P2_Port, .P2_Pin =S1_P2_Pin},
    {.P1_Port=S2_P1_Port, .P1_Pin =S2_P1_Pin, .P2_Port=S2_P2_Port, .P2_Pin =S2_P2_Pin},
    {.P1_Port=S3_P1_Port, .P1_Pin =S3_P1_Pin, .P2_Port=S3_P2_Port, .P2_Pin =S3_P2_Pin}
};

void SwitchInit()
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin=GPIO_PIN_10;
    GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull=GPIO_PULLUP;
    GPIO_InitStruct.Speed=GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct); //Pull up PG10 to disable IS62WV51216
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);
    
    for (int i=0; i<3; i+=1) {
        GPIO_InitTypeDef GPIO_InitStruct={0};
        GPIO_InitStruct.Pin=switches[i].P1_Pin;
        GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull=GPIO_PULLDOWN;
        GPIO_InitStruct.Speed=GPIO_SPEED_MEDIUM;
        HAL_GPIO_Init(switches[i].P1_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin=switches[i].P2_Pin;
        HAL_GPIO_Init(switches[i].P2_Port, &GPIO_InitStruct);
    }
}

uint8_t CheckSwitchState(Switch sw)
{
    uint8_t temp1=HAL_GPIO_ReadPin(sw.P1_Port, sw.P1_Pin);
    uint8_t temp2=HAL_GPIO_ReadPin(sw.P2_Port, sw.P2_Pin);
    return temp1^temp2+temp2&2;
}

void ChangeSwitchState(Switch sw, uint8_t state)
{
    HAL_GPIO_WritePin(sw.P1_Port, sw.P1_Pin, !((state&1u)^((state>>1)&1u)));
    HAL_GPIO_WritePin(sw.P2_Port, sw.P2_Pin, (state>>1)&1u);
    return;
}
