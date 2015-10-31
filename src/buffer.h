#ifndef BUFFER_H
#define BUFFER_H

#include "common.h"

#define BUF_FLAG_OVERWRITE  (1<<0)

typedef struct
{
    uint8_t* buff;          // pointer to the buffer storage array
    uint8_t flags;
    uint32_t head; 			// write index
    uint32_t tail; 			// read index
    uint32_t count;		    // number of elements in array
    uint32_t size;          // size of the buffer
    uint32_t size_mask;     // size of the buffer
}buffer_t;

result buffer_init (buffer_t* buffer);

result buffer_write(buffer_t* buffer, uint8_t* value);

uint8_t buffer_read(buffer_t* buffer);

bool buffer_is_full(buffer_t* buffer);

bool buffer_is_empty(buffer_t* buffer);

#endif /* BUFFER_H */

