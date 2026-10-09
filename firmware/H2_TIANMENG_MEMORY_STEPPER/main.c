#include "ti_msp_dl_config.h"
#include "car_config.h"
#include "a4988.h"
#include "mpu6050.h"
#include "track_memory.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CAR_WAIT = 0,
    CAR_RUN,
    CAR_BRAKE,
    CAR_FINISHED,
    CAR_FAULT
} CarState;

typedef enum {
    TRACK_TEACH = 0,
    TRACK_REPLAY
} TrackMode;

typedef struct {
    bool lastRaw;
    bool stablePressed;
    uint8_t stableTicks;
} KeyDebounce;

static volatile uint32_t g_tickCount;
static CarState g_state = CAR_WAIT;
static TrackMode g_selectedMode = TRACK_TEACH;
static TrackMode g_activeMode = TRACK_TEACH;
static uint32_t g_runMs;
static uint32_t g_finishMs;
static uint32_t g_brakeMs;
static uint32_t g_lostLineMs;
static uint32_t g_replayIndex;
static int32_t g_lastError;
static int32_t g_leftCommand;
static int32_t g_rightCommand;
static uint8_t g_lineMask;
static bool g_leftStartLine;
static uint8_t g_wideLineTicks;
static KeyDebounce g_startKey;
static KeyDebounce g_leftKey;
static KeyDebounce g_rightKey;
static KeyDebounce g_modeKey;

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
        DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN1_PORT,
            MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN2_PORT,
            MOTOR_DIR_RIGHT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN1_PORT,
            MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN2_PORT,
            MOTOR_DIR_RIGHT_IN2_PIN);
    }
}

static void motor_direction_left(bool forward)
{
    bool in1High = forward ? (LEFT_FORWARD_IN1_HIGH != 0) :
        (LEFT_FORWARD_IN1_HIGH == 0);

    if (in1High) {
        DL_GPIO_setPins(MOTOR_DIR_LEFT_IN1_PORT,
            MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN2_PORT,
            MOTOR_DIR_LEFT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN1_PORT,
            MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_LEFT_IN2_PORT,
            MOTOR_DIR_LEFT_IN2_PIN);
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
    DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN1_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN2_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN1_PORT, MOTOR_DIR_LEFT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN2_PORT, MOTOR_DIR_LEFT_IN2_PIN);
    g_leftCommand = 0;
    g_rightCommand = 0;
}

static void motor_short_brake(void)
{
    DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN1_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN2_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    DL_GPIO_setPins(MOTOR_DIR_LEFT_IN1_PORT, MOTOR_DIR_LEFT_IN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR_LEFT_IN2_PORT, MOTOR_DIR_LEFT_IN2_PIN);
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

static bool key_is_pressed(GPIO_Regs *port, uint32_t pin)
{
    return !pin_is_high(port, pin);
}

static bool key_pressed_event(
    KeyDebounce *key, GPIO_Regs *port, uint32_t pin)
{
    bool rawPressed = key_is_pressed(port, pin);

    if (rawPressed == key->lastRaw) {
        if (key->stableTicks < 5U) {
            ++key->stableTicks;
        }
    } else {
        key->lastRaw = rawPressed;
        key->stableTicks = 0U;
    }
    if ((key->stableTicks == 5U) &&
        (key->stablePressed != rawPressed)) {
        key->stablePressed = rawPressed;
        return key->stablePressed;
    }
    return false;
}

static int32_t manual_turn_command(void)
{
    bool left = key_is_pressed(
        TEACH_KEYS_LEFT_PORT, TEACH_KEYS_LEFT_PIN);
    bool right = key_is_pressed(
        TEACH_KEYS_RIGHT_PORT, TEACH_KEYS_RIGHT_PIN);

    if (left == right) {
        return 0;
    }
    return left ? -TEACH_MANUAL_TURN : TEACH_MANUAL_TURN;
}

static uint8_t line_read(void)
{
    uint8_t mask = 0U;

    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S1_PIN)) mask |= 0x01U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S2_PIN)) mask |= 0x02U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S3_PIN)) mask |= 0x04U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S4_PIN)) mask |= 0x08U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S5_PIN)) mask |= 0x10U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S6_PIN)) mask |= 0x20U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S7_PIN)) mask |= 0x40U;
    if (pin_is_high(LINE_SENSOR_PORT, LINE_SENSOR_S8_PIN)) mask |= 0x80U;

#if LINE_BLACK_IS_HIGH
    return mask;
#else
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

static void print_mode(void)
{
    debug_puts("MODE,");
    debug_puts((g_selectedMode == TRACK_TEACH) ? "TEACH" : "REPLAY");
    debug_puts(",MEM=");
    debug_u32(track_memory_count());
    debug_puts(",VALID=");
    debug_u32(track_memory_valid() ? 1U : 0U);
    debug_puts("\r\n");
}

