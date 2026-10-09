#include "ti_msp_dl_config.h"
#include "a4988.h"
#include "ball_controller.h"
#include "ball_feedback.h"
#include "car_config.h"
#include "display.h"
#include "mpu6050.h"

#include <stdbool.h>
#include <stdint.h>

#if TRACK_B_DETECTOR_READY
#error "TRACK_B_DETECTOR_READY requires a verified encoder/landmark implementation"
#endif

typedef enum {
    SYS_WAIT = 0,
    SYS_RUN,
    SYS_BRAKE,
    SYS_FINISHED,
    SYS_FAULT,
    SYS_BLOCKED
} SystemState;

typedef enum {
    QUESTION_1 = 1,
    QUESTION_2,
    QUESTION_3,
    QUESTION_4,
    QUESTION_5
} QuestionMode;

typedef enum {
    BALL_STAGE_IDLE = 0,
    BALL_STAGE_TO_POSITIVE,
    BALL_STAGE_TO_NEGATIVE,
    BALL_STAGE_HOLD_CENTER
} BallStage;

typedef enum {
    LINE_RUNNING = 0,
    LINE_FINISH,
    LINE_FAULT
} LineResult;

typedef struct {
    bool lastRaw;
    bool stablePressed;
    uint8_t stableTicks;
} KeyDebounce;

static volatile uint32_t g_tickCount;
static uint32_t g_systemMs;
static SystemState g_state = SYS_WAIT;
/* Power-up in Q1 because this mode cannot energize the car or ball actuator. */
static QuestionMode g_question = QUESTION_1;
static BallStage g_ballStage = BALL_STAGE_IDLE;
static uint32_t g_runMs;
static uint32_t g_finishMs;
static uint32_t g_brakeMs;
static uint32_t g_lostLineMs;
static uint32_t g_ballStableMs;
static int32_t g_lastError;
static int32_t g_leftCommand;
static int32_t g_rightCommand;
static uint8_t g_lineMask;
static bool g_leftStartLine;
static bool g_lapArmed;
static bool g_stepperTickRunning;
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

static int32_t abs_i32(int32_t value)
{
    return (value < 0) ? -value : value;
}

static const char *question_name(QuestionMode question)
{
    switch (question) {
        case QUESTION_1: return "Q1_VIDEO";
        case QUESTION_2: return "Q2_LAP";
        case QUESTION_3: return "Q3_BALL";
        case QUESTION_4: return "Q4_AB_BAL";
        case QUESTION_5: return "Q5_LAP_BAL";
        default: return "Q_UNKNOWN";
    }
}

static const char *state_name(SystemState state)
{
    switch (state) {
        case SYS_WAIT: return "WAIT";
        case SYS_RUN: return "RUN";
        case SYS_BRAKE: return "BRAKE";
        case SYS_FINISHED: return "FINISHED";
        case SYS_FAULT: return "FAULT";
        case SYS_BLOCKED: return "BLOCKED";
        default: return "UNKNOWN";
    }
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
        DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN1_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN2_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_RIGHT_IN1_PORT, MOTOR_DIR_RIGHT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_RIGHT_IN2_PORT, MOTOR_DIR_RIGHT_IN2_PIN);
    }
}

