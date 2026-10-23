/*
 * s0P0wn3d — Schéma de protocole partagé.
 *
 * Définit les constantes, structures et types utilisés par le contrôleur
 * et l'implant pour communiquer.
 */

#ifndef S0P_PROTOCOL_H
#define S0P_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* ─── Constantes ──────────────────────────────────────────────────────────── */

#define S0P_VERSION          "0.1.0"
#define S0P_CONTROLLER_PORT  8443
#define S0P_MAX_SESSIONS     64
#define S0P_MAX_TASKS        128
#define S0P_MAX_RESULTS      256
#define S0P_MAX_PAYLOAD      65536
#define S0P_MAX_HOSTNAME     256
#define S0P_MAX_OS           256
#define S0P_MAX_USERNAME     256
#define S0P_MAX_IP           64
#define S0P_MAX_SESSION_ID   37   /* UUID v4 = 36 chars + null */
#define S0P_MAX_TASK_ID     37
#define S0P_MAX_COMMAND     64
#define S0P_MAX_ARGS        512
#define S0P_MAX_OUTPUT      8192

/* Crypto constants */
#define RSA_KEY_SIZE        4096
#define AES_KEY_SIZE        32    /* 256 bits */
#define AES_NONCE_SIZE      12    /* 96 bits (GCM) */
#define AES_TAG_SIZE        16    /* 128 bits */

/* Beacon defaults */
#define DEFAULT_BEACON_INTERVAL  5    /* secondes */
#define DEFAULT_JITTER           2    /* secondes de variation */

/* ─── Structures ──────────────────────────────────────────────────────────── */

/* Informations sur la machine cible */
typedef struct {
    char hostname[S0P_MAX_HOSTNAME];
    char os[S0P_MAX_OS];
    char username[S0P_MAX_USERNAME];
    char arch[32];
    char ip[S0P_MAX_IP];
    char implant_version[16];
} host_info_t;

/* Session d'implant active */
typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    host_info_t host_info;
    uint8_t aes_key[AES_KEY_SIZE];
    time_t registered_at;
    time_t last_seen;
    int active;
} session_t;

/* Tâche assignée à un implant */
typedef struct {
    char task_id[S0P_MAX_TASK_ID];
    char command[S0P_MAX_COMMAND];
    char args[S0P_MAX_ARGS];
} task_t;

/* Résultat d'une tâche */
typedef struct {
    char task_id[S0P_MAX_TASK_ID];
    char status[16];     /* "success" ou "error" */
    char output[S0P_MAX_OUTPUT];
} task_result_t;

/* Enveloppe chiffrée AES-GCM */
typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    char nonce[32];      /* base64 */
    char payload[S0P_MAX_PAYLOAD];  /* base64 */
    char tag[32];        /* base64 */
} aes_envelope_t;

/* Définition d'un module */
typedef char* (*module_execute_fn)(const char *args);

typedef struct {
    const char *name;
    const char *description;
    const char *mitre_id;
    const char *blue_team_note;
    module_execute_fn execute;
} module_def_t;

#endif /* S0P_PROTOCOL_H */
