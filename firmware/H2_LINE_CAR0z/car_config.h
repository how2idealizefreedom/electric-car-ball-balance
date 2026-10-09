#ifndef CAR_CONFIG_H_
#define CAR_CONFIG_H_

/* ========== Timing ========== */
#define CONTROL_PERIOD_MS          (5U)
#define TELEMETRY_PERIOD_MS        (100U)
#define MIN_FINISH_TIME_MS         (6000U)
#define BRAKE_TIME_MS              (220U)
#define LOST_LINE_FAULT_MS         (400U)

/* ========== Line sensor polarity ========== */
/* Direct white-background/black-line test: white=low, black=high. */
#define LINE_BLACK_IS_HIGH         (1)

/* ========== Motor PWM ========== */
/* PWM is expressed in permille: 0..1000. */
#define PWM_PERIOD_COUNT           (1600U)
#define PWM_LIMIT                  (350)

/* ========== General mode: speed and PD ========== */
#define PWM_BASE_STRAIGHT          (220)
#define PWM_BASE_MEDIUM_ERROR      (190)
#define PWM_BASE_LARGE_ERROR       (160)
#define PWM_SEARCH_INNER           (55)
#define PWM_SEARCH_OUTER           (150)

/* Line error positions are -350..+350. */
#define LINE_KP_NUM                (7)
#define LINE_KP_DEN                (10)
#define LINE_KD_NUM                (18)
#define LINE_KD_DEN                (10)
#define TURN_LIMIT                 (400)

/* ========== Mode switching ========== */
/* Default mode on startup: 0 = General, 1 = Track */
#define CAR_MODE_DEFAULT           (0)

/* Long-press duration to start the car (ms). Short press toggles mode. */
#define LONG_PRESS_MS              (1000U)

/* ========== Track mode: oval track parameters ========== */
/*
 * Track geometry: two 1.5m parallel straights + two 0.5m radius semicircles.
 * Car starts from a straight segment.
 *
 * Curve direction: -1 = left turns (counter-clockwise), 1 = right turns (clockwise).
 * Set to match the direction the car runs on the oval.
 */
#define TRACK_CURVE_DIR            (1)

/* Pre-brake time before entering a curve (ms). Linearly ramps PWM down. */
#define TRACK_PRE_BRAKE_MS         (200U)

/* Track mode: straight segment parameters (higher speed, lower Kp) */
#define TRACK_STRAIGHT_BASE_PWM    (380)
#define TRACK_STRAIGHT_KP_NUM      (5)
#define TRACK_STRAIGHT_KP_DEN      (10)
#define TRACK_STRAIGHT_KD_NUM      (10)
#define TRACK_STRAIGHT_KD_DEN      (10)

/* Track mode: curve segment parameters (lower speed, higher Kd) */
#define TRACK_CURVE_BASE_PWM       (170)
#define TRACK_CURVE_KP_NUM         (10)
#define TRACK_CURVE_KP_DEN         (10)
#define TRACK_CURVE_KD_NUM         (25)
#define TRACK_CURVE_KD_DEN         (10)

/*
 * Estimated segment boundaries in ms.
 * CALIBRATION NEEDED: run the car in General mode and use telemetry
 * timestamps to determine when each segment transition occurs.
 *
 * Segment 0: Straight 1 (start)
 * Segment 1: Semicircle 1
 * Segment 2: Straight 2
 * Segment 3: Semicircle 2
 */
#define TRACK_SEG0_END_MS          (2000U)
#define TRACK_SEG1_END_MS          (4500U)
#define TRACK_SEG2_END_MS          (6500U)
#define TRACK_SEG3_END_MS          (9000U)

/* ========== Motor polarity ========== */
/* Change either value to 0 if that side runs backward during the test. */
#define RIGHT_FORWARD_IN1_HIGH     (1)
#define LEFT_FORWARD_IN1_HIGH      (1)

#endif /* CAR_CONFIG_H_ */
