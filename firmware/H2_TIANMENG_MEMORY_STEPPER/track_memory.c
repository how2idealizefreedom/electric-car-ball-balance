#include "track_memory.h"
#include "car_config.h"

#include <stddef.h>

typedef struct {
    int16_t left;
    int16_t right;
} TrackSample;

static TrackSample g_samples[TRACK_MEMORY_MAX_SAMPLES];
static uint32_t g_sampleCount;
static uint32_t g_nextSampleMs;
static bool g_learning;
static bool g_valid;

static int16_t clamp_i16(int32_t value)
{
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return (int16_t) value;
}

void track_memory_clear(void)
{
    g_sampleCount = 0U;
    g_nextSampleMs = 0U;
    g_learning = false;
    g_valid = false;
}

void track_memory_begin_learning(void)
{
    track_memory_clear();
    g_learning = true;
}

void track_memory_cancel_learning(void)
{
    g_learning = false;
    g_valid = false;
    g_sampleCount = 0U;
}

void track_memory_record(uint32_t runMs, int32_t left, int32_t right)
{
    if (!g_learning || (g_sampleCount >= TRACK_MEMORY_MAX_SAMPLES)) {
        return;
    }

    while ((runMs >= g_nextSampleMs) &&
           (g_sampleCount < TRACK_MEMORY_MAX_SAMPLES)) {
        g_samples[g_sampleCount].left = clamp_i16(left);
        g_samples[g_sampleCount].right = clamp_i16(right);
        ++g_sampleCount;
        g_nextSampleMs += TRACK_MEMORY_SAMPLE_MS;
    }
}

bool track_memory_finish_learning(void)
{
    g_learning = false;
    g_valid = (g_sampleCount >= TRACK_MEMORY_MIN_SAMPLES);
    return g_valid;
}

bool track_memory_valid(void)
{
    return g_valid;
}

uint32_t track_memory_count(void)
{
    return g_sampleCount;
}

uint32_t track_memory_index(uint32_t runMs)
{
    return runMs / TRACK_MEMORY_SAMPLE_MS;
}

bool track_memory_get(
    uint32_t runMs, int32_t *left, int32_t *right, uint32_t *index)
{
    uint32_t sampleIndex = track_memory_index(runMs);

    if (!g_valid || (g_sampleCount == 0U) ||
        (left == NULL) || (right == NULL)) {
        return false;
    }
    if (sampleIndex >= g_sampleCount) {
        uint32_t recordedDurationMs =
            g_sampleCount * TRACK_MEMORY_SAMPLE_MS;
        if (runMs >= (recordedDurationMs + TRACK_MEMORY_END_GRACE_MS)) {
            return false;
        }
        sampleIndex = g_sampleCount - 1U;
    }
    *left = g_samples[sampleIndex].left;
    *right = g_samples[sampleIndex].right;
    if (index != NULL) {
        *index = sampleIndex;
    }
    return true;
}
