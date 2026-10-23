#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *json_extract_string(const char *json, const char *key) {
    if (!json || !key) return NULL;

    char search[256];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    const char *start = strstr(json, search);
    if (!start) return NULL;

    start += strlen(search);
    const char *end = strchr(start, '"');
    if (!end) return NULL;

    size_t len = end - start;
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }
    return result;
}

char *json_extract_nested(const char *json, const char *outer, const char *inner) {
    if (!json || !outer || !inner) return NULL;

    char outer_search[256];
    snprintf(outer_search, sizeof(outer_search), "\"%s\":", outer);
    const char *outer_start = strstr(json, outer_search);
    if (!outer_start) return NULL;

    outer_start = strchr(outer_start, '{');
    if (!outer_start) return NULL;

    char inner_search[256];
    snprintf(inner_search, sizeof(inner_search), "\"%s\":\"", inner);
    const char *start = strstr(outer_start, inner_search);
    if (!start) return NULL;

    start += strlen(inner_search);
    const char *end = strchr(start, '"');
    if (!end) return NULL;

    size_t len = end - start;
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }
    return result;
}

int json_parse_result(const char *json, task_result_t *result) {
    if (!json || !result) return -1;

    memset(result, 0, sizeof(task_result_t));

    char *task_id = json_extract_string(json, "task_id");
    char *status = json_extract_string(json, "status");
    char *output = json_extract_string(json, "output");

    if (task_id) {
        strncpy(result->task_id, task_id, sizeof(result->task_id) - 1);
        free(task_id);
    }
    if (status) {
        strncpy(result->status, status, sizeof(result->status) - 1);
        free(status);
    }
    if (output) {
        strncpy(result->output, output, sizeof(result->output) - 1);
        free(output);
    }

    return 0;
}

int json_parse_tasks(const char *json, task_t *tasks, int max_count) {
    if (!json || !tasks || max_count <= 0) return 0;

    int count = 0;
    const char *search = "\"task_id\":\"";
    const char *pos = json;

    while (count < max_count && (pos = strstr(pos, search))) {
        memset(&tasks[count], 0, sizeof(task_t));

        pos += strlen(search);
        const char *end = strchr(pos, '"');
        if (!end) break;

        size_t len = end - pos;
        if (len < sizeof(tasks[count].task_id)) {
            memcpy(tasks[count].task_id, pos, len);
            tasks[count].task_id[len] = '\0';
        }

        char *cmd = json_extract_string(pos, "command");
        char *args = json_extract_string(pos, "args");

        if (cmd) {
            strncpy(tasks[count].command, cmd, sizeof(tasks[count].command) - 1);
            free(cmd);
        }
        if (args) {
            strncpy(tasks[count].args, args, sizeof(tasks[count].args) - 1);
            free(args);
        }

        count++;
        pos = end + 1;
    }

    return count;
}

char *json_build_ack(void) {
    char *json = malloc(64);
    if (json) {
        sprintf(json, "{\"status\":\"ack\"}");
    }
    return json;
}

char *json_build_sessions(const session_t *sessions, int count) {
    if (!sessions || count <= 0) return NULL;

    char *json = malloc(4096);
    if (!json) return NULL;

    int offset = 0;
    offset += sprintf(json + offset, "{\"sessions\":[");
    for (int i = 0; i < count; i++) {
        if (i > 0) offset += sprintf(json + offset, ",");
        offset += sprintf(json + offset,
            "{\"session_id\":\"%s\",\"hostname\":\"%s\",\"os\":\"%s\",\"username\":\"%s\"}",
            sessions[i].session_id, sessions[i].host.hostname,
            sessions[i].host.os, sessions[i].host.username);
    }
    offset += sprintf(json + offset, "],\"total\":%d}", count);

    return json;
}

char *json_build_result(const task_result_t *result) {
    if (!result) return NULL;

    char *json = malloc(S0P_MAX_OUTPUT + 256);
    if (!json) return NULL;

    sprintf(json,
        "{\"task_id\":\"%s\",\"status\":\"%s\",\"output\":\"%s\"}",
        result->task_id, result->status, result->output);

    return json;
}
