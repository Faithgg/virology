#include "task_runner.h"
#include "modules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int task_runner_execute(const task_t *task, task_result_t *result) {
    if (!task || !result) return -1;

    memset(result, 0, sizeof(task_result_t));
    snprintf(result->task_id, sizeof(result->task_id), "%s", task->task_id);

    const module_def_t *mod = modules_find(task->command);
    if (!mod) {
        strcpy(result->status, "error");
        snprintf(result->output, sizeof(result->output),
                "Unknown command: %s", task->command);
        return -1;
    }

    char *output = mod->execute(task->args);
    if (output) {
        strcpy(result->status, "success");
        strncpy(result->output, output, sizeof(result->output) - 1);
        free(output);
    } else {
        strcpy(result->status, "error");
        strcpy(result->output, "Execution failed");
    }

    return 0;
}
