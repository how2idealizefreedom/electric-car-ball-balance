#ifndef CAR_CONFIG_H_
#define CAR_CONFIG_H_

/* Timing */
#define CONTROL_PERIOD_MS          (5U)
#define TELEMETRY_PERIOD_MS        (100U)
#define MIN_FINISH_TIME_MS         (6000U)
#define BRAKE_TIME_MS              (220U)
#define LOST_LINE_STAGE2_MS        (180U)
#define LOST_LINE_FAULT_MS         (1100U)

/* Direct white-background/black-line test: white=low, black=high. */
#define LINE_BLACK_IS_HIGH         (1)

/* PWM is expressed in permille: 0..1000. */
#define PWM_PERIOD_COUNT           (1600U)
#define PWM_LIMIT                  (350)
#define PWM_BASE_STRAIGHT          (220)
#define PWM_BASE_MEDIUM_ERROR      (190)
#define PWM_BASE_LARGE_ERROR       (160)
#define PWM_SEARCH_GENTLE_INNER    (30)
#define PWM_SEARCH_GENTLE_OUTER    (180)
#define PWM_SEARCH_HARD_INNER      (0)
#define PWM_SEARCH_HARD_OUTER      (260)

/* Line error positions are -350..+350. */
#define LINE_KP_NUM                (7)
#define LINE_KP_DEN                (10)
#define LINE_KD_NUM                (18)
#define LINE_KD_DEN                (10)
#define TURN_LIMIT                 (400)

/* Change either value to 0 if that side runs backward during the test. */
#define RIGHT_FORWARD_IN1_HIGH     (1)
#define LEFT_FORWARD_IN1_HIGH      (1)

#endif /* CAR_CONFIG_H_ */
