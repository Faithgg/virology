#ifndef S0P_TASK_QUEUE_H
#define S0P_TASK_QUEUE_H

#include "../shared/protocol.h"

void task_queue_init(void);
void task_queue_enqueue(const char *session_id, const task_t *task);
int task_queue_dequeue(const char *session_id, task_t *task);

#endif
