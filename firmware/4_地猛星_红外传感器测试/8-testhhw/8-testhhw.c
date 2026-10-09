#include <stdint.h>

#include "ti_msp_dl_config.h"

/*
 * Raw digital state for CCS Expressions/Watch.
 * Bit 0..7 correspond to CH1..CH8.
 */
volatile uint8_t g_sensor_state = 0U;

static void UART_sendChar(char value)
{
    DL_UART_transmitDataBlocking(UART_0_INST, (uint8_t) value);
}

static void UART_sendString(const char *text)
{
    while (*text != '\0') {
        UART_sendChar(*text);
        text++;
    }
}

static uint8_t Sensor_readState(void)
{
    uint8_t state = 0U;

    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH1_PIN) != 0U) {
        state |= (uint8_t) (1U << 0);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH2_PIN) != 0U) {
        state |= (uint8_t) (1U << 1);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH3_PIN) != 0U) {
        state |= (uint8_t) (1U << 2);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH4_PIN) != 0U) {
        state |= (uint8_t) (1U << 3);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_A_PORT,
            GPIO_SENSORS_A_CH5_PIN) != 0U) {
        state |= (uint8_t) (1U << 4);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH6_PIN) != 0U) {
        state |= (uint8_t) (1U << 5);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_B_PORT,
            GPIO_SENSORS_B_CH7_PIN) != 0U) {
        state |= (uint8_t) (1U << 6);
    }
    if (DL_GPIO_readPins(GPIO_SENSORS_A_PORT,
            GPIO_SENSORS_A_CH8_PIN) != 0U) {
        state |= (uint8_t) (1U << 7);
    }

    return state;
}

static void Sensor_printState(uint8_t state)
{
    uint8_t channel;

    for (channel = 0U; channel < 8U; channel++) {
        UART_sendString("CH");
        UART_sendChar((char) ('1' + channel));
        UART_sendChar('=');
        UART_sendChar(((state & (uint8_t) (1U << channel)) != 0U) ? '1' : '0');

        if (channel < 7U) {
            UART_sendChar(' ');
        }
    }
    UART_sendString("\r\n");
}

int main(void)
{
    SYSCFG_DL_init();

    UART_sendString("\r\n[8-testhhw] READY\r\n");
    UART_sendString("Each working channel must toggle between 0 and 1.\r\n");

    while (1) {
        g_sensor_state = Sensor_readState();
        Sensor_printState(g_sensor_state);

        /* Print five samples per second. */
        delay_cycles(CPUCLK_FREQ / 5U);
    }
}
