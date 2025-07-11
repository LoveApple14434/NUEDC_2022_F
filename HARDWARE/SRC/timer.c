#include "timer.h"
#include "led.h"
#include "main.h"
#include "adc.h"
#include <string.h>
#include "AD9959.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//定时器中断驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/4/7
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

TIM_HandleTypeDef htim2;

uint32_t cnt=0, mem=0, base_freq=0;
extern adc_data adc_cache;

void Error_Handler(void){LED1=!LED1;}

/* TIM2 init function */
void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffffu;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */
  HAL_TIM_Base_Start(&htim2);
  /* USER CODE END TIM2_Init 2 */

}

extern struct data cache;
TIM_HandleTypeDef TIM3_Handler;      //定时器句柄 

//通用定时器3中断初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//这里使用的是定时器3!(定时器3挂在APB1上，时钟为HCLK/2)
void TIM3_Init(u16 arr,u16 psc)
{  
    TIM3_Handler.Instance=TIM3;                          //通用定时器3
    TIM3_Handler.Init.Prescaler=psc;                     //分频系数
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //向上计数器
    TIM3_Handler.Init.Period=arr;                        //自动装载值
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//时钟分频因子
    HAL_TIM_Base_Init(&TIM3_Handler);

    TIM_MasterConfigTypeDef master = {0};
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;  // 更新事件触发ADC
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM3_Handler, &master);
    
    //HAL_TIM_Base_Start(&TIM3_Handler); //使能定时器3
}

TIM_HandleTypeDef htim4;
void TIM4_Init(u16 arr,u16 psc)
{  
    htim4.Instance=TIM4;
    htim4.Init.Prescaler=psc;
    htim4.Init.Period=arr;
    htim4.Init.CounterMode=TIM_COUNTERMODE_UP;
    htim4.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim4);
    HAL_TIM_Base_Start_IT(&htim4);
}

TIM_HandleTypeDef htim14;
TIM_OC_InitTypeDef htim14_ch1;
void TIM14_Init(u16 arr, u16 psc)
{
    htim14.Instance=TIM14;
    htim14.Init.Prescaler=psc;
    htim14.Init.Period=arr;
    htim14.Init.CounterMode=TIM_COUNTERMODE_UP;
    htim14.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    htim14.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim14);

    htim14_ch1.OCMode=TIM_OCMODE_PWM1;
    htim14_ch1.OCPolarity=TIM_OCPOLARITY_LOW;
    htim14_ch1.Pulse=arr/2;
    HAL_TIM_PWM_ConfigChannel(&htim14, &htim14_ch1, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
}

//定时器底册驱动，开启时钟，设置中断优先级
//此函数会被HAL_TIM_Base_Init()函数调用
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(htim->Instance==TIM2)
    {
/* USER CODE BEGIN TIM2_MspInit 0 */

/* USER CODE END TIM2_MspInit 0 */
    /* TIM2 clock enable */
    __HAL_RCC_TIM2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PA0-WKUP     ------> TIM2_ETR
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    if(htim->Instance==TIM3)
	  {
		    __HAL_RCC_TIM3_CLK_ENABLE();            //使能TIM3时钟
		    HAL_NVIC_SetPriority(TIM3_IRQn,1,3);    //设置中断优先级，抢占优先级1，子优先级3
		    HAL_NVIC_EnableIRQ(TIM3_IRQn);          //开启ITM3中断   
	  }
    if(htim->Instance==TIM4)
    {
        __HAL_RCC_TIM4_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM4_IRQn,1,1);
        HAL_NVIC_EnableIRQ(TIM4_IRQn);
    }
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if(htim->Instance==TIM14)
    {
        __HAL_RCC_TIM14_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        GPIO_InitTypeDef GPIO_InitStruct={0};
        GPIO_InitStruct.Pin=PWM_Pin;
        GPIO_InitStruct.Pull=GPIO_PULLUP;
        GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate=GPIO_AF9_TIM14;
        GPIO_InitStruct.Mode=GPIO_MODE_AF_PP;
        HAL_GPIO_Init(PWM_GPIO_Port, &GPIO_InitStruct);
    }
}


//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM3_Handler);
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
    if(__HAL_TIM_GET_FLAG(&htim4, TIM_FLAG_UPDATE) != RESET)
    {
        __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE); // 手动清除标志
    }
}


//回调函数，定时器中断服务函数调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim==&htim4)
    {
      //LED0=!LED0;
      cnt=__HAL_TIM_GET_COUNTER(&htim2);
      if(cnt>mem)
        base_freq=(cnt-mem)*10;//*1.00003;
      mem=cnt;
    }
}
