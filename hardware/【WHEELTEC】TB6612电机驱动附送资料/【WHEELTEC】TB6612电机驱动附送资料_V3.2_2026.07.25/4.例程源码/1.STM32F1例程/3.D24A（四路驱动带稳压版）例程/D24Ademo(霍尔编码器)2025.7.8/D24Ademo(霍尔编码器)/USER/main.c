#include "stm32f10x.h"

#include "delay.h"
#include "gpio.h"
#include "moto.h"
#include "pwm.h"
#include "adc.h"
#include "usart.h"
#include "encoder.h"

 /**************************************************************************
公司：轮趣科技(东莞)有限公司
品牌：WHEELTEC
官网：wheeltec.net
淘宝店铺：shop114407458.taobao.com 
速卖通: https://minibalance.aliexpress.com/store/4455017
版本：V1.0
修改时间：2022-09-01

Brand: WHEELTEC
Website: wheeltec.net
Taobao shop: shop114407458.taobao.com 
Aliexpress: https://minibalance.aliexpress.com/store/4455017
Version: V1.0
Update：2023-06-28

All rights reserved
**************************************************************************/



 
int TargetVelocity=500;
int main(void)
 {	
	int encoder_A,encoder_B,encoder_C,encoder_D;
	int Velocity_PWMA=0,Velocity_PWMB=0,Velocity_PWMC=0,Velocity_PWMD=0;
	u16 adcx;
	float vcc;
   SystemInit(); //配置系统时钟为72M   
   delay_init();    //延时函数初始化
   Gpio_Init();    //初始化gpioB pin_0/1/8/9/12/13/14/15
   uart_init(115200);		//串口初始化
	 
   adc_Init();				//ADC1的初始化 
	 
   PWM_Int(7199,0);      //初始化pwm输出 72000 000 /7199+1=10000    
   Encoder_Init_Tim8();
   Encoder_Init_Tim2();
   Encoder_Init_Tim3();
   Encoder_Init_Tim4();
  while(1)
	{
		adcx=Get_adc_Average(ADC_Channel_5,10);  //获取adc的值
		vcc=(float)adcx*(3.3*11/4096);     				//求当前电压
		/*
		开环控制：不使用编码器的数值来调整控制输入。控制输入是基于设定值直接设定的，不考虑系统的当前状态或输出。
		特点：如果电机的负载突然增加（外部干扰）或电机效率发生变化（电池电压、电机温度等干扰），开环控制可能无法及时调整，导致实际转速与设定值有所偏差。
		tips：使用时将以下注释打开即可
		*/
    Velocity_PWMA=2000;				//开环控制-直接设定PWM值
		Velocity_PWMB=2000;
		Velocity_PWMC=2000;
		Velocity_PWMD=2000;
		Set_PWM(Velocity_PWMA,Velocity_PWMB,Velocity_PWMC,Velocity_PWMD);//开环控制-根据PWM值驱动电机
		printf("开环控制，电压=%6.2f V\r\n",vcc);
		printf("A编码器=%d，A-PWM=%d，B编码器=%d，B-PWM=%d\r\n",encoder_A,Velocity_PWMA,encoder_B,Velocity_PWMB);
		printf("C编码器=%d，C-PWM=%d，D编码器=%d，D-PWM=%d\r\n",encoder_C,Velocity_PWMC,encoder_D,Velocity_PWMD);
		
		/*读取编码器数据方法：通过读取对应定时器的计数值来获取编码器的数据。在学习闭环控制前需要学习如何使用*/
		encoder_A=Read_Encoder(8);		//读取编码器数值，开环时：仅监测当前电机速度。闭环时：监测当前电机速度，用于PID控制器闭环反馈 
		encoder_B=Read_Encoder(2);
		encoder_C=Read_Encoder(3);
		encoder_D=Read_Encoder(4);
		
		/*
		闭环控制：将采样到的编码器数值输入PID控制器形成闭环反馈，再根据PID控制器闭环控制后输出的PWM值对电机进行控制
		特点：通过实时反馈调整，可以更精确地控制电机速度，减少误差，同时能够适应系统参数的变化（电池电压、电机效率等）和外部干扰（负载，摩擦系数等），保持系统的稳定性
		tips：使用时将以下注释打开即可
		*/
//    Velocity_PWMA=Velocity_A(TargetVelocity,encoder_A);		
//		Velocity_PWMB=Velocity_B(TargetVelocity,encoder_B);		//PID闭环控制-输出PWM值
//		Velocity_PWMC=Velocity_C(TargetVelocity,encoder_C);
//		Velocity_PWMD=Velocity_D(TargetVelocity,encoder_D);
//		Set_PWM(Velocity_PWMA,Velocity_PWMB,Velocity_PWMC,Velocity_PWMD);	//闭环控制-根据PWM值驱动电机
//		printf("闭环控制，电压=%6.2f V\r\n",vcc);
//		printf("A目标速度=%d，A编码器=%d，A-PWM=%d\r\n",TargetVelocity,encoder_A,Velocity_PWMA);
//		printf("B目标速度=%d，B编码器=%d，B-PWM=%d\r\n",TargetVelocity,encoder_B,Velocity_PWMB);		
//		printf("C目标速度=%d，C编码器=%d，C-PWM=%d\r\n",TargetVelocity,encoder_C,Velocity_PWMC);
//		printf("D目标速度=%d，D编码器=%d，D-PWM=%d\r\n",TargetVelocity,encoder_D,Velocity_PWMD);	
	}
 }

