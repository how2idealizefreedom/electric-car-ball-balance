#include "ball_controller.h"
#include "a4988.h"
#include "ball_feedback.h"
#include "car_config.h"

static int32_t g_targetMm;

void ball_controller_init(void)
{
    g_targetMm = BALL_CENTER_TARGET_MM;
}

bool ball_controller_capable(void)
{
    return (K230_UART_PROTOCOL_READY != 0) &&
        (BALL_AXIS_CALIBRATED != 0);
}

void ball_controller_set_target(int32_t positionMm)
{
    g_targetMm = positionMm;
}

bool ball_controller_step(uint32_t nowMs)
{
    BallFeedback feedback;
    int32_t errorMm;
    int32_t tiltMdeg;
    int32_t stepOffset;

    if (!ball_controller_capable() ||
        !ball_feedback_latest(nowMs, &feedback)) {
        a4988_enable(false);
        return false;
    }
    errorMm = g_targetMm - feedback.positionMm;
    tiltMdeg =
        (errorMm * BALL_KP_MDEG_PER_MM_NUM) /
            BALL_KP_MDEG_PER_MM_DEN -
        (feedback.velocityMmPerSecond * BALL_KD_MDEG_PER_MMPS_NUM) /
            BALL_KD_MDEG_PER_MMPS_DEN;
    if (tiltMdeg > BALL_MAX_TILT_MDEG) {
        tiltMdeg = BALL_MAX_TILT_MDEG;
    } else if (tiltMdeg < -BALL_MAX_TILT_MDEG) {
        tiltMdeg = -BALL_MAX_TILT_MDEG;
    }
    stepOffset = (tiltMdeg * BALL_STEPS_PER_DEG_NUM) /
        (1000 * BALL_STEPS_PER_DEG_DEN);
    stepOffset *= BALL_ACTUATOR_SIGN;
    a4988_enable(true);
    a4988_set_target(BALL_LEVEL_MICROSTEPS + stepOffset);
    return true;
}

void ball_controller_disable(void)
{
    a4988_enable(false);
}

int32_t ball_controller_target(void)
{
    return g_targetMm;
}
