#include "mpu6050.h"

#include "ti_msp_dl_config.h"

#include <stddef.h>

#define MPU6050_REG_PWR_MGMT_1  (0x6BU)
#define MPU6050_REG_WHO_AM_I    (0x75U)
#define MPU6050_EXPECTED_ID     (0x68U)
#define MPU6050_TIMEOUT_LOOPS   (100000U)

static int32_t wait_controller_idle(void)
{
    uint32_t timeout = MPU6050_TIMEOUT_LOOPS;

    while ((DL_I2C_getControllerStatus(MPU_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (--timeout == 0U) {
            return -1;
        }
    }
    if ((DL_I2C_getControllerStatus(MPU_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        return -2;
    }
    return 0;
}

static int32_t wait_bus_not_busy(void)
{
    uint32_t timeout = MPU6050_TIMEOUT_LOOPS;

    while ((DL_I2C_getControllerStatus(MPU_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY_BUS) != 0U) {
        if (--timeout == 0U) {
            return -3;
        }
    }
    return 0;
}

int32_t MPU6050_writeRegister(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    int32_t status;

    status = wait_controller_idle();
    if (status != 0) {
        return status;
    }
    DL_I2C_flushControllerTXFIFO(MPU_I2C_INST);
    if (DL_I2C_fillControllerTXFIFO(MPU_I2C_INST, tx, sizeof(tx)) !=
            sizeof(tx)) {
        return -4;
    }
    DL_I2C_startControllerTransfer(MPU_I2C_INST,
        MPU6050_ADDRESS_AD0_LOW, DL_I2C_CONTROLLER_DIRECTION_TX,
        sizeof(tx));
    status = wait_bus_not_busy();
    if (status != 0) {
        return status;
    }
    return wait_controller_idle();
}

int32_t MPU6050_readRegisters(uint8_t reg, uint8_t *data, uint32_t length)
{
    uint32_t index;
    uint32_t timeout;
    int32_t status;

    if ((data == NULL) || (length == 0U) || (length > 255U)) {
        return -5;
    }

    status = wait_controller_idle();
    if (status != 0) {
        return status;
    }
    DL_I2C_flushControllerTXFIFO(MPU_I2C_INST);
    DL_I2C_transmitControllerData(MPU_I2C_INST, reg);
    DL_I2C_startControllerTransfer(MPU_I2C_INST,
        MPU6050_ADDRESS_AD0_LOW, DL_I2C_CONTROLLER_DIRECTION_TX, 1U);
    status = wait_bus_not_busy();
    if (status != 0) {
        return status;
    }
    status = wait_controller_idle();
    if (status != 0) {
        return status;
    }

    DL_I2C_flushControllerRXFIFO(MPU_I2C_INST);
    DL_I2C_startControllerTransfer(MPU_I2C_INST,
        MPU6050_ADDRESS_AD0_LOW, DL_I2C_CONTROLLER_DIRECTION_RX,
        length);
    for (index = 0U; index < length; ++index) {
        timeout = MPU6050_TIMEOUT_LOOPS;
        while (DL_I2C_isControllerRXFIFOEmpty(MPU_I2C_INST)) {
            if ((DL_I2C_getControllerStatus(MPU_I2C_INST) &
                    DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
                return -6;
            }
            if (--timeout == 0U) {
                return -7;
            }
        }
        data[index] = DL_I2C_receiveControllerData(MPU_I2C_INST);
    }
    status = wait_bus_not_busy();
    if (status != 0) {
        return status;
    }
    return wait_controller_idle();
}

int32_t MPU6050_probe(uint8_t *whoAmI)
{
    int32_t result;

    if (whoAmI == NULL) {
        return -8;
    }
    *whoAmI = 0U;
    result = MPU6050_readRegisters(MPU6050_REG_WHO_AM_I, whoAmI, 1U);
    if (result != 0) {
        return result;
    }
    return (*whoAmI == MPU6050_EXPECTED_ID) ? 0 : -9;
}

int32_t MPU6050_wake(void)
{
    return MPU6050_writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00U);
}
