#include "ti_msp_dl_config.h"
#include "car_config.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CAR_WAIT = 0,
    CAR_RUN,
    CAR_BRAKE,
    CAR_FINISHED,
    CAR_FAULT
} CarState;

static volatile uint32_t g_tickCount;
static CarState g_state = CAR_WAIT;
static uint32_t g_runMs;
static uint32_t g_finishMs;
static uint32_t g_brakeMs;
static uint32_t g_lostLineMs;
static int32_t g_lastError;
static int32_t g_leftCommand;
static int32_t g_rightCommand;
static uint8_t g_lineMask;
static bool g_leftStartLine;
static bool g_lineLossActive;
static uint8_t g_wideLineTicks;

static void debug_puts(const char *text)
{
    while (*text != '\0') {
        DL_UART_transmitDataBlocking(DEBUG_UART_INST, (uint8_t) *text);
        ++text;
    }
}

static void debug_u32(uint32_t value)
{
    char buffer[10];
    uint32_t index = 0U;

    if (value == 0U) {
        DL_UART_transmitDataBlocking(DEBUG_UART_INST, (uint8_t) '0');
        return;
    }

    while ((value != 0U) && (index < sizeof(buffer))) {
        buffer[index++] = (char) ('0' + (value % 10U));
        value /= 10U;
    }
    while (index != 0U) {
        DL_UART_transmitDataBlocking(
            DEBUG_UART_INST, (uint8_t) buffer[--index]);
    }
}

static void debug_i32(int32_t value)
{
    if (value < 0) {
        DL_UART_transmitDataBlocking(DEBUG_UART_INST, (uint8_t) '-');
        value = -value;
    }
    debug_u32((uint32_t) value);
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static void pwm_write(uint32_t channel, int32_t permille)
{
    uint32_t compare;

    permille = clamp_i32(permille, 0, 1000);
    compare = PWM_PERIOD_COUNT -
        (((uint32_t) permille * PWM_PERIOD_COUNT) / 1000U);
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, compare, channel);
}

static void motor_direction_right(bool forward)
{
    bool in1High = forward ? (RIGHT_FORWARD_IN1_HIGH != 0) :
        (RIGHT_FORWARD_IN1_HIGH == 0);

    if (in1High) {
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    }
}

static void motor_direction_left(bool forward)
{
    bool in1High = forward ? (LEFT_FORWARD_IN1_HIGH != 0) :
        (LEFT_FORWARD_IN1_HIGH == 0);

    if (in1High) {
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_LEFT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_PORT, MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_PORT, MOTOR_DIR_LEFT_IN2_PIN);
    }
}

static void motor_drive(int32_t left, int32_t right)
{
    bool leftForward = (left >= 0);
    bool rightForward = (right >= 0);

    left = (left < 0) ? -left : left;
    right = (right < 0) ? -right : right;
    left = clamp_i32(left, 0, PWM_LIMIT);
    right = clamp_i32(right, 0, PWM_LIMIT);

    motor_direction_left(leftForward);
    motor_direction_right(rightForward);
    pwm_write(GPIO_MOTOR_PWM_C1_IDX, left);
    pwm_write(GPIO_MOTOR_PWM_C0_IDX, right);
    DL_GPIO_setPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);

    g_leftCommand = leftForward ? left : -left;
    g_rightCommand = rightForward ? right : -right;
}

static void motor_coast_disable(void)
{
    pwm_write(GPIO_MOTOR_PWM_C0_IDX, 0);
    pwm_write(GPIO_MOTOR_PWM_C1_IDX, 0);
    DL_GPIO_clearPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_PORT,
        MOTOR_DIR_RIGHT_IN1_PIN | MOTOR_DIR_RIGHT_IN2_PIN |
        MOTOR_DIR_LEFT_IN1_PIN | MOTOR_DIR_LEFT_IN2_PIN);
    g_leftCommand = 0;
    g_rightCommand = 0;
}

static void motor_short_brake(void)
{
    DL_GPIO_setPins(MOTOR_DIR_PORT,
        MOTOR_DIR_RIGHT_IN1_PIN | MOTOR_DIR_RIGHT_IN2_PIN |
        MOTOR_DIR_LEFT_IN1_PIN | MOTOR_DIR_LEFT_IN2_PIN);
    pwm_write(GPIO_MOTOR_PWM_C0_IDX, 1000);
    pwm_write(GPIO_MOTOR_PWM_C1_IDX, 1000);
    DL_GPIO_setPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);
    g_leftCommand = 0;
    g_rightCommand = 0;
}

static bool pin_is_high(GPIO_Regs *port, uint32_t pin)
{
    return ((DL_GPIO_readPins(port, pin) & pin) != 0U);
}

