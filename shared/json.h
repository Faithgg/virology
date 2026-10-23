#ifndef S0P_JSON_H
#define S0P_JSON_H

#include "protocol.h"

char *json_extract_string(const char *json, const char *key);
char *json_extract_nested(const char *json, const char *outer, const char *inner);
int json_parse_result(const char *json, task_result_t *result);
int json_parse_tasks(const char *json, task_t *tasks, int max_count);
char *json_build_ack(void);
char *json_build_sessions(const session_t *sessions, int count);
char *json_build_result(const task_result_t *result);

#endif
