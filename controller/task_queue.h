/*
 * s0P0wn3d — File de tâches (côté contrôleur).
 *
 * FIFO thread-safe par session.
 */

#ifndef S0P_TASK_QUEUE_H
#define S0P_TASK_QUEUE_H

#include <pthread.h>
#include "../shared/protocol.h"

/* Initialise la file de tâches. */
int task_queue_init(void);

/* Ajoute une tâche à la file d'une session. */
int task_queue_enqueue(const char *session_id, const task_t *task);

/* Récupère la prochaine tâche pour une session (FIFO).
 * Retourne 0 si une tâche a été récupérée, -1 sinon. */
int task_queue_dequeue(const char *session_id, task_t *out);

/* Retourne le nombre de tâches en attente pour une session. */
int task_queue_pending(const char *session_id);

#endif /* S0P_TASK_QUEUE_H */
