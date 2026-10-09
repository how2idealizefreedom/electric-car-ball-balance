#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>
#include <stdint.h>

void display_init(void);
bool display_ready(void);
void display_show_mode(uint8_t question, const char *name);
void display_show_status(const char *status, uint32_t elapsedMs);
void display_service(void);

#endif /* DISPLAY_H_ */
