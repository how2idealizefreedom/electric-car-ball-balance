#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

#define MPU6050_ADDRESS_AD0_LOW  (0x68U)

int32_t MPU6050_writeRegister(uint8_t reg, uint8_t value);
int32_t MPU6050_readRegisters(uint8_t reg, uint8_t *data, uint32_t length);
int32_t MPU6050_probe(uint8_t *whoAmI);
int32_t MPU6050_wake(void);

#endif
