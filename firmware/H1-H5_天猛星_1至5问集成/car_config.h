#ifndef CAR_CONFIG_H_
#define CAR_CONFIG_H_

/* Scheduler and race timing. */
#define CONTROL_PERIOD_MS           (5U)
#define TELEMETRY_PERIOD_MS         (100U)
#define MIN_FINISH_TIME_MS          (6000U)
#define FINISH_MARKER_MIN_ACTIVE    (4U)
#define FINISH_MARKER_TICKS         (3U)
#define BRAKE_TIME_MS               (220U)
#define LOST_LINE_FAULT_MS          (400U)
#define Q3_TIME_LIMIT_MS            (5000U)
#define Q4_TIME_LIMIT_MS            (8000U)
#define Q5_TIME_LIMIT_MS            (30000U)

/* Direct measurement: white background=low, black line=high. */
#define LINE_BLACK_IS_HIGH          (1)

/* The accepted 22:44/P1 line-following baseline. Tune one group at a time. */
#define PWM_PERIOD_COUNT            (1600U)
#define PWM_LIMIT                   (350)
#define PWM_BASE_STRAIGHT           (220)
#define PWM_BASE_MEDIUM_ERROR       (190)
#define PWM_BASE_LARGE_ERROR        (160)
#define PWM_SEARCH_INNER            (55)
#define PWM_SEARCH_OUTER            (150)
#define LINE_KP_NUM                 (8)
#define LINE_KP_DEN                 (10)
#define LINE_KD_NUM                 (18)
#define LINE_KD_DEN                 (10)
#define TURN_LIMIT                  (400)

/* Verified direction convention from the working D24A/P1 combination. */
#define RIGHT_FORWARD_IN1_HIGH      (1)
#define LEFT_FORWARD_IN1_HIGH       (1)

/* Disabled until the corresponding physical interface is proven. */
#define MPU6050_BOOT_PROBE          (0)
#define OLED_BACKEND_READY          (1)
#define K230_UART_PROTOCOL_READY    (1)
#define BALL_AXIS_CALIBRATED        (0)
#define TRACK_B_DETECTOR_READY      (0)

/* Ball-control targets and completion thresholds from the problem statement. */
#define BALL_CENTER_TARGET_MM       (0)
#define BALL_POSITIVE_TARGET_MM     (50)
#define BALL_NEGATIVE_TARGET_MM     (-50)
#define BALL_ALLOWED_ERROR_MM       (10)
#define BALL_STABLE_SPEED_MMPS      (20)
#define BALL_STABLE_TIME_MS         (300U)
#define BALL_FEEDBACK_TIMEOUT_MS    (200U)

/* Safe placeholders; keep BALL_AXIS_CALIBRATED=0 until measured. */
#define BALL_LEVEL_MICROSTEPS       (0)
#define BALL_STEPS_PER_DEG_NUM      (0)
#define BALL_STEPS_PER_DEG_DEN      (1)
#define BALL_KP_MDEG_PER_MM_NUM     (0)
#define BALL_KP_MDEG_PER_MM_DEN     (1)
#define BALL_KD_MDEG_PER_MMPS_NUM   (0)
#define BALL_KD_MDEG_PER_MMPS_DEN   (1)
#define BALL_MAX_TILT_MDEG          (0)

/* A4988 service values. STEP timer runs only while motion is pending. */
#define A4988_TICK_HZ                       (10000U)
#define A4988_MAX_ABS_MICROSTEPS            (400)
#define A4988_MAX_RATE_SPS                  (1200)
#define A4988_MIN_RATE_SPS                  (120)
#define A4988_ACCEL_SPS2                    (3000)
#define A4988_SLOW_ZONE_MICROSTEPS          (80)
#define A4988_JOG_MICROSTEPS                (20)
#define A4988_STEP_PORT                     A4988_IO_STEP_PORT
#define A4988_DIR_PORT                      A4988_IO_DIR_PORT
#define A4988_ENABLE_PORT                   A4988_IO_ENABLE_PORT
#define A4988_HOME_PORT                     A4988_IO_HOME_PORT

#endif /* CAR_CONFIG_H_ */
