#include "os_cpu.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "includes.h"
#include "AD9959.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "touch.h"
#include "lcd.h"
#include "rfft.h"
#include "adc.h"
#include "timer.h"
#include "key.h"
#include "led.h"
#include "switch.h"

extern const Switch switches[3];
#define Switch_ADC	switches[0]
#define Switch_mlt	switches[2]
#define Swch_ADC_env		4
#define Swch_ADC_am			1
#define Swch_ADC_fm			2
#define Swch_mlt_base		3
#define Swch_mlt_am			1
#define Swch_mlt_fm			4

#define kHz 1000
#define MHz 1000000

#define AM_DDS_CHANNEL	0
#define FM_DDS_CHANNEL	0

#define START_TASK_PRIO			10  
#define START_STK_SIZE			128
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata);


#define DEV_TASK_PRIO         6
#define DEV_STK_SIZE          128
OS_STK DEV_TASK_STK[DEV_STK_SIZE];
void dev_task(void *pdata);
uint8_t DrawTickBox(uint16_t, uint16_t, uint16_t, uint8_t, u16, u16);


#define KEY_TASK_PRIO         	1
#define KEY_STK_SIZE			128
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
void key_task(void *pdata);

#define AM_TASK_PRIO			4
#define AM_STK_SIZE				1024
OS_STK AM_TASK_STK[AM_STK_SIZE];
void am_task(void *pdata);

#define DDS_SWEEP_TASK_PRIO		7
#define DDS_SWEEP_STK_SIZE		128
OS_STK DDS_SWEEP_TASK_STK[DDS_SWEEP_STK_SIZE];
void dds_sweep_task(void *pdata);

#define FM_TASK_PRIO			3
#define FM_STK_SIZE				1024
OS_STK	FM_TASK_STK[FM_STK_SIZE];
void fm_task(void *pdata);

#define JUDGE_TASK_PRIO			2
#define JUDGE_STK_SIZE			128
OS_STK	JUDGE_TASK_STK[JUDGE_STK_SIZE];
void judge_task(void *pdata);
uint8_t is_am=2;


void input_task(void *pdata);
#define INPUT_FREQ1			1
#define INPUT_FREQ2			2
#define INPUT_STEP			3
#define INPUT_AMP			4
void dds_task(void *pdata);

adc_data adc_cache={
	.cache={0},
	.pos=0,
	.save={0}
};
double temp_fft[1024]={0};
uint8_t adc_usage;

DDS_data DDS_data_stack={
	.amp={150,150,150,150},
	.channel_state={0,0,0,0},
	.controled_channel=0,
	.freq1={1*MHz,1*MHz,1*MHz,1*MHz},
	.freq2={100*MHz,100*MHz,100*MHz,100*MHz},
	.step={1*MHz,1*MHz,1*MHz,1*MHz},
	.now_freq={1*MHz, 1*MHz, 1*MHz, 1*MHz}
};
#define CHANNEL_STATE_OFF 0
#define CHANNEL_STATE_SINGLE 1
#define CHANNEL_STATE_SWEEP 2

extern uint32_t mem, cnt;
extern uint32_t base_freq;
uint32_t am_base_freq, fm_base_freq;

uint8_t down=0;

uint32_t adjust_filter(uint32_t target);
uint32_t adjust_adc(uint32_t target);
#define abs_minus(a, b)	(((a)>(b))?((a)-(b)):((b)-(a)))

