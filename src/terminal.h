#ifndef TERMINAL_H
#define TERMINAL_H

#include "common.h"

void term_init(void);
result term_putchar(uint8_t* data);
uint32_t term_puts(uint8_t* data, uint32_t len);
uint32_t print(const char *format, ...);

#endif /* TERMINAL_H */
