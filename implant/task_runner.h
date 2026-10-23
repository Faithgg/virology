/*
 * s0P0wn3d — Exécuteur de tâches de l'implant.
 *
 * Reçoit une tâche, trouve le module correspondant, l'exécute,
 * et construit le résultat.
 */

#ifndef S0P_TASK_RUNNER_H
#define S0P_TASK_RUNNER_H

#include "../shared/protocol.h"

/* Exécute une tâche et construit le résultat.
 * Le result->output est alloué dynamiquement par le module. */
void task_runner_execute(const task_t *task, task_result_t *result);

#endif /* S0P_TASK_RUNNER_H */
