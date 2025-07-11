#include "adc.h"
#include "delay.h"
#include "main.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//ADC驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/4/11
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

DMA_HandleTypeDef hdma_adc1={0};
ADC_HandleTypeDef ADC1_Handler={0};//ADC句柄
extern adc_data adc_cache;

//初始化ADC
//ch: ADC_channels 
//通道值 0~16取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
void MY_ADC_Init(void)
{ 
    ADC1_Handler.Instance=ADC1;
    ADC1_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;   //4分频，ADCCLK=PCLK2/4=90/4=22.5MHZ
    ADC1_Handler.Init.Resolution=ADC_RESOLUTION_12B;             //12位模式
    ADC1_Handler.Init.DataAlign=ADC_DATAALIGN_RIGHT;             //右对齐
    ADC1_Handler.Init.ScanConvMode=DISABLE;                      //非扫描模式
    ADC1_Handler.Init.EOCSelection=DISABLE;                      //关闭EOC中断
    ADC1_Handler.Init.ContinuousConvMode=DISABLE;                //关闭连续转换
    ADC1_Handler.Init.NbrOfConversion=1;                         //1个转换在规则序列中 也就是只转换规则序列1 
    ADC1_Handler.Init.DiscontinuousConvMode=DISABLE;             //禁止不连续采样模式
    ADC1_Handler.Init.NbrOfDiscConversion=0;                     //不连续采样通道数为0
    ADC1_Handler.Init.ExternalTrigConv=ADC_EXTERNALTRIGCONV_T3_TRGO;       //TIM3
    ADC1_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_RISING;//rising edge
    ADC1_Handler.Init.DMAContinuousRequests=ENABLE;             //DMA请求
    //HAL_ADC_Init(ADC_HandleTypeDef *hadc);                    等最后再执行

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_adc1.Instance = DMA2_Stream0;  // 使用DMA2 Stream0（请根据芯片参考手册选择正确的流）
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;  // 通道0（根据参考手册选择）
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;  // 外设到内存
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;  // 外设地址不递增
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;  // 内存地址递增
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   // 外设数据宽度16位
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;          // 内存数据宽度32位
    hdma_adc1.Init.Mode = DMA_CIRCULAR;  // 循环模式
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;  // 高优先级
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_adc1.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    hdma_adc1.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_adc1.Init.PeriphBurst = DMA_PBURST_SINGLE;
    HAL_DMA_Init(&hdma_adc1);
    __HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_TC);  // 传输完成中断
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);  // 根据实际DMA流修改
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);         // 使能DMA中断
    __HAL_LINKDMA(&ADC1_Handler, DMA_Handle, hdma_adc1);

    ADC_ChannelConfTypeDef chan = {0};
    chan.Channel = ADC_CHANNEL_5;
    chan.Rank = 1;
    chan.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&ADC1_Handler, &chan);

    HAL_ADC_Init(&ADC1_Handler);                                 //初始化 

    //HAL_ADC_Start_DMA(&ADC1_Handler, (uint32_t*)adc_cache.cache, ADClen);
}

//ADC底层驱动，引脚配置，时钟使能
//此函数会被HAL_ADC_Init()调用
//hadc:ADC句柄
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_ADC1_CLK_ENABLE();            //使能ADC1时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();			//开启GPIOA时钟
	
    GPIO_Initure.Pin=GPIO_PIN_5;            //PA5
    GPIO_Initure.Mode=GPIO_MODE_ANALOG;     //模拟
    GPIO_Initure.Pull=GPIO_NOPULL;          //不带上下拉
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
}

//获得ADC值
//ch: 通道值 0~16，取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
//返回值:转换结果
u16 Get_Adc(u32 ch)   
{
    ADC_ChannelConfTypeDef ADC1_ChanConf;
    
    ADC1_ChanConf.Channel=ch;                                   //通道
    ADC1_ChanConf.Rank=1;                                       //第1个序列，序列1
    ADC1_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;        //采样时间
    ADC1_ChanConf.Offset=0;                 
    HAL_ADC_ConfigChannel(&ADC1_Handler,&ADC1_ChanConf);        //通道配置
	
    HAL_ADC_Start(&ADC1_Handler);                               //开启ADC
	
    HAL_ADC_PollForConversion(&ADC1_Handler,10);                //轮询转换
 
	return (u16)HAL_ADC_GetValue(&ADC1_Handler);	        //返回最近一次ADC1规则组的转换结果
}
//获取指定通道的转换值，取times次,然后平均 
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(u32 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 
