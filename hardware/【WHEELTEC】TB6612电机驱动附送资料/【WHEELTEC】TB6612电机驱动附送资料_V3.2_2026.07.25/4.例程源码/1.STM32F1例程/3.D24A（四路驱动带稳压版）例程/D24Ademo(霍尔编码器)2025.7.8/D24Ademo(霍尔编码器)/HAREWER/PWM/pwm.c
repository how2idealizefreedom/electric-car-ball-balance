#include "pwm.h"
#include "math.h"
#include <stdlib.h>

/**************************************************************************
函数功能：pwm初始化
入口参数：arr：设为一个时钟频率的最大值  psc： 预分频值
返回  值：无
**************************************************************************/
void PWM_Int(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;                //定义结构体GPIO_InitStructure
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;      //定义结构体TIM_TimeBaseStructure   
	TIM_OCInitTypeDef TIM_OCInitStructure;              //定义结构体TIM_OCInitStructure
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//使能PA端口时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);//使能定时器3
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	TIM_TimeBaseStructure.TIM_Period = arr;                //设置下一个更新活动的自动重装载寄存器的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc;             //预分配值
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;           //时钟分割
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode= TIM_OCMode_PWM1;             //PWM脉冲宽度调制1
	TIM_OCInitStructure.TIM_Pulse = 0;                           //设置待装入捕获比较寄存器的脉冲值
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;    //设置TIM输出极性为高
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//比较输出使能
	TIM_OC1Init(TIM5,&TIM_OCInitStructure);
	TIM_OC2Init(TIM5,&TIM_OCInitStructure);
	TIM_OC3Init(TIM5,&TIM_OCInitStructure);
	TIM_OC4Init(TIM5,&TIM_OCInitStructure);
	
	
	TIM_OC1PreloadConfig(TIM5,TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM5,TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM5,TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM5,TIM_OCPreload_Enable);//使能预装载寄存器
	
	TIM_ARRPreloadConfig(TIM5,ENABLE);              //使能自动装载允许位
	TIM_Cmd(TIM5,ENABLE);//启动定时器5	
}
int myabs(int value){
	if (value<0) value=-value;
	return value;
}
void Set_PWM(int motor1,int motor2,int motor3,int motor4)
{
	if(motor1>0){
		GPIO_SetBits(GPIOC, GPIO_Pin_14);	 // 高电平      PC14 --- AIN2      1   
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);	 // 低电平}   PC13 --- AIN1      0
	
	}else{
		 GPIO_SetBits(GPIOC, GPIO_Pin_13);	 // 高电平       PC13 --- AIN1     1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_14);	 // 低电平}    PC14 --- AIN2     0
	}
	if(motor2>0){
		GPIO_SetBits(GPIOB, GPIO_Pin_13);     //高电平   PB13 --- BIN2       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_12);  // 低电平   PB12 --- BIN1       0
	}else{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);     //高电平   PB12 --- BIN1     1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_13);  // 低电平   PB13 --- BIN2        0
	}
	if(motor3>0){
		 GPIO_SetBits(GPIOB, GPIO_Pin_1);     //高电平   PB1 --- CIN2       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_0);  // 低电平   PB0 --- CIN1       0
	}else{
	  GPIO_SetBits(GPIOB, GPIO_Pin_0);     //高电平   PB0 --- CIN1       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_1);  // 低电平   PB1 --- CIN2       0
	}
	if(motor4>0){
		 GPIO_SetBits(GPIOC, GPIO_Pin_1);     //高电平   PC2 --- DIN2       1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_2);  // 低电平   PC1 --- DIN1       0
	}else{
	GPIO_SetBits(GPIOC, GPIO_Pin_2);     //高电平   PC1 --- DIN1       1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_1);  // 低电平   PC2 --- DIN2       0
	}
	TIM_SetCompare1(TIM5,myabs(motor1));
	TIM_SetCompare2(TIM5,myabs(motor2));
    TIM_SetCompare3(TIM5,myabs(motor3));//设置TIM3通道3的占空比  3000/7200
    TIM_SetCompare4(TIM5,myabs(motor4));
	
}



