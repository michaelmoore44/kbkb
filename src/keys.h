#ifndef __KEYS_H
#define __KEYS_H

#include "stm32f4xx_hal.h"
#include "common.h"

void keys_init(void);
void keys_scan(void);
void keys_translate(uint8_t* buf);


#endif /* __KEYS_H */