static void car_start(void)
{
    g_activeMode = g_selectedMode;
    if ((g_activeMode == TRACK_REPLAY) && !track_memory_valid()) {
        g_activeMode = TRACK_TEACH;
        g_selectedMode = TRACK_TEACH;
        debug_puts("WARN,NO_MEMORY,FALLBACK_TEACH\r\n");
    }
    if (g_activeMode == TRACK_TEACH) {
        track_memory_begin_learning();
    }
    a4988_enable(false);
    g_runMs = 0U;
    g_finishMs = 0U;
    g_brakeMs = 0U;
    g_lostLineMs = 0U;
    g_replayIndex = 0U;
    g_lastError = 0;
    g_leftStartLine = false;
    g_wideLineTicks = 0U;
    g_state = CAR_RUN;
    DL_GPIO_setPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);
    debug_puts("STATE,RUN,MODE=");
    debug_puts((g_activeMode == TRACK_TEACH) ? "TEACH" : "REPLAY");
    debug_puts("\r\n");
}

static void car_fault(const char *reason)
{
    motor_coast_disable();
    if ((g_state == CAR_RUN) && (g_activeMode == TRACK_TEACH)) {
        track_memory_cancel_learning();
    }
    g_state = CAR_FAULT;
    debug_puts("STATE,FAULT,");
    debug_puts(reason);
    debug_puts("\r\n");
}

static bool finish_line_detected(uint8_t active)
{
    bool wide = (active >= 6U);

    if (!wide) {
        g_leftStartLine = true;
        g_wideLineTicks = 0U;
    } else if (g_wideLineTicks < 255U) {
        ++g_wideLineTicks;
    }
    return g_leftStartLine && (g_runMs >= MIN_FINISH_TIME_MS) &&
        (g_wideLineTicks >= 4U);
}

static void begin_finish_brake(void)
{
    g_finishMs = g_runMs;
    g_brakeMs = 0U;
    if (g_activeMode == TRACK_TEACH) {
        if (track_memory_finish_learning()) {
            g_selectedMode = TRACK_REPLAY;
            debug_puts("MEM,SAVED,N=");
            debug_u32(track_memory_count());
            debug_puts("\r\n");
        } else {
            debug_puts("MEM,INVALID_TOO_SHORT\r\n");
        }
    }
    g_state = CAR_BRAKE;
    motor_short_brake();
    debug_puts("STATE,BRAKE,T=");
    debug_u32(g_finishMs);
    debug_puts("\r\n");
}

static void control_teach(bool valid, int32_t error, int32_t manualTurn)
{
    int32_t derivative;
    int32_t turn;
    int32_t base;

    if (!valid) {
        if (g_lastError < 0) {
            motor_drive(PWM_SEARCH_INNER + manualTurn,
                PWM_SEARCH_OUTER - manualTurn);
        } else {
            motor_drive(PWM_SEARCH_OUTER + manualTurn,
                PWM_SEARCH_INNER - manualTurn);
        }
        track_memory_record(g_runMs, g_leftCommand, g_rightCommand);
        return;
    }

    derivative = error - g_lastError;
    turn = ((error * LINE_KP_NUM) / LINE_KP_DEN) +
        ((derivative * LINE_KD_NUM) / LINE_KD_DEN) + manualTurn;
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
    track_memory_record(g_runMs, g_leftCommand, g_rightCommand);
}

static bool control_replay(bool valid, int32_t error, int32_t manualTurn)
{
    int32_t memoryLeft;
    int32_t memoryRight;
    int32_t assist = 0;

    if (!track_memory_get(g_runMs, &memoryLeft, &memoryRight,
            &g_replayIndex)) {
        car_fault("MEMORY_END");
        return false;
    }
    if (valid) {
        int32_t derivative = error - g_lastError;
        assist = ((error * MEMORY_ASSIST_KP_NUM) /
                     MEMORY_ASSIST_KP_DEN) +
            ((derivative * MEMORY_ASSIST_KD_NUM) /
                MEMORY_ASSIST_KD_DEN);
        assist = clamp_i32(
            assist, -MEMORY_ASSIST_LIMIT, MEMORY_ASSIST_LIMIT);
        g_lastError = error;
    }
    motor_drive(memoryLeft + assist + manualTurn,
        memoryRight - assist - manualTurn);
    return true;
}

static void control_run(void)
{
    int32_t error = g_lastError;
    int32_t manualTurn = manual_turn_command();
    uint8_t active;
    bool valid;
    uint32_t lostLimit;

    g_runMs += CONTROL_PERIOD_MS;
    g_lineMask = line_read();
    active = bit_count(g_lineMask);
    valid = line_error(g_lineMask, &error);

    if (finish_line_detected(active)) {
        begin_finish_brake();
        return;
    }

    if (valid) {
        g_lostLineMs = 0U;
    } else {
        g_lostLineMs += CONTROL_PERIOD_MS;
    }
    lostLimit = (g_activeMode == TRACK_TEACH) ?
        TEACH_LOST_LINE_FAULT_MS : REPLAY_LOST_LINE_FAULT_MS;
    if (g_lostLineMs >= lostLimit) {
        car_fault("LINE_LOST");
        return;
    }

    if (g_activeMode == TRACK_TEACH) {
        control_teach(valid, error, manualTurn);
    } else {
        (void) control_replay(valid, error, manualTurn);
    }
}

