#ifndef TRACK_MEMORY_H_
#define TRACK_MEMORY_H_

#include <stdbool.h>
#include <stdint.h>

void track_memory_clear(void);
void track_memory_begin_learning(void);
void track_memory_cancel_learning(void);
void track_memory_record(uint32_t runMs, int32_t left, int32_t right);
bool track_memory_finish_learning(void);
bool track_memory_valid(void);
uint32_t track_memory_count(void);
uint32_t track_memory_index(uint32_t runMs);
bool track_memory_get(
    uint32_t runMs, int32_t *left, int32_t *right, uint32_t *index);

#endif /* TRACK_MEMORY_H_ */
