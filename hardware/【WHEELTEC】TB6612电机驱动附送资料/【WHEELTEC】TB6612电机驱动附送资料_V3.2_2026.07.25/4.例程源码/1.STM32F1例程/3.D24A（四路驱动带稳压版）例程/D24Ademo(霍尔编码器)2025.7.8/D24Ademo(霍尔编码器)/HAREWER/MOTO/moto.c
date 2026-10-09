#include "moto.h"

float Velcity_Kp=1,  Velcity_Ki=3,  Velcity_Kd; //相关速度PID参数
/**************************************************************************
函数功能：电机的正反转
入口参数：mode   mode=0时为正转  mode=1时反转
返回  值：无
**************************************************************************/


void moto(int mode)
{
	if(mode==1)    //顺时针转动
	{
	 GPIO_SetBits(GPIOC, GPIO_Pin_14);	 // 高电平      PC14 --- AIN2      1   
	 GPIO_ResetBits(GPIOC, GPIO_Pin_13);	 // 低电平}   PC13 --- AIN1      0
	
	 GPIO_SetBits(GPIOB, GPIO_Pin_13);     //高电平   PB13 --- BIN2       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_12);  // 低电平   PB12 --- BIN1       0
		
	 GPIO_SetBits(GPIOB, GPIO_Pin_1);     //高电平   PB1 --- CIN2       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_0);  // 低电平   PB0 --- CIN1       0
		
	 GPIO_SetBits(GPIOC, GPIO_Pin_1);     //高电平   PC2 --- DIN2       1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_2);  // 低电平   PC1 --- DIN1       0
	
	}
	 if(mode==0)   //逆时针转动
	{
	 GPIO_SetBits(GPIOC, GPIO_Pin_13);	 // 高电平       PC13 --- AIN1     1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_14);	 // 低电平}    PC14 --- AIN2     0
	
	 GPIO_SetBits(GPIOB, GPIO_Pin_12);     //高电平   PB12 --- BIN1     1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_13);  // 低电平   PB13 --- BIN2        0
		
     GPIO_SetBits(GPIOB, GPIO_Pin_0);     //高电平   PB0 --- CIN1       1
	 GPIO_ResetBits(GPIOB, GPIO_Pin_1);  // 低电平   PB1 --- CIN2       0
		
	 GPIO_SetBits(GPIOC, GPIO_Pin_2);     //高电平   PC1 --- DIN1       1
	 GPIO_ResetBits(GPIOC, GPIO_Pin_1);  // 低电平   PC2 --- DIN2       0
	 }
 
}
/***************************************************************************
函数功能：电机的闭环控制
入口参数：左右电机的编码器值
返回值  ：电机的PWM
***************************************************************************/
int Velocity_A(int TargetVelocity, int CurrentVelocity)
{  
    int Bias;  //定义相关变量
		static int ControlVelocity, Last_bias; //静态变量，函数调用结束后其值依然存在
		
		Bias=TargetVelocity-CurrentVelocity; //求速度偏差
		
		ControlVelocity+=Velcity_Kp*(Bias-Last_bias)+Velcity_Ki*Bias;  //增量式PI控制器
                                                                   //Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
	                                                                 //Velcity_Ki*Bias             速度控制值由Bias不断积分得到 偏差越大加速度越大
		if(ControlVelocity>7200)ControlVelocity=7200;
		if(ControlVelocity<-7200)ControlVelocity=-7200;
		Last_bias=Bias;	
		return ControlVelocity; //返回速度控制值
}

int Velocity_B(int TargetVelocity, int CurrentVelocity)
{  
    int Bias;  //定义相关变量
		static int ControlVelocity, Last_bias; //静态变量，函数调用结束后其值依然存在
		
		Bias=TargetVelocity-CurrentVelocity; //求速度偏差
		
		ControlVelocity+=Velcity_Kp*(Bias-Last_bias)+Velcity_Ki*Bias;  //增量式PI控制器
                                                                   //Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
	                                                                 //Velcity_Ki*Bias             速度控制值由Bias不断积分得到 偏差越大加速度越大
		if(ControlVelocity>7200)ControlVelocity=7200;
		if(ControlVelocity<-7200)ControlVelocity=-7200;
		Last_bias=Bias;	
		return ControlVelocity; //返回速度控制值
}
int Velocity_C(int TargetVelocity, int CurrentVelocity)
{  
    int Bias;  //定义相关变量
		static int ControlVelocity, Last_bias; //静态变量，函数调用结束后其值依然存在
		
		Bias=TargetVelocity-CurrentVelocity; //求速度偏差
		
		ControlVelocity+=Velcity_Kp*(Bias-Last_bias)+Velcity_Ki*Bias;  //增量式PI控制器
                                                                   //Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
	                                                                 //Velcity_Ki*Bias             速度控制值由Bias不断积分得到 偏差越大加速度越大
		if(ControlVelocity>7200)ControlVelocity=7200;
		if(ControlVelocity<-7200)ControlVelocity=-7200;
		Last_bias=Bias;	
		return ControlVelocity; //返回速度控制值
}
int Velocity_D(int TargetVelocity, int CurrentVelocity)
{  
    int Bias;  //定义相关变量
		static int ControlVelocity, Last_bias; //静态变量，函数调用结束后其值依然存在
		
		Bias=TargetVelocity-CurrentVelocity; //求速度偏差
		
		ControlVelocity+=Velcity_Kp*(Bias-Last_bias)+Velcity_Ki*Bias;  //增量式PI控制器
                                                                   //Velcity_Kp*(Bias-Last_bias) 作用为限制加速度
	                                                                 //Velcity_Ki*Bias             速度控制值由Bias不断积分得到 偏差越大加速度越大
		if(ControlVelocity>7200)ControlVelocity=7200;
		if(ControlVelocity<-7200)ControlVelocity=-7200;
		Last_bias=Bias;	
		return ControlVelocity; //返回速度控制值
}