int main(void)
{ 
    HAL_Init();                    	
    Stm32_Clock_Init(336,8,2,7);  	
	delay_init(168);               	
	uart_init(115200);              
	LCD_Init();				
	//LED_Init();		
	tp_dev.init();				    
	Init_AD9959();		
	MY_ADC_Init();
	KEY_Init();
	TIM3_Init(25-1, 2-1);		//40*42kHz
	TIM4_Init(5000-1, 1680-1);	//10Hz
	MX_TIM2_Init();							//for base frequency detect
	TIM14_Init(42-1, 2-1);		//for filter, default 1MHz, div 50 is 20kHz
	LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	SwitchInit();
	OSInit();                       
    OSTaskCreateExt((void(*)(void*) )start_task,                
                    (void*          )0,                         
                    (OS_STK*        )&START_TASK_STK[START_STK_SIZE-1],
                    (INT8U          )START_TASK_PRIO,           
                    (INT16U         )START_TASK_PRIO,           
                    (OS_STK*        )&START_TASK_STK[0],        
                    (INT32U         )START_STK_SIZE,            
                    (void*          )0,                         
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSStart(); 
}


void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	pdata=pdata;
	OSStatInit();  
	
	OS_ENTER_CRITICAL();  
	
    OSTaskCreateExt((void(*)(void*) )dev_task,                 
                    (void*          )0,
                    (OS_STK*        )&DEV_TASK_STK[DEV_STK_SIZE-1],
                    (INT8U          )DEV_TASK_PRIO,          
                    (INT16U         )DEV_TASK_PRIO,            
                    (OS_STK*        )&DEV_TASK_STK[0],         
                    (INT32U         )DEV_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSTaskCreateExt((void(*)(void*) )dds_sweep_task,                 
                    (void*          )&DDS_data_stack,
                    (OS_STK*        )&DDS_SWEEP_TASK_STK[DDS_SWEEP_STK_SIZE-1],
                    (INT8U          )DDS_SWEEP_TASK_PRIO,          
                    (INT16U         )DDS_SWEEP_TASK_PRIO,            
                    (OS_STK*        )&DDS_SWEEP_TASK_STK[0],         
                    (INT32U         )DDS_SWEEP_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSTaskCreateExt((void(*)(void*) )am_task,                 
                    (void*          )0,
                    (OS_STK*        )&AM_TASK_STK[AM_STK_SIZE-1],
                    (INT8U          )AM_TASK_PRIO,          
                    (INT16U         )AM_TASK_PRIO,            
                    (OS_STK*        )&AM_TASK_STK[0],         
                    (INT32U         )AM_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSTaskCreateExt((void(*)(void*) )key_task,                 
                    (void*          )0,
                    (OS_STK*        )&KEY_TASK_STK[KEY_STK_SIZE-1],
                    (INT8U          )KEY_TASK_PRIO,          
                    (INT16U         )KEY_TASK_PRIO,            
                    (OS_STK*        )&KEY_TASK_STK[0],         
                    (INT32U         )KEY_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSTaskCreateExt((void(*)(void*) )fm_task,                 
                    (void*          )0,
                    (OS_STK*        )&FM_TASK_STK[FM_STK_SIZE-1],
                    (INT8U          )FM_TASK_PRIO,          
                    (INT16U         )FM_TASK_PRIO,            
                    (OS_STK*        )&FM_TASK_STK[0],         
                    (INT32U         )FM_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	OSTaskCreateExt((void(*)(void*) )judge_task,                 
                    (void*          )0,
                    (OS_STK*        )&JUDGE_TASK_STK[JUDGE_STK_SIZE-1],
                    (INT8U          )JUDGE_TASK_PRIO,          
                    (INT16U         )JUDGE_TASK_PRIO,            
                    (OS_STK*        )&JUDGE_TASK_STK[0],         
                    (INT32U         )JUDGE_STK_SIZE,            
                    (void*          )0,                           
                    (INT16U         )OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR|OS_TASK_OPT_SAVE_FP);
	//OSTaskSuspend(DEV_TASK_PRIO);
	OSTaskSuspend(AM_TASK_PRIO);
	OSTaskSuspend(FM_TASK_PRIO);
	//OSTaskSuspend(JUDGE_TASK_PRIO);
	OSTaskSuspend(DEV_TASK_PRIO);
	OSTaskSuspend(KEY_TASK_PRIO);
    OS_EXIT_CRITICAL();             
	OSTaskSuspend(START_TASK_PRIO); 
}

void judge_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	uint16_t max, min;
	uint16_t adc_delta_gate=500;
	while (1) {
		delay_ms(50);
		while (adc_usage!=0)
			delay_ms(5);
		adc_usage=1;
		ChangeSwitchState(Switch_ADC,Swch_ADC_env);
		adjust_adc(40*kHz);
		HAL_TIM_Base_Start(&htim3);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_cache.cache, ADClen);
		OS_ENTER_CRITICAL();
		// LCD_ShowNum(0, 160, adc_usage, 1, 32);
		LCD_ShowString(0, lcddev.height-40, 400, 32, 32, (u8*)"Judging Envelop...      ");
		OS_EXIT_CRITICAL();
		delay_ms(100);
		HAL_ADC_Stop_DMA(&hadc1);
		HAL_TIM_Base_Stop(&htim3);
		max=0;min=65535;
		for (int i=0; i<ADClen; i+=1) {
			if (adc_cache.save[i]>max)
				max=adc_cache.save[i];
			if (adc_cache.save[i]<min)
				min=adc_cache.save[i];
		}
		adc_usage=0;
		LCD_ShowNum(20, 160, max, 4, 32);
		LCD_ShowNum(120, 160, min, 4, 32);
		if(max-min>adc_delta_gate)
		//over gate, judge as AM
		{
			if (is_am!=1) {
				LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
				LCD_ShowString(lcddev.width-80, 0, 80, 32, 32, (u8*)"   AM");
				is_am=1;
				OSTaskSuspend(FM_TASK_PRIO);
				OSTaskResume(AM_TASK_PRIO);
			}
			
		}
		else
		//not over gate, judge as FM or CW
		{
			if (is_am!=0) {
				LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
				LCD_ShowString(lcddev.width-80, 0, 80, 32, 32, (u8*)"CW/FM");
				is_am=0;
				OSTaskSuspend(AM_TASK_PRIO);
				OSTaskResume(FM_TASK_PRIO);
			}
			
		}
		LCD_ShowNum(0, 400, is_am, 1, 32);
		delay_ms(5000);
	}
}

void key_task(void *pdata)
{
	OS_CPU_SR cpu_sr=0;
	uint8_t key;
	while (1) 
	{
		delay_ms(2);
		key=KEY_Scan(0);
		if(key)
		{
			switch (key) {
				case KEY0_PRES:
					OS_ENTER_CRITICAL();
					OSTaskSuspend(FM_TASK_PRIO);
					OSTaskSuspend(AM_TASK_PRIO);
					OSTaskSuspend(JUDGE_TASK_PRIO);
					LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
					for (u8 i=0; i<4; i+=1)
						DrawTickBox(
						lcddev.width*i/4+lcddev.width/16, 
						10, 
						lcddev.width/10, 
						i==DDS_data_stack.controled_channel, 
						BLACK, 
						RED
						);
					OSTaskResume(DEV_TASK_PRIO);
					OS_EXIT_CRITICAL();
					break;
				case KEY2_PRES:
					OS_ENTER_CRITICAL();
					OSTaskSuspend(DEV_TASK_PRIO);
					LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
					OSTaskResume(JUDGE_TASK_PRIO);
					OS_EXIT_CRITICAL();
					break;
				default:
					break;
			}
		}
	}
}

uint32_t adjust_filter(uint32_t target)
{
	HAL_TIM_PWM_Stop(&htim14, TIM_CHANNEL_1);
	target=42*MHz/(target*50);
	__HAL_TIM_SET_AUTORELOAD(&htim14, target-1);
	__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, target/2);
	HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);
	delay_ms(10);
	return 42*MHz/(target*50);
}

uint32_t adjust_adc(uint32_t target)
{
	HAL_TIM_Base_Stop(&TIM3_Handler);
	adjust_filter(target/2);
	target=42*MHz/target;
	__HAL_TIM_SET_AUTORELOAD(&TIM3_Handler, target-1);
	delay_ms(20);
	return 42*MHz/target;
	//sometimes adc rate can't be exactly the target, so return the actual adc_rate to update it.
}

//Draw a tick box. Just a still image, not interactive.
//
//params: 	x: 		tick box start point x
//		  	y: 		tick box start point y
//        	a: 		length of the square box. should be bigger than 5
//			ticked: 0 for not ticked, else for ticked
//        	backColor: the color of the box
//        	tickColor: the color of the ticked part
//
//return: 	0 for successfully drawed
// 			1 for out of drawing range(according to lcddev and leave 5 space for left and right)
//          2 for len fault(should be bigger than 5)
uint8_t DrawTickBox(uint16_t x, uint16_t y, uint16_t a, uint8_t ticked, u16 backColor, u16 tickColor)
{
	if(
		x<0 || y<0 ||
		x+a>lcddev.width || y+a>lcddev.height
	) return 1;
	else if(a<5) return 2;
	else
	{
		LCD_Fill(x, y, x+a, y+a, backColor);
		if(ticked)
			LCD_Fill(x+2, y+2, x+a-2, y+a-2, tickColor);
		return 0;
	}
}

void fm_task(void* pdata)
{
	OS_CPU_SR cpu_sr=0;
	double tmax, tmin, fmax, avg;
	uint16_t fmax_p;
	uint8_t max_cnt, min_cnt, arror;
	uint16_t mod_freq;
	uint32_t adc_rate=36*kHz;
	double mod_depth=0;
	adc_rate=adjust_adc(adc_rate);
	uint8_t key=0;
	uint8_t round_cnt=0;
	uint8_t success_cnt=0;
	uint32_t dds_rate;
	while (1) {
		if (round_cnt%50==0) {
			success_cnt=0;
			round_cnt=0;
			for (int i=0; i<25; i+=1) {
				dds_rate=38*MHz-(uint32_t)i*MHz;
				DDS_data_stack.freq1[0]=dds_rate;
				DDS_data_stack.channel_state[0]=CHANNEL_STATE_SINGLE;
				dds_task((DDS_data*)&DDS_data_stack);
				// LCD_ShowNum(0, 240, dds_rate, 9, 32);
				ChangeSwitchState(Switch_mlt, Swch_mlt_base);
				LCD_ShowString(0, lcddev.height-40, 400, 32, 32, (u8*)"Counting Frequency...   ");
				delay_ms(50);
				HAL_TIM_Base_Start_IT(&htim4);
				delay_ms(300);
				HAL_TIM_Base_Stop_IT(&htim4);
				if (base_freq>3.8*MHz && base_freq<8.2*MHz) {
					uint32_t temp=dds_rate-base_freq;
					// LCD_ShowNum(160, 500, temp, 9, 32);
					// LCD_ShowNum(160, 460, fm_base_freq, 9, 32);
					if (success_cnt==0) {
						fm_base_freq=temp;
						success_cnt=1;
					}
					else if (abs_minus(fm_base_freq, temp)<250*kHz) {
						// LCD_ShowNum(160, 540, abs_minus(fm_base_freq, temp), 9, 32);
						fm_base_freq=temp;
						success_cnt+=1;
					}
					else {
						success_cnt=1;
						fm_base_freq=temp;
					}
					// LCD_ShowNum(120, 500, success_cnt, 1, 32);
					if (success_cnt==3) {
						break;
					}
				}
				else {
					success_cnt=0;
				}
				// LCD_ShowString(0, 0, 160, 32, 32, (u8*)"waiting");
				// while (key!=KEY0_PRES) {
				// 	key=KEY_Scan(0);
				// }
				// key=0;
				// LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
			}
			uint32_t temp2=fm_base_freq-fm_base_freq%(500*kHz);
			if (fm_base_freq-temp2>250*kHz) 
				temp2+=500*kHz;
			fm_base_freq=temp2; 
		}
		round_cnt+=1;
		avg=0;
		delay_ms(20);
		while (adc_usage!=0) 
			delay_ms(5);
		adc_usage=3;
		adc_rate=adjust_adc(adc_rate);
		ChangeSwitchState(Switch_ADC, (uint8_t)Swch_ADC_fm);
		LCD_ShowString(0, lcddev.height-40, 400, 32, 32, (u8*)"Measuring CW/FM...      ");
		DDS_data_stack.freq1[0]=fm_base_freq+10.65*MHz;
		dds_task((DDS_data*)&DDS_data_stack);
		ChangeSwitchState(Switch_mlt, Swch_mlt_fm);
		// HAL_GPIO_WritePin(Switch_mlt.P1_Port, Switch_mlt.P1_Pin, 1);
		// HAL_GPIO_WritePin(Switch_mlt.P2_Port, Switch_mlt.P2_Pin, 0);
		// HAL_GPIO_WritePin(Switch_ADC.P1_Port, Switch_ADC.P1_Pin, 1);
		// HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, 1);
		// LCD_ShowNum(0, 400, HAL_GPIO_ReadPin(Switch_ADC.P1_Port, Switch_ADC.P1_Pin), 1, 32);
		// LCD_ShowNum(20, 400, HAL_GPIO_ReadPin(Switch_ADC.P2_Port, Switch_ADC.P2_Pin), 1, 32);
		// LCD_ShowNum(0, 440, HAL_GPIO_ReadPin(Switch_mlt.P1_Port, Switch_mlt.P1_Pin), 1, 32);
		// LCD_ShowNum(20, 440, HAL_GPIO_ReadPin(Switch_mlt.P2_Port, Switch_mlt.P2_Pin), 1, 32);
		delay_ms(100);
		HAL_TIM_Base_Start(&htim3);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc_cache.cache, ADClen);
		// LCD_ShowNum(0, 160, adc_usage, 1, 32);
		delay_ms(100);
		HAL_ADC_Stop_DMA(&hadc1);
		HAL_TIM_Base_Stop(&htim3);
		for (int i=0; i<ADClen; i+=1) {
			temp_fft[i]=(double)adc_cache.save[i];
			avg+=temp_fft[i];
		}
		adc_usage=0;
		avg/=ADClen;
		for (int i=0; i<ADClen; i+=1) {
			temp_fft[i]-=avg;
		}
		//copy adc data and remove DC
		tmax=0; tmin=0;
		max_cnt=0; min_cnt=0;
		arror=temp_fft[1]>temp_fft[0]?1:0;
		for (int i=1; i<ADClen; i+=1) {
			if (temp_fft[i]-temp_fft[i-1]>5 && arror==0) {
				arror=1;
				tmin=(tmin*min_cnt+temp_fft[i-1])/(min_cnt+1);
				min_cnt++;
			}
			if (temp_fft[i]-temp_fft[i-1]<-5 && arror==1) {
				arror=0;
				tmax=(tmax*max_cnt+temp_fft[i-1])/(max_cnt+1);
				max_cnt++;
			}
		}
		if (tmax-tmin>40) 
			LCD_ShowString(lcddev.width-80, 0, 80, 32, 32, (u8*)"   FM");
		else
			LCD_ShowString(lcddev.width-80, 0, 80, 32, 32, (u8*)"   CW");
		LCD_ShowFloat(0, 160, tmax+10000, 5, 4, 16);
		LCD_ShowFloat(240, 160, tmin+10000, 5, 4, 16);
		LCD_ShowNum(0, 180, max_cnt, 3, 16);
		LCD_ShowNum(200, 180, min_cnt, 3, 16);
		rfft(temp_fft, ADClen);
		uint16_t mem_p=fmax_p;
		fmax=temp_fft[0]; fmax_p=0;
		for (int i=1; i<ADClen/2; i+=1) {
			temp_fft[i]=sqr_pls(temp_fft[i], temp_fft[ADClen-i]);
			if(temp_fft[i]>fmax)
			{
				fmax_p=i;
				fmax=temp_fft[i];
			}
		}
		if(mem_p!=fmax_p) mod_depth=0;
		//mod_depth=(tmax-tmin)/fmax_p;
		LCD_ShowFloat(0, 200, mod_depth+100, 3, 4, 32);
		double a[]={0.0336,0.0455,0.0198,0.0455,0.0582,0.0754,0.1114,0.0925};
		double b[]={0.0633,0.277,0.5915,0.5318,0.5736,0.5294,0.392,0.5434};
		double c[]={0.406,0.175,-0.021,0.133,0.126,0.196,0.328,0.132};
		uint8_t temp=(uint8_t)((fmax_p)*(1.0*adc_rate/kHz)/ADClen+0.5-3);
		if(temp>200) temp=0;
		else if(temp>7) temp=7;
		if(mod_depth+100>100.2)
			mod_depth=((-b[temp]+sqrt(b[temp]*b[temp]-4*a[temp]*(c[temp]-(tmax-tmin)/fmax_p)))/(2*a[temp]) + mod_depth*3)/4;
		else
			mod_depth=(-b[temp]+sqrt(b[temp]*b[temp]-4*a[temp]*(c[temp]-(tmax-tmin)/fmax_p)))/(2*a[temp]);
		LCD_ShowString(0, 0, 160, 32, 32, (u8*)"Base Freq:");
		LCD_ShowNum(160, 0, fm_base_freq, 9, 32);
		if (tmax-tmin>40) {
			LCD_ShowString(0, 40, 160, 32, 32, (u8*)"Mod Freq: ");
			LCD_ShowNum(160, 40, fmax_p*adc_rate/ADClen, 5, 32);
			LCD_ShowString(0, 80, 160, 32, 32, (u8*)"Mod Depth:");
			LCD_ShowFloat(160, 80, mod_depth, 3, 4, 32);
		}
		else 
			LCD_Fill(0, 40, lcddev.width, 200, GRED);
		// LCD_ShowNum(0, 300, (u32)temp, 4, 32);
		// key=0;
		// while (key!=KEY0_PRES) {
		// 	key=KEY_Scan(0);
		// }
		// LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	}
}

void am_task(void* pdata)
{
	OS_CPU_SR cpu_sr=0;
	//uint16_t time_ox=5;
	//uint16_t time_oy=lcddev.height/2-5;
	//no need to draw time zone wave
	uint16_t width	=lcddev.width-10;
	uint16_t height	=lcddev.height/2-10;
	uint16_t freq_ox=5;
	uint16_t freq_oy=lcddev.height-45;
	uint16_t prev_x, x, prev_y, y;
	double min, max, range, max2, max3, avg;
	uint16_t max_p, max2_p, max3_p;
	double am;//调幅指数
	int am_freq=15*kHz;
	uint32_t adc_rate=300*kHz;
	uint32_t dds_rate;
	adc_rate=adjust_adc(adc_rate);
	uint8_t key;
	HAL_TIM_Base_Stop_IT(&htim4);
	uint8_t round_cnt=0;
	uint8_t success_cnt=0;	//continious seccess rounds in Base Freq detection
	while(1){
		// round_cnt+=1;
		if (round_cnt%10==0) {
			round_cnt=0;
			success_cnt=0;
			for (int i=0; i<25; i+=1) {
				dds_rate=38*MHz-(uint32_t)i*MHz;
				DDS_data_stack.freq1[0]=dds_rate;
				DDS_data_stack.channel_state[0]=CHANNEL_STATE_SINGLE;
				dds_task((DDS_data*)&DDS_data_stack);
				LCD_ShowNum(0, 240, dds_rate, 9, 32);
				ChangeSwitchState(Switch_mlt, Swch_mlt_base);
				LCD_ShowString(0, lcddev.height-40, 400, 32, 32, (u8*)"Counting Frequency...   ");
				delay_ms(50);
				HAL_TIM_Base_Start_IT(&htim4);
				delay_ms(300);
				HAL_TIM_Base_Stop_IT(&htim4);
				if (base_freq>3.8*MHz && base_freq<8.2*MHz) {
					uint32_t temp=dds_rate-base_freq;
					LCD_ShowNum(0, 500, temp, 9, 32);
					if (success_cnt==0) {
						am_base_freq=temp;
						success_cnt=1;
					}
					else if (abs_minus(am_base_freq, temp)<10*kHz) {
						LCD_ShowNum(0, 500, abs_minus(am_base_freq,temp), 9, 32);
						LCD_ShowNum(200, 500, (abs_minus(am_base_freq,temp)>10*kHz)?123:321, 3, 32);
						LCD_ShowNum(300, 500, 10*kHz, 6, 32);
						am_base_freq=temp;
						success_cnt+=1;
						LCD_ShowNum(00, 540, success_cnt, 1, 32);
					}
					else {
						success_cnt=1;
						am_base_freq=temp;
					}
					if (success_cnt==3) {
						break;
					}
				}
				// LCD_ShowString(0, 0, 160, 32, 32, (u8*)"waiting");
				// while (key!=KEY0_PRES) {
				// 	key=KEY_Scan(0);
				// }
				// key=0;
				// LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
			}
			// LCD_ShowString(0, 0, 160, 32, 32, (u8*)"waiting");
			// key=0;
			// while (key!=KEY0_PRES) {
			// 	key=KEY_Scan(0);
			// }
			// LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
			dds_rate=am_base_freq-80*kHz;	
			DDS_data_stack.freq1[AM_DDS_CHANNEL]=dds_rate;
			dds_task((DDS_data*)&DDS_data_stack);
		}
		round_cnt+=1;
		dds_task((DDS_data*)&DDS_data_stack);
		delay_ms(100);
		avg=0;
		while (adc_usage!=0) 
			delay_ms(5);
		adc_usage=2;
		adc_rate=adjust_adc(adc_rate);
		ChangeSwitchState(Switch_ADC,Swch_ADC_am);
		ChangeSwitchState(Switch_mlt, Swch_mlt_am);
		LCD_ShowString(0, lcddev.height-40, 400, 32, 32, (u8*)"Measuring AM...         ");
		HAL_TIM_Base_Start(&htim3);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*) adc_cache.cache, ADClen);
		//LCD_ShowString(0, 160, 160, 32, 32, (u8*)"AM ADCing ");
		// LCD_ShowNum(0, 160, adc_usage, 1, 32);
		delay_ms(100);
		HAL_ADC_Stop_DMA(&hadc1);
		HAL_TIM_Base_Stop(&htim3);
		for (int i=0; i<ADClen; i+=1) {
			temp_fft[i]=(double)adc_cache.save[i];
			avg+=temp_fft[i];
		}
		adc_usage=0;
		avg/=ADClen;
		for (int i=0; i<ADClen; i+=1) {
			temp_fft[i]-=avg;
		}
		//copy adc saved data to temp_fft and remove DC
		rfft(temp_fft, ADClen);
		//do rfft, mind that adclen should be 2^n, and that temp_fft would be directly changed
		max=temp_fft[0]; min=temp_fft[0];
		for (int i=1; i<ADClen/2; i+=1) {
			temp_fft[i]=sqr_pls(temp_fft[i], temp_fft[ADClen-i]);
			if (temp_fft[i]>max) {
				max=temp_fft[i];
				max_p=i;
			}
		}
		//find the max on frequency zone
		// for (int i=0; i<ADClen/2; i+=1)
		// 	if(
		// 		(i-max_p>0 && ((i-max_p)*adc_rate/ADClen)%kHz>100 && ((i-max_p)*adc_rate/ADClen)%kHz<900) ||
		// 		(max_p-i>0 && ((max_p-i)*adc_rate/ADClen)%kHz>100 && ((max_p-i)*adc_rate/ADClen)%kHz<900)
		// 	)
		// 		temp_fft[i]=0;
		//remove all noise except those at about n kHz to max freq
		//because sideband could only be n kHz from baseband freq
		min=0;
		LCD_ShowNum(300, 160, adc_rate, 10, 32);
		//for debug
		max2=0; max2_p=ADClen;
		for (int i=0; i<ADClen/2; i+=1) {
			if(temp_fft[i]>max2 && (i-max_p>5 || i-max_p<-5))
			{
				max2=temp_fft[i];
				max2_p=i;
			}
		}
		max3=0; max3_p=ADClen;
		for(int i=0; i<ADClen/2; i+=1){
			if(temp_fft[i]>max3 && (i-max_p>5 || i-max_p<-5) && (i-max2_p>5 || i-max2_p<-5))
			{
				max3=temp_fft[i];
				max3_p=i;
			}
		}
		//find max2 and max3(probably could be am side band)
		// if((int)((int)max_p-(int)max2_p)-(int)((int)max3_p-(int)max_p)>1 || (int)((int)max_p-(int)max2_p)-(int)((int)max3_p-(int)max_p)<-1)
		// {
		// 	max2_p=max_p;
		// 	max3_p=max_p;
		// 	max2=0;
		// 	max3=0;
		// }
		//check if max2 and max3 are symmetry to max
		//if not, remove those two
		for (int i=0; i<ADClen/2; i+=1) {
			if(
				(i-max_p>3 || i-max_p<-3) &&
				(i-max2_p>3 || i-max2_p<-3) &&
				(i-max3_p>3 || i-max3_p<-3)
			)
				temp_fft[i]=0;
		}
		//remove all harmonic wave n kHz(previously leaved behind at about line 280), except max, max2 and max3
		//if max2 and max3 put to max at line 310, then only base band would be kept in the freq plot
		LCD_ShowNum(0, 120, max_p, 3, 32);
		LCD_ShowNum(100, 120, max2_p, 3, 32);
		LCD_ShowNum(200, 120, max3_p, 3, 32);
		LCD_ShowNum(250, 160, ((u32)temp_fft[max_p]), 6, 16);
		LCD_ShowNum(250, 200, ((u32)temp_fft[max2_p]), 6, 16);
		LCD_ShowNum(250, 240, ((u32)temp_fft[max3_p]), 6, 16);
		//for debug
		am_freq=(int)(((int)max2_p-(int)max3_p)*(double)adc_rate/(2*ADClen));
		if(am_freq<0) am_freq*=-1;
		//calc the mod freq
		am=0.3*(max2+max3)/max+0.7*am;	//加权平均消抖
		//calc the mod depth
		//then do the drawing work!
		am_base_freq=dds_rate+adc_rate*max_p/ADClen;
		LCD_ShowString(0, 0, 160, 32, 32, (u8*)"Base Freq:");
		LCD_ShowNum(160, 0, am_base_freq, 9, 32);
		LCD_ShowString(0, 40, 160, 32, 32, (u8*)"AM Freq:");
		LCD_ShowNum(200, 40, am_freq, 5, 32);
		LCD_ShowString(0, 80, 160, 32, 32, (u8*)"AM Depth: ");
		LCD_ShowNum(200, 80, (u32)(am*100), 3, 32);
		range=max-min;
		prev_x=freq_ox;
		prev_y=freq_oy-(uint16_t)((temp_fft[0]-min)*height/range);
		LCD_Fill(freq_ox, freq_oy-height, freq_ox+width, freq_oy, GRED);
		for (int i=1; i<ADClen/2; i+=1) {
			OS_ENTER_CRITICAL();
			x = freq_ox + i * width * 2 / ADClen;
    	    y = freq_oy - (uint16_t)((temp_fft[i]-min)*height/range);
			LCD_DrawLine(prev_x,prev_y,x,y);
			prev_x=x;
			prev_y=y;
			OS_EXIT_CRITICAL();
		}
		// round_cnt+=1;
		// if (round_cnt%10==0) {
		// 	// dds_rate=am_base_freq-128*kHz;
		// 	// DDS_data_stack.freq1[0]=dds_rate;
		// 	// adc_rate=300*kHz;
		// 	// adc_rate=adjust_adc(adc_rate);
		// 	// dds_task((DDS_data*)&DDS_data_stack);
		// 	// round_cnt=0;
		// }
		//动态调整DDS rate会导致不稳定。

		// LCD_ShowString(0, 0, 160, 32, 32, (u8*)"waiting");
		// key=0;
		// while (key!=KEY0_PRES) {
		// 	key=KEY_Scan(0);
		// }
		// LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	}
}