static uint8_t line_read(void)
{
    uint8_t mask = 0U;

    if (pin_is_high(LINE_SENSOR_S1_PORT, LINE_SENSOR_S1_PIN)) mask |= 0x01U;
    if (pin_is_high(LINE_SENSOR_S2_PORT, LINE_SENSOR_S2_PIN)) mask |= 0x02U;
    if (pin_is_high(LINE_SENSOR_S3_PORT, LINE_SENSOR_S3_PIN)) mask |= 0x04U;
    if (pin_is_high(LINE_SENSOR_S4_PORT, LINE_SENSOR_S4_PIN)) mask |= 0x08U;
    if (pin_is_high(LINE_SENSOR_S5_PORT, LINE_SENSOR_S5_PIN)) mask |= 0x10U;
    if (pin_is_high(LINE_SENSOR_S6_PORT, LINE_SENSOR_S6_PIN)) mask |= 0x20U;
    if (pin_is_high(LINE_SENSOR_S7_PORT, LINE_SENSOR_S7_PIN)) mask |= 0x40U;
    if (pin_is_high(LINE_SENSOR_S8_PORT, LINE_SENSOR_S8_PIN)) mask |= 0x80U;

#if LINE_BLACK_IS_HIGH
    return mask;
#else
    /* Normalize telemetry and control so a 1 always means black line. */
    return (uint8_t) (mask ^ 0xFFU);
#endif
}

static uint8_t bit_count(uint8_t value)
{
    uint8_t count = 0U;
    while (value != 0U) {
        count = (uint8_t) (count + (value & 1U));
        value >>= 1U;
    }
    return count;
}

static bool line_error(uint8_t mask, int32_t *error)
{
    static const int16_t position[8] = {
        -350, -250, -150, -50, 50, 150, 250, 350
    };
    int32_t sum = 0;
    uint32_t active = 0U;
    uint32_t index;

    for (index = 0U; index < 8U; ++index) {
        if ((mask & (uint8_t) (1U << index)) != 0U) {
            sum += position[index];
            ++active;
        }
    }
    if (active == 0U) {
        return false;
    }
    *error = sum / (int32_t) active;
    return true;
}

static bool button_pressed_event(void)
{
    static bool lastRaw;
    static bool stablePressed;
    static uint8_t stableTicks;
    bool rawPressed = !pin_is_high(
        USER_INPUT_PORT, USER_INPUT_START_STOP_PIN);

    if (rawPressed == lastRaw) {
        if (stableTicks < 5U) {
            ++stableTicks;
        }
    } else {
        lastRaw = rawPressed;
        stableTicks = 0U;
    }

    if ((stableTicks == 5U) && (stablePressed != rawPressed)) {
        stablePressed = rawPressed;
        if (stablePressed) {
            return true;
        }
    }
    return false;
}

static void car_start(void)
{
    g_runMs = 0U;
    g_finishMs = 0U;
    g_brakeMs = 0U;
    g_lostLineMs = 0U;
    g_lastError = 0;
    g_leftStartLine = false;
    g_lineLossActive = false;
    g_wideLineTicks = 0U;
    g_state = CAR_RUN;
    DL_GPIO_setPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);
    debug_puts("STATE,RUN\r\n");
}

static void car_fault(const char *reason)
{
    motor_coast_disable();
    g_state = CAR_FAULT;
    debug_puts("STATE,FAULT,");
    debug_puts(reason);
    debug_puts("\r\n");
}

static void control_run(void)
{
    int32_t error;
    int32_t derivative;
    int32_t turn;
    int32_t base;
    uint8_t active;
    bool valid;
    bool wide;

    g_runMs += CONTROL_PERIOD_MS;
    g_lineMask = line_read();
    active = bit_count(g_lineMask);
    wide = (active >= 6U);
    valid = line_error(g_lineMask, &error);

    if (!wide) {
        g_leftStartLine = true;
        g_wideLineTicks = 0U;
    } else if (g_wideLineTicks < 255U) {
        ++g_wideLineTicks;
    }

    if (g_leftStartLine && (g_runMs >= MIN_FINISH_TIME_MS) &&
        (g_wideLineTicks >= 4U)) {
        g_finishMs = g_runMs;
        g_brakeMs = 0U;
        g_state = CAR_BRAKE;
        motor_short_brake();
        debug_puts("STATE,BRAKE,T=");
        debug_u32(g_finishMs);
        debug_puts("\r\n");
        return;
    }

    if (!valid) {
        int32_t searchInner;
        int32_t searchOuter;

        if (!g_lineLossActive) {
            g_lineLossActive = true;
            debug_puts("EVENT,LINE_LOST_BEGIN,T=");
            debug_u32(g_runMs);
            debug_puts(",E=");
            debug_i32(g_lastError);
            debug_puts("\r\n");
        }
        g_lostLineMs += CONTROL_PERIOD_MS;
        if (g_lostLineMs >= LOST_LINE_FAULT_MS) {
            car_fault("LINE_LOST");
            return;
        }

        if (g_lostLineMs < LOST_LINE_STAGE2_MS) {
            searchInner = PWM_SEARCH_GENTLE_INNER;
            searchOuter = PWM_SEARCH_GENTLE_OUTER;
        } else {
            searchInner = PWM_SEARCH_HARD_INNER;
            searchOuter = PWM_SEARCH_HARD_OUTER;
        }

        if (g_lastError < 0) {
            motor_drive(searchInner, searchOuter);
        } else {
            motor_drive(searchOuter, searchInner);
        }
        return;
    }

    if (g_lineLossActive) {
        debug_puts("EVENT,LINE_REACQUIRED,T=");
        debug_u32(g_runMs);
        debug_puts(",D=");
        debug_u32(g_lostLineMs);
        debug_puts(",M=");
        debug_u32(g_lineMask);
        debug_puts("\r\n");
        g_lineLossActive = false;
    }
    g_lostLineMs = 0U;
    derivative = error - g_lastError;
    turn = ((error * LINE_KP_NUM) / LINE_KP_DEN) +
        ((derivative * LINE_KD_NUM) / LINE_KD_DEN);
    turn = clamp_i32(turn, -TURN_LIMIT, TURN_LIMIT);

    if ((error > 250) || (error < -250)) {
        base = PWM_BASE_LARGE_ERROR;
    } else if ((error > 150) || (error < -150)) {
        base = PWM_BASE_MEDIUM_ERROR;
    } else {
        base = PWM_BASE_STRAIGHT;
    }

    motor_drive(base + turn, base - turn);
    g_lastError = error;
}

