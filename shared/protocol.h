#ifndef S0P_PROTOCOL_H
#define S0P_PROTOCOL_H

#include <stdint.h>
#include <time.h>

#define S0P_VERSION "0.1.0"
#define S0P_CONTROLLER_PORT 8443

#define S0P_MAX_SESSIONS     32
#define S0P_MAX_SESSION_ID   64
#define S0P_MAX_HOSTNAME     256
#define S0P_MAX_OS           64
#define S0P_MAX_USERNAME     64
#define S0P_MAX_IP           16
#define S0P_MAX_ARCH         32
#define S0P_MAX_COMMAND      256
#define S0P_MAX_ARGS         512
#define S0P_MAX_TASK_ID      64
#define S0P_MAX_OUTPUT       (1024 * 64)
#define S0P_MAX_PAYLOAD      (1024 * 100)

#define AES_KEY_SIZE         32
#define RSA_KEY_SIZE         2048

typedef struct {
    char hostname[S0P_MAX_HOSTNAME];
    char os[S0P_MAX_OS];
    char username[S0P_MAX_USERNAME];
    char arch[S0P_MAX_ARCH];
    char ip[S0P_MAX_IP];
    char implant_version[32];
} host_info_t;

typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    host_info_t host;
    uint8_t aes_key[AES_KEY_SIZE];
    time_t created_at;
    time_t last_seen;
} session_t;

typedef struct {
    char task_id[S0P_MAX_TASK_ID];
    char command[S0P_MAX_COMMAND];
    char args[S0P_MAX_ARGS];
} task_t;

typedef struct {
    char task_id[S0P_MAX_TASK_ID];
    char status[32];
    char output[S0P_MAX_OUTPUT];
} task_result_t;

#endif
