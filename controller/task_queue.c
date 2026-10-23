#include "task_queue.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    task_t tasks[32];
    int head;
    int tail;
} task_queue_t;

static task_queue_t queues[S0P_MAX_SESSIONS];
static int queue_count = 0;

void task_queue_init(void) {
    memset(queues, 0, sizeof(queues));
    queue_count = 0;
}

static task_queue_t *get_or_create_queue(const char *session_id) {
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queues[i].session_id, session_id) == 0) {
            return &queues[i];
        }
    }
    if (queue_count >= S0P_MAX_SESSIONS) return NULL;
    task_queue_t *q = &queues[queue_count];
    strcpy(q->session_id, session_id);
    q->head = 0;
    q->tail = 0;
    queue_count++;
    return q;
}

void task_queue_enqueue(const char *session_id, const task_t *task) {
    task_queue_t *q = get_or_create_queue(session_id);
    if (!q || !task) return;

    int next = (q->tail + 1) % 32;
    if (next != q->head) {
        q->tasks[q->tail] = *task;
        q->tail = next;
    }
}

int task_queue_dequeue(const char *session_id, task_t *task) {
    task_queue_t *q = get_or_create_queue(session_id);
    if (!q || !task) return -1;

    if (q->head == q->tail) return -1;

    *task = q->tasks[q->head];
    q->head = (q->head + 1) % 32;
    return 0;
}
