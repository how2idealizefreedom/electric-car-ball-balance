#include "ball_feedback.h"
#include "car_config.h"

#define BALL_RX_RING_SIZE       (128U)
#define BALL_FRAME_MAX_LENGTH   (63U)

static volatile BallFeedback g_feedback;
static volatile BallRawFrame g_rawFrame;
static volatile uint8_t g_rxRing[BALL_RX_RING_SIZE];
static volatile uint8_t g_rxHead;
static volatile uint8_t g_rxTail;
static volatile uint32_t g_rxOverflowCount;
static char g_frameBuffer[BALL_FRAME_MAX_LENGTH + 1U];
static uint8_t g_frameLength;

static bool parse_integer(const char **cursor, int32_t *value)
{
    const char *text = *cursor;
    int32_t result = 0;
    int32_t sign = 1;
    bool haveDigit = false;

    if (*text == '-') {
        sign = -1;
        ++text;
    }
    while ((*text >= '0') && (*text <= '9')) {
        haveDigit = true;
        result = result * 10 + (int32_t) (*text - '0');
        ++text;
    }
    if (!haveDigit) {
        return false;
    }
    *value = result * sign;
    *cursor = text;
    return true;
}

static bool consume_comma(const char **cursor)
{
    if (**cursor != ',') {
        return false;
    }
    ++(*cursor);
    return true;
}

static bool parse_frame(uint32_t nowMs)
{
    const char *cursor = g_frameBuffer;
    int32_t valid;
    int32_t x;
    int32_t error;
    int32_t velocity;
    int32_t lost;

    if ((cursor[0] != '$') || (cursor[1] != 'B') || (cursor[2] != ',')) {
        return false;
    }
    cursor += 3;
    if (!parse_integer(&cursor, &valid) || !consume_comma(&cursor) ||
        !parse_integer(&cursor, &x) || !consume_comma(&cursor) ||
        !parse_integer(&cursor, &error) || !consume_comma(&cursor) ||
        !parse_integer(&cursor, &velocity) || !consume_comma(&cursor) ||
        !parse_integer(&cursor, &lost) || (*cursor != '\0')) {
        return false;
    }
    if (((valid != 0) && (valid != 1)) || (x < 0) || (x > 319) ||
        (error < -320) || (error > 320) ||
        (velocity < -320) || (velocity > 320) ||
        (lost < 0) || (lost > 100000)) {
        return false;
    }

    g_rawFrame.valid = valid;
    g_rawFrame.xPixels = x;
    g_rawFrame.errorPixels = error;
    g_rawFrame.velocityPixelsPerFrame = velocity;
    g_rawFrame.lostFrames = lost;
    g_rawFrame.timestampMs = nowMs;
    ++g_rawFrame.sequence;
    return true;
}

void ball_feedback_init(void)
{
    g_feedback.positionMm = 0;
    g_feedback.velocityMmPerSecond = 0;
    g_feedback.confidence = 0U;
    g_feedback.timestampMs = 0U;
    g_feedback.valid = false;
    g_rawFrame.valid = 0;
    g_rawFrame.xPixels = 0;
    g_rawFrame.errorPixels = 0;
    g_rawFrame.velocityPixelsPerFrame = 0;
    g_rawFrame.lostFrames = 0;
    g_rawFrame.timestampMs = 0U;
    g_rawFrame.sequence = 0U;
    g_rxHead = 0U;
    g_rxTail = 0U;
    g_rxOverflowCount = 0U;
    g_frameLength = 0U;
}

void ball_feedback_rx_isr_byte(uint8_t byte)
{
    uint8_t next = (uint8_t) ((g_rxHead + 1U) % BALL_RX_RING_SIZE);

    if (next == g_rxTail) {
        ++g_rxOverflowCount;
        return;
    }
    g_rxRing[g_rxHead] = byte;
    g_rxHead = next;
}

void ball_feedback_service(uint32_t nowMs)
{
    while (g_rxTail != g_rxHead) {
        char character = (char) g_rxRing[g_rxTail];
        g_rxTail = (uint8_t) ((g_rxTail + 1U) % BALL_RX_RING_SIZE);

        if (character == '$') {
            g_frameLength = 0U;
            g_frameBuffer[g_frameLength++] = character;
        } else if (g_frameLength == 0U) {
            continue;
        } else if (character == '\n') {
            g_frameBuffer[g_frameLength] = '\0';
            (void) parse_frame(nowMs);
            g_frameLength = 0U;
        } else if (character != '\r') {
            if (g_frameLength < BALL_FRAME_MAX_LENGTH) {
                g_frameBuffer[g_frameLength++] = character;
            } else {
                g_frameLength = 0U;
            }
        }
    }
}

bool ball_feedback_raw_latest(uint32_t nowMs, BallRawFrame *frame)
{
    BallRawFrame snapshot;

    snapshot.valid = g_rawFrame.valid;
    snapshot.xPixels = g_rawFrame.xPixels;
    snapshot.errorPixels = g_rawFrame.errorPixels;
    snapshot.velocityPixelsPerFrame = g_rawFrame.velocityPixelsPerFrame;
    snapshot.lostFrames = g_rawFrame.lostFrames;
    snapshot.timestampMs = g_rawFrame.timestampMs;
    snapshot.sequence = g_rawFrame.sequence;
    if ((snapshot.sequence == 0U) ||
        ((nowMs - snapshot.timestampMs) > BALL_FEEDBACK_TIMEOUT_MS)) {
        return false;
    }
    *frame = snapshot;
    return true;
}

void ball_feedback_update(int32_t positionMm, int32_t velocityMmPerSecond,
    uint8_t confidence, uint32_t timestampMs)
{
    g_feedback.valid = false;
    g_feedback.positionMm = positionMm;
    g_feedback.velocityMmPerSecond = velocityMmPerSecond;
    g_feedback.confidence = confidence;
    g_feedback.timestampMs = timestampMs;
    g_feedback.valid = true;
}

bool ball_feedback_latest(uint32_t nowMs, BallFeedback *feedback)
{
    BallFeedback snapshot;

    snapshot.positionMm = g_feedback.positionMm;
    snapshot.velocityMmPerSecond = g_feedback.velocityMmPerSecond;
    snapshot.confidence = g_feedback.confidence;
    snapshot.timestampMs = g_feedback.timestampMs;
    snapshot.valid = g_feedback.valid;
    if (!snapshot.valid ||
        ((nowMs - snapshot.timestampMs) > BALL_FEEDBACK_TIMEOUT_MS)) {
        return false;
    }
    *feedback = snapshot;
    return true;
}