static void motor_direction_left(bool forward)
{
    bool in1High = forward ? (LEFT_FORWARD_IN1_HIGH != 0) :
        (LEFT_FORWARD_IN1_HIGH == 0);

    if (in1High) {
        DL_GPIO_setPins(MOTOR_DIR_LEFT_IN1_PORT, MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN2_PORT, MOTOR_DIR_LEFT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_DIR_LEFT_IN1_PORT, MOTOR_DIR_LEFT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_DIR_LEFT_IN2_PORT, MOTOR_DIR_LEFT_IN2_PIN);
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

static bool lap_finish_detected(uint8_t mask, uint8_t active)
{
    bool finishMarker =
        (active >= FINISH_MARKER_MIN_ACTIVE) &&
        ((mask & 0x18U) == 0x18U);

    if (!finishMarker) {
        g_leftStartLine = true;
        g_wideLineTicks = 0U;
    } else if (g_wideLineTicks < 255U) {
        ++g_wideLineTicks;
    }
    if (!g_lapArmed && g_leftStartLine &&
        (g_runMs >= MIN_FINISH_TIME_MS)) {
        g_lapArmed = true;
        debug_puts("EVENT,LAP_ARMED,T=");
        debug_u32(g_runMs);
        debug_puts("\r\n");
    }
    if (g_lapArmed && (g_wideLineTicks >= FINISH_MARKER_TICKS)) {
        debug_puts("EVENT,A_DETECTED,T=");
        debug_u32(g_runMs);
        debug_puts(",M=");
        debug_u32(mask);
        debug_puts("\r\n");
        return true;
    }
    return false;
}

static LineResult line_follow_step(bool detectFinishA)
{
    int32_t error = g_lastError;
    int32_t derivative;
    int32_t turn;
    int32_t base;
    uint8_t active;
    bool valid;

    g_lineMask = line_read();
    active = bit_count(g_lineMask);
    valid = line_error(g_lineMask, &error);
    if (detectFinishA && lap_finish_detected(g_lineMask, active)) {
        return LINE_FINISH;
    }
    if (!valid) {
        g_lostLineMs += CONTROL_PERIOD_MS;
        if (g_lostLineMs >= LOST_LINE_FAULT_MS) {
            return LINE_FAULT;
        }
        if (g_lastError < 0) {
            motor_drive(PWM_SEARCH_INNER, PWM_SEARCH_OUTER);
        } else {
            motor_drive(PWM_SEARCH_OUTER, PWM_SEARCH_INNER);
        }
        return LINE_RUNNING;
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
    return LINE_RUNNING;
}

static const char *mode_block_reason(QuestionMode question)
{
    if ((question == QUESTION_3) || (question == QUESTION_4) ||
        (question == QUESTION_5)) {
        if (!K230_UART_PROTOCOL_READY) return "K230_PROTOCOL_TBD";
        if (!BALL_AXIS_CALIBRATED) return "BALL_AXIS_NOT_CALIBRATED";
    }
    if ((question == QUESTION_4) && !TRACK_B_DETECTOR_READY) {
        return "B_DETECTOR_TBD";
    }
    return (const char *) 0;
}

static void print_mode(void)
{
    debug_puts("MODE,Q=");
    debug_u32((uint32_t) g_question);
    debug_puts(",NAME=");
    debug_puts(question_name(g_question));
    debug_puts(",OLED=");
    debug_u32(display_ready() ? 1U : 0U);
    debug_puts("\r\n");
    display_show_mode((uint8_t) g_question, question_name(g_question));
}

static void stepper_tick_sync(void)
{
    bool needTick = a4988_motion_pending();

    if (needTick && !g_stepperTickRunning) {
        NVIC_ClearPendingIRQ(STEPPER_TICK_INST_INT_IRQN);
        NVIC_EnableIRQ(STEPPER_TICK_INST_INT_IRQN);
        DL_TimerG_startCounter(STEPPER_TICK_INST);
        g_stepperTickRunning = true;
    } else if (!needTick && g_stepperTickRunning) {
        DL_TimerG_stopCounter(STEPPER_TICK_INST);
        NVIC_DisableIRQ(STEPPER_TICK_INST_INT_IRQN);
        NVIC_ClearPendingIRQ(STEPPER_TICK_INST_INT_IRQN);
        a4988_force_step_low();
        g_stepperTickRunning = false;
    }
}

static void all_actuators_safe(void)
{
    motor_coast_disable();
    ball_controller_disable();
    stepper_tick_sync();
}

static void system_fault(const char *reason)
{
    all_actuators_safe();
    g_state = SYS_FAULT;
    debug_puts("STATE,FAULT,Q=");
    debug_u32((uint32_t) g_question);
    debug_puts(",REASON=");
    debug_puts(reason);
    debug_puts("\r\n");
    display_show_status("FAULT", g_runMs);
}

static void system_finished(void)
{
    all_actuators_safe();
    g_finishMs = g_runMs;
    g_state = SYS_FINISHED;
    DL_GPIO_setPins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
    debug_puts("STATE,FINISHED,Q=");
    debug_u32((uint32_t) g_question);
    debug_puts(",T=");
    debug_u32(g_finishMs);
    debug_puts("\r\n");
    display_show_status("FINISHED", g_finishMs);
}

static void begin_finish_brake(void)
{
    g_finishMs = g_runMs;
    g_brakeMs = 0U;
    ball_controller_disable();
    stepper_tick_sync();
    g_state = SYS_BRAKE;
    motor_short_brake();
    debug_puts("STATE,BRAKE,Q=");
    debug_u32((uint32_t) g_question);
    debug_puts(",T=");
    debug_u32(g_finishMs);
    debug_puts("\r\n");
}

static void reset_run_state(void)
{
    g_runMs = 0U;
    g_finishMs = 0U;
    g_brakeMs = 0U;
    g_lostLineMs = 0U;
    g_ballStableMs = 0U;
    g_lastError = 0;
    g_leftStartLine = false;
    g_lapArmed = false;
    g_wideLineTicks = 0U;
    g_ballStage = BALL_STAGE_IDLE;
}

static void system_start(void)
{
    const char *blocked = mode_block_reason(g_question);
    BallFeedback feedback;

    all_actuators_safe();
    reset_run_state();
    if (blocked != (const char *) 0) {
        g_state = SYS_BLOCKED;
        debug_puts("STATE,BLOCKED,Q=");
        debug_u32((uint32_t) g_question);
        debug_puts(",REASON=");
        debug_puts(blocked);
        debug_puts("\r\n");
        display_show_status("BLOCKED", 0U);
        return;
    }
    if (((g_question == QUESTION_3) || (g_question == QUESTION_4) ||
        (g_question == QUESTION_5)) &&
        !ball_feedback_latest(g_systemMs, &feedback)) {
        g_state = SYS_BLOCKED;
        debug_puts("STATE,BLOCKED,REASON=NO_FRESH_BALL_DATA\r\n");
        display_show_status("NO BALL DATA", 0U);
        return;
    }

    if (g_question == QUESTION_3) {
        g_ballStage = BALL_STAGE_TO_POSITIVE;
        ball_controller_set_target(BALL_POSITIVE_TARGET_MM);
    } else if ((g_question == QUESTION_4) ||
        (g_question == QUESTION_5)) {
        g_ballStage = BALL_STAGE_HOLD_CENTER;
        ball_controller_set_target(BALL_CENTER_TARGET_MM);
    }
    g_state = SYS_RUN;
    if ((g_question == QUESTION_2) || (g_question == QUESTION_4) ||
        (g_question == QUESTION_5)) {
        DL_GPIO_setPins(DRV_SAFE_PORT, DRV_SAFE_STBY_PIN);
    }
    debug_puts("STATE,RUN,Q=");
    debug_u32((uint32_t) g_question);
    debug_puts("\r\n");
    display_show_status("RUN", 0U);
}

static void run_question_1(void)
{
    g_runMs += CONTROL_PERIOD_MS;
    /* K230 WiFi capture remains independent until a control protocol is supplied. */
}

static void run_question_2(void)
{
    LineResult result;

    g_runMs += CONTROL_PERIOD_MS;
    result = line_follow_step(true);
    if (result == LINE_FINISH) {
        begin_finish_brake();
    } else if (result == LINE_FAULT) {
        system_fault("LINE_LOST");
    }
}

static bool ball_is_stable(const BallFeedback *feedback, int32_t targetMm)
{
    return (abs_i32(feedback->positionMm - targetMm) <=
            BALL_ALLOWED_ERROR_MM) &&
        (abs_i32(feedback->velocityMmPerSecond) <=
            BALL_STABLE_SPEED_MMPS);
}

static void run_question_3(void)
{
    BallFeedback feedback;
    int32_t target;

    g_runMs += CONTROL_PERIOD_MS;
    if (!ball_controller_step(g_systemMs) ||
        !ball_feedback_latest(g_systemMs, &feedback)) {
        system_fault("BALL_LINK_LOST");
        return;
    }
    target = ball_controller_target();
    if (ball_is_stable(&feedback, target)) {
        g_ballStableMs += CONTROL_PERIOD_MS;
    } else {
        g_ballStableMs = 0U;
    }
    if ((g_ballStage == BALL_STAGE_TO_POSITIVE) &&
        (g_ballStableMs >= BALL_STABLE_TIME_MS)) {
        g_ballStage = BALL_STAGE_TO_NEGATIVE;
        g_ballStableMs = 0U;
        ball_controller_set_target(BALL_NEGATIVE_TARGET_MM);
        debug_puts("EVENT,Q3_POSITIVE_REACHED,T=");
        debug_u32(g_runMs);
        debug_puts("\r\n");
    } else if ((g_ballStage == BALL_STAGE_TO_NEGATIVE) &&
        (g_ballStableMs >= BALL_STABLE_TIME_MS)) {
        system_finished();
    } else if (g_runMs >= Q3_TIME_LIMIT_MS) {
        system_fault("Q3_TIMEOUT");
    }
}

static bool track_b_detected(void)
{
    return false;
}

static void run_question_4(void)
{
    LineResult lineResult;

    g_runMs += CONTROL_PERIOD_MS;
    if (!ball_controller_step(g_systemMs)) {
        system_fault("BALL_LINK_LOST");
        return;
    }
    lineResult = line_follow_step(false);
    if (lineResult == LINE_FAULT) {
        system_fault("LINE_LOST");
    } else if (track_b_detected()) {
        begin_finish_brake();
    } else if (g_runMs >= Q4_TIME_LIMIT_MS) {
        system_fault("Q4_TIMEOUT");
    }
}

static void run_question_5(void)
{
    LineResult lineResult;

    g_runMs += CONTROL_PERIOD_MS;
    if (!ball_controller_step(g_systemMs)) {
        system_fault("BALL_LINK_LOST");
        return;
    }
    lineResult = line_follow_step(true);
    if (lineResult == LINE_FINISH) {
        begin_finish_brake();
    } else if (lineResult == LINE_FAULT) {
        system_fault("LINE_LOST");
    } else if (g_runMs >= Q5_TIME_LIMIT_MS) {
        system_fault("Q5_TIMEOUT");
    }
}

static void telemetry_send(void)
{
    BallFeedback feedback;
    BallRawFrame rawFrame;
    bool ballValid = ball_feedback_latest(g_systemMs, &feedback);
    bool rawValid = ball_feedback_raw_latest(g_systemMs, &rawFrame);

    debug_puts("T=");
    debug_u32(g_runMs);
    debug_puts(",S=");
    debug_u32((uint32_t) g_state);
    debug_puts(",Q=");
    debug_u32((uint32_t) g_question);
    debug_puts(",M=");
    debug_u32(g_lineMask);
    debug_puts(",E=");
    debug_i32(g_lastError);
    debug_puts(",L=");
    debug_i32(g_leftCommand);
    debug_puts(",R=");
    debug_i32(g_rightCommand);
    debug_puts(",BALL=");
    debug_u32(ballValid ? 1U : 0U);
    if (ballValid) {
        debug_puts(",X=");
        debug_i32(feedback.positionMm);
        debug_puts(",V=");
        debug_i32(feedback.velocityMmPerSecond);
    }
    debug_puts(",K230=");
    debug_u32(rawValid ? 1U : 0U);
    if (rawValid) {
        debug_puts(",BV=");
        debug_i32(rawFrame.valid);
        debug_puts(",BX=");
        debug_i32(rawFrame.xPixels);
        debug_puts(",BE=");
        debug_i32(rawFrame.errorPixels);
        debug_puts(",BVX=");
        debug_i32(rawFrame.velocityPixelsPerFrame);
        debug_puts(",BL=");
        debug_i32(rawFrame.lostFrames);
        debug_puts(",BSEQ=");
        debug_u32(rawFrame.sequence);
    }
    debug_puts(",STEP=");
    debug_i32(a4988_position());
    debug_puts("/");
    debug_i32(a4988_target());
    debug_puts(",STIRQ=");
    debug_u32(g_stepperTickRunning ? 1U : 0U);
    debug_puts("\r\n");
}

static void change_question(void)
{
    all_actuators_safe();
    g_question = (g_question == QUESTION_5) ? QUESTION_1 :
        (QuestionMode) ((uint32_t) g_question + 1U);
    g_state = SYS_WAIT;
    reset_run_state();
    print_mode();
}

static void idle_stepper_service(bool leftEvent, bool rightEvent)
{
    bool leftHeld = key_is_pressed(TEACH_KEYS_LEFT_PORT, TEACH_KEYS_LEFT_PIN);
    bool rightHeld = key_is_pressed(
        TEACH_KEYS_RIGHT_PORT, TEACH_KEYS_RIGHT_PIN);

    if (g_question != QUESTION_3) {
        return;
    }
    if (leftHeld && rightHeld) {
        a4988_enable(false);
        debug_puts("A4988,DISABLED\r\n");
    } else if (leftEvent) {
        a4988_enable(true);
        a4988_set_target(a4988_target() - A4988_JOG_MICROSTEPS);
        debug_puts("A4988,JOG,-20,TARGET=");
        debug_i32(a4988_target());
        debug_puts("\r\n");
    } else if (rightEvent) {
        a4988_enable(true);
        a4988_set_target(a4988_target() + A4988_JOG_MICROSTEPS);
        debug_puts("A4988,JOG,+20,TARGET=");
        debug_i32(a4988_target());
        debug_puts("\r\n");
    }
}

static void control_step(void)
{
    static uint32_t ledMs;
    static uint32_t telemetryMs;
    static uint32_t displayMs;
    bool startEvent;
    bool leftEvent;
    bool rightEvent;
    bool modeEvent;

    g_systemMs += CONTROL_PERIOD_MS;
    startEvent = key_pressed_event(&g_startKey,
        USER_INPUT_PORT, USER_INPUT_START_STOP_PIN);
    leftEvent = key_pressed_event(&g_leftKey,
        TEACH_KEYS_LEFT_PORT, TEACH_KEYS_LEFT_PIN);
    rightEvent = key_pressed_event(&g_rightKey,
        TEACH_KEYS_RIGHT_PORT, TEACH_KEYS_RIGHT_PIN);
    modeEvent = key_pressed_event(&g_modeKey,
        TEACH_KEYS_MODE_PORT, TEACH_KEYS_MODE_PIN);
    ball_feedback_service(g_systemMs);
    a4988_control_5ms();
    g_lineMask = line_read();
    ledMs += CONTROL_PERIOD_MS;
    telemetryMs += CONTROL_PERIOD_MS;
    displayMs += CONTROL_PERIOD_MS;

    if (g_state == SYS_RUN) {
        if (startEvent) {
            if (g_question == QUESTION_1) {
                system_finished();
            } else {
                system_fault("BUTTON_STOP");
            }
        } else {
            switch (g_question) {
                case QUESTION_1: run_question_1(); break;
                case QUESTION_2: run_question_2(); break;
                case QUESTION_3: run_question_3(); break;
                case QUESTION_4: run_question_4(); break;
                case QUESTION_5: run_question_5(); break;
                default: system_fault("BAD_QUESTION"); break;
            }
        }
        if (ledMs >= 100U) {
            ledMs = 0U;
            DL_GPIO_togglePins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
        }
    } else if (g_state == SYS_BRAKE) {
        g_brakeMs += CONTROL_PERIOD_MS;
        g_runMs += CONTROL_PERIOD_MS;
        if (g_brakeMs >= BRAKE_TIME_MS) {
            motor_coast_disable();
            system_finished();
        }
    } else {
        idle_stepper_service(leftEvent, rightEvent);
        if (modeEvent) {
            change_question();
        } else if (startEvent) {
            system_start();
            ledMs = 0U;
            telemetryMs = 0U;
        } else if ((g_state == SYS_WAIT) && (ledMs >= 500U)) {
            ledMs = 0U;
            DL_GPIO_togglePins(STATUS_LED_PORT, STATUS_LED_LED_PIN);
        }
    }

    stepper_tick_sync();
    if (telemetryMs >= ((g_state == SYS_RUN) ?
        TELEMETRY_PERIOD_MS : 500U)) {
        telemetryMs = 0U;
        telemetry_send();
    }
    if (displayMs >= 100U) {
        displayMs = 0U;
        display_show_status(state_name(g_state),
            (g_state == SYS_FINISHED) ? g_finishMs : g_runMs);
    }
    display_service();
}

int main(void)
{
    uint32_t handledTicks = 0U;

    SYSCFG_DL_init();
    motor_coast_disable();
    a4988_init();
    ball_feedback_init();
    ball_controller_init();
    display_init();
    DL_TimerG_startCounter(MOTOR_PWM_INST);

    NVIC_SetPriority(CONTROL_TICK_INST_INT_IRQN, 0U);
    NVIC_EnableIRQ(CONTROL_TICK_INST_INT_IRQN);
    DL_TimerA_startCounter(CONTROL_TICK_INST);
    NVIC_SetPriority(STEPPER_TICK_INST_INT_IRQN, 2U);
    NVIC_DisableIRQ(STEPPER_TICK_INST_INT_IRQN);
    DL_TimerG_stopCounter(STEPPER_TICK_INST);
    NVIC_SetPriority(BALL_UART_INST_INT_IRQN, 1U);
    NVIC_ClearPendingIRQ(BALL_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(BALL_UART_INST_INT_IRQN);

    debug_puts("H1-H5 TIANMENG SAFE INTEGRATION - SDK 2.11.00.07\r\n");
    debug_puts("START=PB21; MODE=EXP_KEY2/PB11; SERVICE=PA26/PA27\r\n");
    debug_puts("BASELINE=P1_22_44; MEMORY_REPLAY=OFF; A4988_IRQ=ON_DEMAND\r\n");
    debug_puts("K230_RX=UART1_PA9,115200,FRAME=$B,V,X,E,VX,LOST\r\n");
    if (!display_ready()) {
        debug_puts("WARN,OLED_NOT_DETECTED,UART_MODE_DISPLAY_ACTIVE\r\n");
    }
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

void BALL_UART_INST_IRQHandler(void)
{
    if (DL_UART_Main_getPendingInterrupt(BALL_UART_INST) ==
        DL_UART_MAIN_IIDX_RX) {
        ball_feedback_rx_isr_byte(
            DL_UART_Main_receiveData(BALL_UART_INST));
    }
}
