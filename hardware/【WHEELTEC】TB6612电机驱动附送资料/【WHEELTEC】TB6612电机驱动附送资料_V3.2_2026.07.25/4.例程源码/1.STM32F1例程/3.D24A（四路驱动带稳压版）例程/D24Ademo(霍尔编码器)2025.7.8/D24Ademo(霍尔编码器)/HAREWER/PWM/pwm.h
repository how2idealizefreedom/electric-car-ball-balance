#ifndef __PWM_H
#define	__PWM_H

#include "stm32f10x.h"

void PWM_Int(u16 arr,u16 psc);
void Set_PWM(int motor1,int motor2,int motot3,int motor4);

#endif