void dev_task(void *pdata)
{
	//uint8_t down=0;
	OS_CPU_SR cpu_sr=0;
	int arg=0;
	uint16_t time=0;
	LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	for (u8 i=0; i<4; i+=1)
			DrawTickBox(
				lcddev.width*i/4+lcddev.width/16, 
				10, 
				lcddev.width/10, 
				i==DDS_data_stack.controled_channel, 
				BLACK, 
				RED
			);
    while(1)
	{
		delay_ms(20);
		time+=1;
		OS_ENTER_CRITICAL();
		LCD_ShowxNum(0, 200, time, 5, 16, 0);
		//LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
		
		switch (DDS_data_stack.channel_state[DDS_data_stack.controled_channel]) {
			case CHANNEL_STATE_OFF:
			LCD_ShowString(0, lcddev.height-40, 160, 32, 32, (u8*)"OFF");
			LCD_ShowString(lcddev.width/2, lcddev.height-40, 160, 32, 32, (u8*)"      ");
			LCD_Fill(0, lcddev.height/5, lcddev.width, lcddev.height-40, GRED);
			break;
			case CHANNEL_STATE_SINGLE:
			LCD_ShowString(0, lcddev.height/4, 160, 32, 32, (u8*)"FREQUENCY");
			LCD_ShowNum(160, lcddev.height/4, DDS_data_stack.freq1[DDS_data_stack.controled_channel], 9, 32);
			LCD_ShowString(0, lcddev.height/2, 32, 32, 32, (u8*)"AMPLITUDE");
			LCD_ShowNum(160, lcddev.height/2, DDS_data_stack.amp[DDS_data_stack.controled_channel], 4, 32);
			LCD_ShowString(0, lcddev.height-40, 160, 32, 32, (u8*)"ON ");
			LCD_ShowString(lcddev.width/2, lcddev.height-40, 160, 32, 32, (u8*)"SINGLE");
			LCD_Fill(0, lcddev.height/2+40, lcddev.width, lcddev.height-40, GRED);
			break;
			case CHANNEL_STATE_SWEEP:
			LCD_ShowString(0, lcddev.height/5, 160, 32, 32, (u8*)"START FREQUENCY");
			LCD_ShowNum(160, lcddev.height/5, DDS_data_stack.freq1[DDS_data_stack.controled_channel], 9, 32);
			LCD_ShowString(0, lcddev.height*2/5, 160, 32, 32, (u8*)"END FREQUENCY");
			LCD_ShowNum(160, lcddev.height*2/5, DDS_data_stack.freq2[DDS_data_stack.controled_channel], 4, 32);
			LCD_ShowString(0, lcddev.height*3/5, 160, 32, 32, (u8*)"STEP FREQUENCY");
			LCD_ShowNum(160, lcddev.height*3/5, DDS_data_stack.step[DDS_data_stack.controled_channel], 9, 32);
			LCD_ShowString(0, lcddev.height*4/5, 160, 32, 32, (u8*)"AMPLITUDE");
			LCD_ShowNum(160, lcddev.height*4/5, DDS_data_stack.amp[DDS_data_stack.controled_channel], 4, 32);
			LCD_ShowString(0, lcddev.height-40, 160, 32, 32, (u8*)"ON ");
			LCD_ShowString(lcddev.width/2, lcddev.height-40, 160, 32, 32, (u8*)"SWEEP ");
			break;
			default:
			break;
		}
		OS_EXIT_CRITICAL();
		tp_dev.scan(0);
		if(tp_dev.sta&TP_PRES_DOWN && down==0)
		{
			LCD_Fill(0, lcddev.height/10, lcddev.width, lcddev.height, GRED);
			down=1;
			if(tp_dev.y[0]<40 && tp_dev.y[0]>0)
			{
				DDS_data_stack.controled_channel=(uint8_t)(tp_dev.x[0]*4/lcddev.width);
				for (u8 i=0; i<4; i+=1)
					DrawTickBox(
						lcddev.width*i/4+lcddev.width/16, 
						10, 
						lcddev.width/10, 
						i==DDS_data_stack.controled_channel, 
						BLACK, 
						RED
					);
			}
			else if (tp_dev.y[0]>lcddev.height-40 && tp_dev.y[0]<lcddev.height)
			{
				if(tp_dev.x[0]<lcddev.width/2)
					DDS_data_stack.channel_state[DDS_data_stack.controled_channel]=
						(uint8_t)(DDS_data_stack.channel_state[DDS_data_stack.controled_channel]==CHANNEL_STATE_OFF?CHANNEL_STATE_SINGLE:CHANNEL_STATE_OFF);
				else
					DDS_data_stack.channel_state[DDS_data_stack.controled_channel]=
						3-DDS_data_stack.channel_state[DDS_data_stack.controled_channel];
				OS_ENTER_CRITICAL();
				dds_task((void*)&DDS_data_stack);
				OS_EXIT_CRITICAL();
			}
			else switch (DDS_data_stack.channel_state[DDS_data_stack.controled_channel]) {
				case CHANNEL_STATE_SINGLE:
				if(tp_dev.y[0]<lcddev.height-40)
				{
					if(tp_dev.y[0]<lcddev.height/2)
						arg=INPUT_FREQ1;
					else
						arg=INPUT_AMP;	
					input_task(&arg);
				}
				break;
				case CHANNEL_STATE_SWEEP:
				if(tp_dev.y[0]<lcddev.height-40)
				{
					if(tp_dev.y[0]<lcddev.height*2/5)
						arg=INPUT_FREQ1;
					else if(tp_dev.y[0]<lcddev.height*3/5)
						arg=INPUT_FREQ2;
					else if(tp_dev.y[0]<lcddev.height*4/5)
						arg=INPUT_STEP;
					else
						arg=INPUT_AMP;	
					input_task(&arg);
				}
				break;
				case CHANNEL_STATE_OFF:
				default:
				break;
			}
		}
		else {if(tp_dev.sta&TP_PRES_DOWN) ; else down=0;}
	}
}