static void telemetry_send(void)
{
    debug_puts("T=");
    debug_u32(g_runMs);
    debug_puts(",S=");
    debug_u32((uint32_t) g_state);
    debug_puts(",M=");
    debug_u32(g_lineMask);
    debug_puts(",E=");
    debug_i32(g_lastError);
    debug_puts(",L=");
    debug_i32(g_leftCommand);
    debug_puts(",R=");
    debug_i32(g_rightCommand);
    debug_puts(",X=");
    debug_u32(g_lostLineMs);
    debug_puts("\r\n");
}

static void control_step(void)
{
    static uint32_t ledMs;
    static uint32_t telemetryMs;
    bool buttonEvent = button_pressed_event();

    g_lineMask = line_read();
    ledMs += CONTROL_PERIOD_MS;
    if (g_state == CAR_RUN) {
        if (buttonEvent) {
            car_fault("BUTTON_STOP");
        } else {
            control_run();
        }
        telemetryMs += CONTROL_PERIOD_MS;
        if (telemetryMs >= TELEMETRY_PERIOD_MS) {
            telemetryMs = 0U;
            telemetry_send();
        }
        if (ledMs >= 100U) {
            ledMs = 0U;
            DL_GPIO_togglePins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
        }
    } else if (g_state == CAR_BRAKE) {
        g_brakeMs += CONTROL_PERIOD_MS;
        if (g_brakeMs >= BRAKE_TIME_MS) {
            motor_coast_disable();
            g_finishMs += BRAKE_TIME_MS;
            g_state = CAR_FINISHED;
            DL_GPIO_setPins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
            debug_puts("STATE,FINISHED,T=");
            debug_u32(g_finishMs);
            debug_puts("\r\n");
        }
    } else {
        telemetryMs += CONTROL_PERIOD_MS;
        if (telemetryMs >= 500U) {
            telemetryMs = 0U;
            telemetry_send();
        }
        if (buttonEvent && ((g_state == CAR_WAIT) ||
            (g_state == CAR_FINISHED) || (g_state == CAR_FAULT))) {
            if (g_lineMask == 0U) {
                debug_puts("EVENT,START_REJECT,NO_LINE\r\n");
            } else {
                car_start();
            }
            ledMs = 0U;
            telemetryMs = 0U;
        } else if ((g_state == CAR_WAIT) && (ledMs >= 500U)) {
            ledMs = 0U;
            DL_GPIO_togglePins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
        }
    }
}

int main(void)
{
    uint32_t handledTicks = 0U;

    SYSCFG_DL_init();

    motor_coast_disable();
    DL_TimerG_startCounter(MOTOR_PWM_INST);
    NVIC_EnableIRQ(CONTROL_TICK_INST_INT_IRQN);
    DL_TimerA_startCounter(CONTROL_TICK_INST);

    debug_puts("H2_LINE_CAR CURVE TEST V1 - SDK 2.11.00.07\r\n");
    debug_puts("BTN PA24: START / EMERGENCY STOP\r\n");

    while (1) {
        while (handledTicks != g_tickCount) {
            ++handledTicks;
            control_step();
        }
        __WFI();
    }
}

void CONTROL_TICK_INST_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(CONTROL_TICK_INST)) {
        case DL_TIMER_IIDX_ZERO:
            ++g_tickCount;
            break;
        default:
            break;
    }
}
