#ifndef BALL_FEEDBACK_H_
#define BALL_FEEDBACK_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int32_t positionMm;
    int32_t velocityMmPerSecond;
    uint8_t confidence;
    uint32_t timestampMs;
    bool valid;
} BallFeedback;

typedef struct {
    int32_t valid;
    int32_t xPixels;
    int32_t errorPixels;
    int32_t velocityPixelsPerFrame;
    int32_t lostFrames;
    uint32_t timestampMs;
    uint32_t sequence;
} BallRawFrame;

void ball_feedback_init(void);
void ball_feedback_rx_isr_byte(uint8_t byte);
void ball_feedback_service(uint32_t nowMs);
bool ball_feedback_raw_latest(uint32_t nowMs, BallRawFrame *frame);
void ball_feedback_update(int32_t positionMm, int32_t velocityMmPerSecond,
    uint8_t confidence, uint32_t timestampMs);
bool ball_feedback_latest(uint32_t nowMs, BallFeedback *feedback);

#endif /* BALL_FEEDBACK_H_ */
