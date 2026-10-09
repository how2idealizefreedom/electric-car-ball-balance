#include "ball_feedback.h"
#include "car_config.h"

#define BALL_RX_RING_SIZE       (128U)
#define BALL_FRAME_MAX_LENGTH   (95U)

static volatile BallFeedback g_feedback;
static volatile BallRawFrame g_rawFrame;
static volatile uint8_t g_rxRing[BALL_RX_RING_SIZE];
static volatile uint8_t g_rxHead;
static volatile uint8_t g_rxTail;
static volatile uint32_t g_rxOverflowCount;
static char g_frameBuffer[BALL_FRAME_MAX_LENGTH + 1U];
static uint8_t g_frameLength;
static int32_t g_previousPositionMm;
static uint32_t g_previousPositionMs;
static bool g_havePreviousPosition;

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

static int32_t pixels_to_mm(int32_t xPixels)
{
    int32_t numerator = BALL_PIXEL_REAR_SIGN *
        (xPixels - BALL_IMAGE_CENTER_X_PIXELS) * 10 *
        BALL_PIXELS_PER_CM_DEN;

    if (numerator >= 0) {
        numerator += BALL_PIXELS_PER_CM_NUM / 2;
    } else {
        numerator -= BALL_PIXELS_PER_CM_NUM / 2;
    }
    return numerator / BALL_PIXELS_PER_CM_NUM;
}

static void update_scaled_feedback(int32_t valid, int32_t xPixels,
    int32_t lostFrames, uint32_t nowMs)
{
    int32_t positionMm;
    int32_t velocityMmPerSecond = 0;
    uint8_t confidence;

    if (valid == 0) {
        g_feedback.valid = false;
        g_havePreviousPosition = false;
        return;
    }

    positionMm = pixels_to_mm(xPixels);
    if (g_havePreviousPosition && (nowMs > g_previousPositionMs)) {
        uint32_t deltaMs = nowMs - g_previousPositionMs;
        int32_t instantaneous =
            ((positionMm - g_previousPositionMm) * 1000) /
                (int32_t) deltaMs;

        instantaneous = clamp_i32(instantaneous, -2000, 2000);
        if (g_feedback.valid) {
            velocityMmPerSecond =
                (3 * g_feedback.velocityMmPerSecond + instantaneous) / 4;
        } else {
            velocityMmPerSecond = instantaneous;
        }
    }
    confidence = (lostFrames == 0) ? 100U :
        ((lostFrames <= 2) ? 70U : 40U);
    ball_feedback_update(
        positionMm, velocityMmPerSecond, confidence, nowMs);
    g_previousPositionMm = positionMm;
    g_previousPositionMs = nowMs;
    g_havePreviousPosition = true;
}

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

static bool consume_text(const char **cursor, const char *expected)
{
    const char *text = *cursor;

    while (*expected != '\0') {
        if (*text != *expected) {
            return false;
        }
        ++text;
        ++expected;
    }
    *cursor = text;
    return true;
}

static bool parse_compact_frame(const char **cursor, int32_t *valid,
    int32_t *x, int32_t *error, int32_t *velocity, int32_t *lost)
{
    if (!consume_text(cursor, "$B,") ||
        !parse_integer(cursor, valid) || !consume_comma(cursor) ||
        !parse_integer(cursor, x) || !consume_comma(cursor) ||
        !parse_integer(cursor, error) || !consume_comma(cursor) ||
        !parse_integer(cursor, velocity) || !consume_comma(cursor) ||
        !parse_integer(cursor, lost) || (**cursor != '\0')) {
        return false;
    }
    return true;
}

static bool parse_openmv_labeled_frame(const char **cursor, int32_t *valid,
    int32_t *x, int32_t *error, int32_t *velocity, int32_t *lost)
{
    int32_t online;
    int32_t sourceSequence;

    if (!consume_text(cursor, "K230=") ||
        !parse_integer(cursor, &online) ||
        !consume_text(cursor, ",BV=") ||
        !parse_integer(cursor, valid) ||
        !consume_text(cursor, ",BX=") ||
        !parse_integer(cursor, x) ||
        !consume_text(cursor, ",BE=") ||
        !parse_integer(cursor, error) ||
        !consume_text(cursor, ",BVX=") ||
        !parse_integer(cursor, velocity) ||
        !consume_text(cursor, ",BL=") ||
        !parse_integer(cursor, lost) ||
        !consume_text(cursor, ",BSEQ=") ||
        !parse_integer(cursor, &sourceSequence) || (**cursor != '\0')) {
        return false;
    }
    return (online == 1) && (sourceSequence >= 0);
}

static bool parse_frame(uint32_t nowMs)
{
    const char *cursor = g_frameBuffer;
    int32_t valid;
    int32_t x;
    int32_t error;
    int32_t velocity;
    int32_t lost;

    if (!parse_compact_frame(&cursor, &valid, &x, &error, &velocity, &lost)) {
        cursor = g_frameBuffer;
        if (!parse_openmv_labeled_frame(
            &cursor, &valid, &x, &error, &velocity, &lost)) {
            return false;
        }
    }
    if (((valid != 0) && (valid != 1)) || (x < 0) ||
        (x >= BALL_IMAGE_WIDTH_PIXELS) ||
        (error < -BALL_IMAGE_WIDTH_PIXELS) ||
        (error > BALL_IMAGE_WIDTH_PIXELS) ||
        (velocity < -BALL_IMAGE_WIDTH_PIXELS) ||
        (velocity > BALL_IMAGE_WIDTH_PIXELS) ||
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
    update_scaled_feedback(valid, x, lost, nowMs);
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
    g_previousPositionMm = 0;
    g_previousPositionMs = 0U;
    g_havePreviousPosition = false;
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

        if ((character == '$') || (character == 'K')) {
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
