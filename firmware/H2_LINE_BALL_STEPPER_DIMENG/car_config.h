#ifndef CAR_CONFIG_H_
#define CAR_CONFIG_H_

/* Timing */
#define CONTROL_PERIOD_MS          (5U)
#define TELEMETRY_PERIOD_MS        (100U)
#define MIN_FINISH_TIME_MS         (6000U)
#define BRAKE_TIME_MS              (220U)
#define LOST_LINE_FAULT_MS         (400U)

/* Direct white-background/black-line test: white=low, black=high. */
#define LINE_BLACK_IS_HIGH         (1)

/* PWM is expressed in permille: 0..1000. */
#define PWM_PERIOD_COUNT           (1600U)
#define PWM_LIMIT                  (350)
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

/* Change either value to 0 if that side runs backward during the test. */
#define RIGHT_FORWARD_IN1_HIGH     (1)
#define LEFT_FORWARD_IN1_HIGH      (1)

#define A4988_BRINGUP_MODE                 (1)
#define A4988_TICK_HZ                      (10000U)
#define A4988_MAX_ABS_MICROSTEPS           (400)
#define A4988_MAX_RATE_SPS                 (1200)
#define A4988_MIN_RATE_SPS                 (120)
#define A4988_ACCEL_SPS2                   (3000)
#define A4988_SLOW_ZONE_MICROSTEPS         (80)
#define A4988_TEST_TRAVEL_MICROSTEPS       (20)
#define A4988_STEP_PORT                    A4988_IO_PORT
#define A4988_DIR_PORT                     A4988_IO_PORT
#define A4988_ENABLE_PORT                  A4988_IO_PORT
#define A4988_HOME_PORT                    A4988_IO_PORT

#endif /* CAR_CONFIG_H_ */
