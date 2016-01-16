#ifndef __KEYS_H
#define __KEYS_H

#include "stm32f4xx_hal.h"
#include "common.h"

#define NUM_KEY_BYTES (5)

void keys_init(void);
void keys_scan(void);
void keys_received(uint8_t* buf, uint8_t len);
void keys_get_keys(uint8_t* buf);
void keys_translate(uint8_t* buf);


#endif /* __KEYS_H */
