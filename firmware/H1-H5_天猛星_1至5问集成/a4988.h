#ifndef A4988_H_
#define A4988_H_

#include "ti_msp_dl_config.h"
#include "car_config.h"

#include <stdbool.h>
#include <stdint.h>

static volatile int32_t g_a4988Position;
static volatile int32_t g_a4988Target;
static volatile int32_t g_a4988Rate;
static volatile uint32_t g_a4988Phase;
static volatile bool g_a4988Enabled;
static volatile bool g_a4988StepHigh;
static volatile int32_t g_a4988Direction;

static inline bool a4988_home_active(void);

static inline int32_t a4988_clamp(int32_t value, int32_t low, int32_t high)
{
    return (value < low) ? low : ((value > high) ? high : value);
}

static inline void a4988_init(void)
{
    DL_GPIO_clearPins(A4988_STEP_PORT, A4988_IO_STEP_PIN);
    DL_GPIO_clearPins(A4988_DIR_PORT, A4988_IO_DIR_PIN);
    DL_GPIO_setPins(A4988_ENABLE_PORT, A4988_IO_ENABLE_PIN);
    g_a4988Position = 0;
    g_a4988Target = 0;
    g_a4988Rate = 0;
    g_a4988Phase = 0U;
    g_a4988Enabled = false;
    g_a4988StepHigh = false;
    g_a4988Direction = 0;
}

static inline void a4988_enable(bool enable)
{
    g_a4988Enabled = enable;
    if (enable) {
        DL_GPIO_clearPins(A4988_ENABLE_PORT, A4988_IO_ENABLE_PIN);
    } else {
        g_a4988Rate = 0;
        g_a4988Phase = 0U;
        g_a4988StepHigh = false;
        DL_GPIO_clearPins(A4988_STEP_PORT, A4988_IO_STEP_PIN);
        DL_GPIO_setPins(A4988_ENABLE_PORT, A4988_IO_ENABLE_PIN);
    }
}

static inline void a4988_set_target(int32_t microsteps)
{
    g_a4988Target = a4988_clamp(microsteps,
        -A4988_MAX_ABS_MICROSTEPS, A4988_MAX_ABS_MICROSTEPS);
}

static inline void a4988_control_5ms(void)
{
    int32_t error;
    int32_t desired;
    int32_t rateDelta =
        (A4988_ACCEL_SPS2 * (int32_t) CONTROL_PERIOD_MS) / 1000;

    if (!g_a4988Enabled) {
        return;
    }
    error = g_a4988Target - g_a4988Position;
    if (error == 0) {
        desired = 0;
    } else {
        int32_t magnitude = A4988_MAX_RATE_SPS;
        int32_t distance = (error < 0) ? -error : error;
        if (distance < A4988_SLOW_ZONE_MICROSTEPS) {
            magnitude = A4988_MIN_RATE_SPS +
                ((A4988_MAX_RATE_SPS - A4988_MIN_RATE_SPS) * distance) /
                A4988_SLOW_ZONE_MICROSTEPS;
        }
        desired = (error > 0) ? magnitude : -magnitude;
    }
    if (g_a4988Rate < desired) {
        g_a4988Rate += rateDelta;
        if (g_a4988Rate > desired) {
            g_a4988Rate = desired;
        }
    } else if (g_a4988Rate > desired) {
        g_a4988Rate -= rateDelta;
        if (g_a4988Rate < desired) {
            g_a4988Rate = desired;
        }
    }
}

static inline void a4988_timer_isr(void)
{
    int32_t rate;
    int32_t direction;

    if (g_a4988StepHigh) {
        DL_GPIO_clearPins(A4988_STEP_PORT, A4988_IO_STEP_PIN);
        g_a4988StepHigh = false;
    }
    if (!g_a4988Enabled || (g_a4988Position == g_a4988Target)) {
        return;
    }
    rate = g_a4988Rate;
    if (rate == 0) {
        return;
    }
    direction = (rate > 0) ? 1 : -1;
    if (((g_a4988Target - g_a4988Position) * direction) <= 0) {
        return;
    }
    if (direction != g_a4988Direction) {
        if (direction > 0) {
            DL_GPIO_setPins(A4988_DIR_PORT, A4988_IO_DIR_PIN);
        } else {
            DL_GPIO_clearPins(A4988_DIR_PORT, A4988_IO_DIR_PIN);
        }
        g_a4988Direction = direction;
        return;
    }
    if (a4988_home_active() && (direction < 0)) {
        g_a4988Position = 0;
        g_a4988Target = 0;
        g_a4988Rate = 0;
        return;
    }
    g_a4988Phase += (uint32_t) ((rate > 0) ? rate : -rate);
    if (g_a4988Phase < A4988_TICK_HZ) {
        return;
    }
    g_a4988Phase -= A4988_TICK_HZ;
    DL_GPIO_setPins(A4988_STEP_PORT, A4988_IO_STEP_PIN);
    g_a4988StepHigh = true;
    g_a4988Position += direction;
}

static inline int32_t a4988_position(void)
{
    return g_a4988Position;
}

static inline int32_t a4988_target(void)
{
    return g_a4988Target;
}

static inline bool a4988_enabled(void)
{
    return g_a4988Enabled;
}

static inline bool a4988_motion_pending(void)
{
    return g_a4988Enabled &&
        (g_a4988Position != g_a4988Target) && (g_a4988Rate != 0);
}

static inline void a4988_force_step_low(void)
{
    DL_GPIO_clearPins(A4988_STEP_PORT, A4988_IO_STEP_PIN);
    g_a4988StepHigh = false;
}

static inline bool a4988_home_active(void)
{
    return (DL_GPIO_readPins(A4988_HOME_PORT, A4988_IO_HOME_PIN) == 0U);
}

#endif