static void telemetry_send(void)
{
    debug_puts("T=");
    debug_u32(g_runMs);
    debug_puts(",S=");
    debug_u32((uint32_t) g_state);
    debug_puts(",MODE=");
    debug_puts((g_activeMode == TRACK_TEACH) ? "T" : "R");
    debug_puts(",M=");
    debug_u32(g_lineMask);
    debug_puts(",E=");
    debug_i32(g_lastError);
    debug_puts(",L=");
    debug_i32(g_leftCommand);
    debug_puts(",R=");
    debug_i32(g_rightCommand);
    debug_puts(",MEM=");
    debug_u32((g_activeMode == TRACK_TEACH) ?
        track_memory_count() : g_replayIndex);
    debug_puts("/");
    debug_u32(track_memory_count());
    debug_puts(",STEP=");
    debug_i32(a4988_position());
    debug_puts("/");
    debug_i32(a4988_target());
    debug_puts("\r\n");
}

static void idle_controls(
    bool leftEvent, bool rightEvent, bool modeEvent)
{
    bool leftHeld = key_is_pressed(
        TEACH_KEYS_LEFT_PORT, TEACH_KEYS_LEFT_PIN);
    bool rightHeld = key_is_pressed(
        TEACH_KEYS_RIGHT_PORT, TEACH_KEYS_RIGHT_PIN);

    if (modeEvent) {
        g_selectedMode = (g_selectedMode == TRACK_TEACH) ?
            TRACK_REPLAY : TRACK_TEACH;
        print_mode();
    }
    if (leftHeld && rightHeld) {
        a4988_enable(false);
        debug_puts("A4988,DISABLED\r\n");
        return;
    }
    if (leftEvent) {
        a4988_enable(true);
        a4988_set_target(
            a4988_target() - A4988_JOG_MICROSTEPS);
        debug_puts("A4988,JOG,-20,TARGET=");
        debug_i32(a4988_target());
        debug_puts("\r\n");
    } else if (rightEvent) {
        a4988_enable(true);
        a4988_set_target(
            a4988_target() + A4988_JOG_MICROSTEPS);
        debug_puts("A4988,JOG,+20,TARGET=");
        debug_i32(a4988_target());
        debug_puts("\r\n");
    }
}

static void control_step(void)
{
    static uint32_t ledMs;
    static uint32_t telemetryMs;
    bool startEvent = key_pressed_event(&g_startKey,
        USER_INPUT_PORT, USER_INPUT_START_STOP_PIN);
    bool leftEvent = key_pressed_event(&g_leftKey,
        TEACH_KEYS_LEFT_PORT, TEACH_KEYS_LEFT_PIN);
    bool rightEvent = key_pressed_event(&g_rightKey,
        TEACH_KEYS_RIGHT_PORT, TEACH_KEYS_RIGHT_PIN);
    bool modeEvent = key_pressed_event(&g_modeKey,
        TEACH_KEYS_MODE_PORT, TEACH_KEYS_MODE_PIN);

    a4988_control_5ms();
    g_lineMask = line_read();
    ledMs += CONTROL_PERIOD_MS;

    if (g_state == CAR_RUN) {
        if (startEvent) {
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
            print_mode();
        }
    } else {
        idle_controls(leftEvent, rightEvent, modeEvent);
        telemetryMs += CONTROL_PERIOD_MS;
        if (telemetryMs >= 500U) {
            telemetryMs = 0U;
            telemetry_send();
        }
        if (startEvent && ((g_state == CAR_WAIT) ||
            (g_state == CAR_FINISHED) || (g_state == CAR_FAULT))) {
            car_start();
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
    track_memory_clear();
    a4988_init();

    DL_TimerG_startCounter(MOTOR_PWM_INST);
    NVIC_EnableIRQ(CONTROL_TICK_INST_INT_IRQN);
    DL_TimerA_startCounter(CONTROL_TICK_INST);
    NVIC_EnableIRQ(STEPPER_TICK_INST_INT_IRQN);
    DL_TimerG_startCounter(STEPPER_TICK_INST);

    debug_puts("H2 TIANMENG MEMORY+P1+A4988 - SDK 2.11.00.07\r\n");
    debug_puts("K1 START/STOP; K2 LEFT/JOG-; K3 RIGHT/JOG+; K4 MODE\r\n");
    debug_puts("A4988 DEFAULT DISABLED; D24A DEFAULT DISABLED\r\n");
    print_mode();
#if MPU6050_BOOT_PROBE
    {
        uint8_t whoAmI = 0U;
        int32_t result = MPU6050_probe(&whoAmI);
        debug_puts("MPU6050,WHO_AM_I=");
        debug_u32((uint32_t) whoAmI);
        debug_puts(",RESULT=");
        debug_i32(result);
        debug_puts("\r\n");
    }
#endif

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

void STEPPER_TICK_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(STEPPER_TICK_INST)) {
        case DL_TIMER_IIDX_ZERO:
            a4988_timer_isr();
            break;
        default:
            break;
    }
}
