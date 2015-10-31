
#include "buffer.h"


result buffer_init (buffer_t* buffer)
{
	buffer->size_mask = buffer->size - 1;
	buffer->head = buffer->tail = 0;
	buffer->count = 0;
	return OK;
}

result buffer_write(buffer_t* buffer, uint8_t* value)
{
	result ret_val = FAIL;
	if(!buffer_is_full(buffer) || (buffer->flags & BUF_FLAG_OVERWRITE))
	{
		if(buffer_is_full(buffer))
		{
			//move the tail
			++(buffer->tail);
			buffer->tail &= buffer->size_mask;
		}
		else
			++(buffer->count);
		buffer->buff[buffer->head++] = *value;
		buffer->head &= buffer->size_mask;
		ret_val = OK;
	}
	return ret_val;
}

uint8_t buffer_read(buffer_t* buffer)
{
	uint8_t data;

	data = buffer->buff[(buffer->tail)++];
	buffer->tail &= buffer->size_mask;
	--(buffer->count);

    return data;
}

bool buffer_is_full(buffer_t* buffer)
{
    return buffer->count == buffer->size;
}

bool buffer_is_empty(buffer_t* buffer)
{
    return buffer->count == 0;
}