void input_task(void* pdata)
{
	OS_CPU_SR cpu_sr=0;
	uint32_t integ=0;
	//uint8_t down=0;
	switch (*((int*)pdata)) {
		case INPUT_FREQ1:
		integ=DDS_data_stack.freq1[DDS_data_stack.controled_channel];
		break;
		case INPUT_FREQ2:
		integ=DDS_data_stack.freq2[DDS_data_stack.controled_channel];
		break;
		case INPUT_STEP:
		integ=DDS_data_stack.step[DDS_data_stack.controled_channel];
		break;
		case INPUT_AMP:
		integ=DDS_data_stack.amp[DDS_data_stack.controled_channel];
		break;
	}
	OS_ENTER_CRITICAL();
	POINT_COLOR=BLACK;
	LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	LCD_DrawLine(0, 80, lcddev.width, 80);
	LCD_ShowNum(0, 24, integ, 10, 32);
	LCD_DrawLine(lcddev.width*3/4, 0, 240, 80);
	LCD_DrawLine(lcddev.width*3/4, 40, lcddev.width, 40);
	POINT_COLOR=GREEN;
	LCD_ShowString(lcddev.width*3/4, 4, lcddev.width/4, 32, 32, (u8*)"Confirm");
	POINT_COLOR=RED;
	LCD_ShowString(lcddev.width*3/4, 44, lcddev.width/4, 32, 32, (u8*)"Cancel");
	POINT_COLOR=BLACK;
	LCD_DrawLine(lcddev.width/3, 80, lcddev.width/3, lcddev.height);
	LCD_DrawLine(lcddev.width*2/3, 80, lcddev.width*2/3, lcddev.height);
	LCD_DrawLine(0, (lcddev.height-80)/4+80, lcddev.width, (lcddev.height-80)/4+80);
	LCD_DrawLine(0, (lcddev.height-80)*2/4+80, lcddev.width, (lcddev.height-80)*2/4+80);
	LCD_DrawLine(0, (lcddev.height-80)*3/4+80, lcddev.width, (lcddev.height-80)*3/4+80);
	OS_EXIT_CRITICAL();
	while(1)
	{
		tp_dev.scan(0);
		if(tp_dev.sta&TP_PRES_DOWN && down==0)
		{
			down=1;
			//confirm or cancel
			if(tp_dev.y[0]<80 && tp_dev.y[0]>0)
			{
				if(tp_dev.x[0]>lcddev.width*3/4 && tp_dev.x[0]<lcddev.width)
				{
					if(tp_dev.y[0]<40)
					{
						switch (*((int*)pdata)) {
							case INPUT_FREQ1:
							if(integ>0&&integ<160*MHz)
								DDS_data_stack.freq1[DDS_data_stack.controled_channel]=integ;
							break;
							case INPUT_FREQ2:
							if(integ>0&&integ<160*MHz)
								DDS_data_stack.freq2[DDS_data_stack.controled_channel]=integ;
							break;
							case INPUT_STEP:
							if(integ>0&&integ<160*MHz)
								DDS_data_stack.step[DDS_data_stack.controled_channel]=integ;
							break;
							case INPUT_AMP:
							if(integ>=0 && integ<=1023)
								DDS_data_stack.amp[DDS_data_stack.controled_channel]=integ;
							break;
							default:
							break;
						}
					}
					break;
				}
			}
			//press 1-9
			else if(tp_dev.y[0]>80 && tp_dev.y[0]<(lcddev.height-80)*3/4+80)
			{
				integ=integ*10+(uint32_t)(tp_dev.x[0]*3/lcddev.width)+1+(uint32_t)((tp_dev.y[0]-80)*4/(lcddev.height-80))*3;
				OS_ENTER_CRITICAL();
				LCD_Fill(0+1, 0+1, lcddev.width*3/4-1, 80-1, GRED);
				POINT_COLOR=BLACK;
				LCD_ShowNum(0, 24, integ, 10, 32);
				OS_EXIT_CRITICAL();
			}
			//press Del, 0 or 000
			else 
			{
				switch ((uint8_t)(tp_dev.x[0]*3/lcddev.width)) {
					case 0:
					integ*=1000;
					break;
					case 1:
					integ=integ*10;
					break;
					case 2:
					integ=(uint32_t)(integ/10);
					break;
					default:
					break;
				}

				OS_ENTER_CRITICAL();
				LCD_Fill(0+1, 0+1, lcddev.width*3/4-1, 80-1, GRED);
				POINT_COLOR=BLACK;
				LCD_ShowNum(0, 24, integ, 10, 32);
				OS_EXIT_CRITICAL();
			}
		}
		else {if(tp_dev.sta&TP_PRES_DOWN) ; else down=0;}
	}
	OS_ENTER_CRITICAL();
	dds_task((void*)&DDS_data_stack);
	LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
	for (u8 i=0; i<4; i+=1)
					DrawTickBox(
						lcddev.width*i/4+lcddev.width/16, 
						10, 
						lcddev.width/10, 
						i==DDS_data_stack.controled_channel, 
						BLACK, 
						RED
					);
	OS_EXIT_CRITICAL();
	return;
}

