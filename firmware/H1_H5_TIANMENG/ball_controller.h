#ifndef BALL_CONTROLLER_H_
#define BALL_CONTROLLER_H_

#include <stdbool.h>
#include <stdint.h>

void ball_controller_init(void);
bool ball_controller_capable(void);
void ball_controller_set_target(int32_t positionMm);
bool ball_controller_step(uint32_t nowMs);
void ball_controller_disable(void);
int32_t ball_controller_target(void);

#endif /* BALL_CONTROLLER_H_ */
