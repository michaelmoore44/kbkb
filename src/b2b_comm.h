#ifndef B2B_COMM_H
#define B2B_COMM_H

#include "common.h"

void b2b_comm_init(void);
void b2b_send_pend_msg(void);
bool b2b_comm_send_keys(bool force);
void b2b_check_for_msg(void);

#endif /* B2B_COMM_H */