//dds set task
void dds_task(void* pdata)
{
	DDS_data save=*((DDS_data*)pdata);
	for(int i=0;i<=3;i+=1)
	{
		if(save.channel_state[i]==CHANNEL_STATE_SINGLE)
		{
			Write_Frequence(i, DDS_data_stack.freq1[i]);
			AD9959_IO_Update();
			Write_Amplitude(i, DDS_data_stack.amp[i]);
			AD9959_IO_Update();
		}
	}
	return;
}

//dds sweep task
void dds_sweep_task(void* pdata)
{
	OS_CPU_SR cpu_sr=0;
	while(1)
	{
		for(int i=0;i<=3;i+=1)
			if(DDS_data_stack.channel_state[i]==CHANNEL_STATE_SWEEP)
			{
				OS_ENTER_CRITICAL();
				Write_Frequence(i, DDS_data_stack.now_freq[i]);
				Write_Amplitude(i, DDS_data_stack.amp[i]);
				AD9959_IO_Update();
				OS_EXIT_CRITICAL();
				DDS_data_stack.now_freq[i]+=DDS_data_stack.step[i];
				if(DDS_data_stack.now_freq[i]>DDS_data_stack.freq2[i])
					DDS_data_stack.now_freq[i]=DDS_data_stack.freq1[i];
			}
		delay_ms(100);
	}
}
