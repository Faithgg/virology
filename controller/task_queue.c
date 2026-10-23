/*
 * s0P0wn3d — Implémentation de la file de tâches.
 */

#include "task_queue.h"
#include <string.h>
#include <stdio.h>

#define MAX_TASKS_PER_SESSION 64

typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    task_t tasks[MAX_TASKS_PER_SESSION];
    int head;
    int tail;
    int count;
} session_queue_t;

static session_queue_t queues[S0P_MAX_SESSIONS];
static int queue_count = 0;
static pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;

int task_queue_init(void) {
    memset(queues, 0, sizeof(queues));
    queue_count = 0;
    return 0;
}

static session_queue_t *find_or_create_queue(const char *session_id) {
    /* Chercher une file existante */
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queues[i].session_id, session_id) == 0) {
            return &queues[i];
        }
    }
    /* Créer une nouvelle file */
    if (queue_count >= S0P_MAX_SESSIONS) return NULL;
    session_queue_t *q = &queues[queue_count];
    strncpy(q->session_id, session_id, S0P_MAX_SESSION_ID - 1);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    queue_count++;
    return q;
}

int task_queue_enqueue(const char *session_id, const task_t *task) {
    pthread_mutex_lock(&queue_lock);
    session_queue_t *q = find_or_create_queue(session_id);
    if (!q || q->count >= MAX_TASKS_PER_SESSION) {
        pthread_mutex_unlock(&queue_lock);
        return -1;
    }
    q->tasks[q->tail] = *task;
    q->tail = (q->tail + 1) % MAX_TASKS_PER_SESSION;
    q->count++;
    pthread_mutex_unlock(&queue_lock);
    return 0;
}

int task_queue_dequeue(const char *session_id, task_t *out) {
    pthread_mutex_lock(&queue_lock);
    session_queue_t *q = find_or_create_queue(session_id);
    if (!q || q->count == 0) {
        pthread_mutex_unlock(&queue_lock);
        return -1;
    }
    *out = q->tasks[q->head];
    q->head = (q->head + 1) % MAX_TASKS_PER_SESSION;
    q->count--;
    pthread_mutex_unlock(&queue_lock);
    return 0;
}

int task_queue_pending(const char *session_id) {
    pthread_mutex_lock(&queue_lock);
    int count = 0;
    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queues[i].session_id, session_id) == 0) {
            count = queues[i].count;
            break;
        }
    }
    pthread_mutex_unlock(&queue_lock);
    return count;
}
