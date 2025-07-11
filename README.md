# NUEDC 2022 F

main.c以及CMakeLists位于/USER/

其他各种代码主要位于/HARDWARE/

编译构建输出于/build/

接下来是本题思路环节

![](https://cdn.nlark.com/yuque/0/2025/jpeg/38551157/1751686921688-5eb36034-e2a4-4016-aa45-49552aaaad09.jpeg)

增加额外要求：AM载波取消500kHz步进限制，可能是10M-30M任意频率



07月05日

AM波

技术路线

测量任务：使用单片机GPIO口结合定时器读取载波频率，然后用DDS产生比载波频率略低的本振进行混频，混到低频区后进行低通滤波结合ADC采集，进行FFT，然后找到三个峰值即可计算调制频率与调制深度；

解调任务：使用包络检波解调AM波确保波形同源。

测量任务深入：单片机GPIO口读取采用定时器计数器输入，只能读取高低电平，因此使用高速比较器将AM载波转化成与载波同频的方波再进行测量；又由于AM调制会有一部分幅度过低，比较器可能无法比较，因此使用74HC4046锁相环先将载波频率进行锁定再比较。

实测效果

载波频率：将理想正弦波（假设锁相环已锁定）输入比较器，比较器波形输出至单片机定时计数器的外部触发接口，载波频率测量可测DC-30M+，经过参数调试误差为10Hz数量级；如果将调幅波直接输入至比较器，最高可到0.8调制深度；经过一级放大，最高可至0.98调制深度。

DDS：写了一个可自由调试的UI，包括点频扫频，可自由调整频率与幅度；缺陷：DDS有时会无响应或错误响应，需要重新发送指令，有时甚至需要重启单片机；产生波形略有失真，可能是阻抗匹配问题（示波器FFT查看，除主频外最大谐波相对幅度-28dB）。

ADC&FFT：调制频率由于频域分辨率的影响会有十来赫兹的误差，但是由于步进是1kHz所以可以直接取整解决；调制深度计算还需要调试一下参数，总的来说还是正相关的。如果不做任何处理直接FFT，当调制深度低于30%的时候可能会产生信噪比过低，边带被噪声淹没的问题；编写了一个频域上比较严格的滤除算法之后，可以探测到低至约6%的调制深度。





07月10日

AM的测试有点困难，主要是调制深度达到1.0时载波频率会有差不多20kHz的误差，导致之前的混频到10.7kHz的技术方案不太可行，决定改为混频到约50kHz，用更高的采样率去采集。

今日先测通了FM路线。

载波频率：锁相环锁定范围3-8.5MHz，使用34MHz起始，-4MHz步进本振进行混频后递交给锁相环，以4-8MHz作为有效锁定频段，利用定时器计数器统计脉冲数计算锁定频率反推载波频率，误差10Hz数量级。

调制频率，调制系数，最大频偏：利用锁相环鉴频器进行FM解调，使用ADC采集解调波形，计算极大值均值与极小值均值作差，进行FFT提取主频率，进而利用Excel的预测线功能拟合计算调制系数，发现不同采集轮次之间波动较大，采用新旧值1：3加权平均的方式进行稳定，误差基本在0.2以内。

代码调试小技巧：当最终版代码需要全自动化运行，而你又需要先进行调试时，在自动化代码中加入

```c
LCD_ShowString(0, 0, 160, 32, 32, (u8*)"waiting");
key=0;
while (key!=KEY0_PRES) {
	key=KEY_Scan(0);
}
key=0;
LCD_Fill(0, 0, lcddev.width, lcddev.height, GRED);
```

以及更多LCD_Show系列代码，以达到单步调试的目的。（按KEY0进行下一步）





07月11日

将整个系统调通了。

锁相环的锁定频率存在欺骗行为：在大约3~10MHz能正常锁定，在大约10~12MHz**输出的锁定频率随输入频率增加而下降**，而我们检测只能检测锁相环的输出频率（利用定时器计数器），因此我们即使接收到3~10MHz的锁定信号，我们也不能确定锁相环的输入信号就是同频率信号还是10~12MHz的“欺骗信号”。解决方法是：我们规定4~8MHz为有效锁定范围；由于待测信号载波频率范围为10~30MHz，因此我们将DDS本振从38MHz开始向下以1MHz为步进推进直到14MHz为止（共计25个频点），混频后交给锁相环尝试锁定，依然使用单片机的定时器计数器测量锁相环的输出频率，以推算出的载波频率连续三次在误差范围内近似相同为判定载波频率的依据。具体代码如下：

```c
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
```

由于赛题中对FM有500kHz步进的规定，因此我们将测得的频率直接拟合到最近的500kHz整数倍频即可。对于AM去掉最后47~50行，会有大约几十kHz的误差。为了准确测得AM的频率，我们将AM混频到大约80kHz处，使用300kHz的采样率进行ADC采集，再进行FFT，找到主带频率反推AM频率，误差可降低到0.1kHz以内。

今日花了较多时间用来调试代码的自动化测试流程。目前的代码架构逻辑大致如下：

![画板](https://cdn.nlark.com/yuque/0/2025/jpeg/38551157/1752239762633-053fcdc6-2db2-4e26-aa4a-ebfd3d665586.jpeg)

具体细节参看代码与代码注释。



本题的主要困难点在于对AM取消了500kHz步进限制，这样在AM调制深度较深时会较难获取到AM的载波频率，也就很难进行下一步测量。放大器+比较器然后用计数器计数的方案最高只能做到0.97左右的调制深度，到1.0之后就会有MHz级别的误差了，必须要采用锁相环才能稍微测准一点（10kHz数量级）；而锁相环的锁定范围又不可能覆盖10~30MHz整个带宽（在我们的硬件中，甚至完全不在这个带宽之内），因此要么用多个锁相环来回切换，要么选择进行混频后尝试锁定，多次尝试找到正确的频率。

