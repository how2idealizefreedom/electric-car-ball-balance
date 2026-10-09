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

/*
 * Seven working channels retain the original S1..S8 physical order. S6 is
 * damaged and is absent. Initial polarity follows both module descriptions:
 * reflective white=low comparator output, black line=high. A bit can be
 * cleared from LINE_BLACK_HIGH_MASK if bench telemetry proves that one
 * channel is black-low.
 */
#define LINE_PRESENT_MASK           (0xDFU) /* S1-S5,S7,S8 */
#define LINE_FAILED_MASK            (0x20U) /* physical S6 */
#define LINE_BLACK_HIGH_MASK        (0xDFU)
#define LINE_SENSOR_PITCH_MM        (15U)
#define LINE_S6_VIRTUAL_ERROR       (225)
#define LINE_S6_BRIDGE_MS           (75U)

/* The accepted 22:44/P1 line-following baseline. Tune one group at a time. */
#define PWM_PERIOD_COUNT            (1600U)
#define PWM_LIMIT                   (450)
#define PWM_BASE_STRAIGHT           (220)
#define PWM_BASE_MEDIUM_ERROR       (190)
#define PWM_BASE_LARGE_ERROR        (160)
#define PWM_SEARCH_INNER            (55)
#define PWM_SEARCH_OUTER            (150)
#define LINE_KP_NUM                 (8)
#define LINE_KP_DEN                 (15)
#define LINE_KD_NUM                 (18)
#define LINE_KD_DEN                 (15)
#define LINE_MEDIUM_ERROR_THRESHOLD (225)
#define LINE_LARGE_ERROR_THRESHOLD  (375)
#define TURN_LIMIT                  (400)

/* Verified direction convention from the working D24A/P1 combination. */
#define RIGHT_FORWARD_IN1_HIGH      (1)
#define LEFT_FORWARD_IN1_HIGH       (1)

/* Disabled until the corresponding physical interface is proven. */
#define MPU6050_BOOT_PROBE          (0)
#define OLED_BACKEND_READY          (1)
#define K230_UART_PROTOCOL_READY    (1)
#define BALL_AXIS_CALIBRATED        (1)
#define TRACK_B_DETECTOR_READY      (0)

/*
 * PB3 HOME is already configured as an active-low input with an internal
 * pull-up.  Keep automatic motion disabled until the switch has been wired
 * and verified with A4988 VMOT disconnected.
 */
#define A4988_AUTO_HOME_READY       (0)

/* Ball-control targets and completion thresholds from the problem statement. */
#define BALL_CENTER_TARGET_MM       (0)
#define BALL_POSITIVE_TARGET_MM     (50)
#define BALL_NEGATIVE_TARGET_MM     (-50)
#define BALL_ALLOWED_ERROR_MM       (10)
#define BALL_STABLE_SPEED_MMPS      (20)
#define BALL_STABLE_TIME_MS         (300U)
#define BALL_FEEDBACK_TIMEOUT_MS    (200U)

/*
 * Fast Q3 commissioning baseline:
 * manual HOME/lowest point = 0, measured near-level point = +320 microsteps.
 * The ball rolling toward the front at 320 is handled by closed-loop trim;
 * it does not prove that 320 is the final static level calibration.
 */
#define BALL_LEVEL_MICROSTEPS       (320)
#define BALL_STEPS_PER_DEG_NUM      (34)
#define BALL_STEPS_PER_DEG_DEN      (1)
#define BALL_KP_MDEG_PER_MM_NUM     (40)
#define BALL_KP_MDEG_PER_MM_DEN     (1)
#define BALL_KD_MDEG_PER_MMPS_NUM   (5)
#define BALL_KD_MDEG_PER_MMPS_DEN   (1)
#define BALL_MAX_TILT_MDEG          (2200)

/* Initial OpenMV VGA scale supplied by the user: about 26.5 pixels/cm. */
#define BALL_IMAGE_WIDTH_PIXELS             (640)
#define BALL_IMAGE_CENTER_X_PIXELS          (320)
#define BALL_PIXELS_PER_CM_NUM              (265)
#define BALL_PIXELS_PER_CM_DEN              (10)
/* Set to -1 if BE decreases when the ball moves toward the car rear. */
#define BALL_PIXEL_REAR_SIGN                (1)
/* Positive step offset currently lowers the rear and drives the ball rearward. */
#define BALL_ACTUATOR_SIGN                  (1)

/* A4988 service values. STEP timer runs only while motion is pending. */
#define A4988_TICK_HZ                       (10000U)
#define A4988_MAX_ABS_MICROSTEPS            (400)
#define A4988_MAX_RATE_SPS                  (1200)
#define A4988_MIN_RATE_SPS                  (120)
#define A4988_ACCEL_SPS2                    (3000)
#define A4988_SLOW_ZONE_MICROSTEPS          (80)
#define A4988_JOG_MICROSTEPS                (20)
#define A4988_HOME_SEARCH_TARGET            (-400)
#define A4988_HOME_TIMEOUT_MS               (4000U)
#define A4988_STEP_PORT                     A4988_IO_STEP_PORT
#define A4988_DIR_PORT                      A4988_IO_DIR_PORT
#define A4988_ENABLE_PORT                   A4988_IO_ENABLE_PORT
#define A4988_HOME_PORT                     A4988_IO_HOME_PORT

#endif /* CAR_CONFIG_H_ */
