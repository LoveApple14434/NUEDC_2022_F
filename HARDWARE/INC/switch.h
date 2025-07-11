#ifndef __SWITCH_H
#define __SWITCH_H

#include "sys.h"

#define S1_P1_Port  GPIOG
#define S1_P1_Pin   GPIO_PIN_1
#define S1_P2_Port  GPIOB
#define S1_P2_Pin   GPIO_PIN_13
#define S2_P1_Port  GPIOF
#define S2_P1_Pin   GPIO_PIN_15
#define S2_P2_Port  GPIOG
#define S2_P2_Pin   GPIO_PIN_0
#define S3_P1_Port  GPIOF
#define S3_P1_Pin   GPIO_PIN_13
#define S3_P2_Port  GPIOF
#define S3_P2_Pin   GPIO_PIN_14

typedef struct{
    GPIO_TypeDef* P1_Port;
    uint16_t P1_Pin;
    GPIO_TypeDef* P2_Port;
    uint16_t P2_Pin;
} Switch;

void SwitchInit(void);
uint8_t CheckSwitchState(Switch);
void ChangeSwitchState(Switch,uint8_t);

#endif
