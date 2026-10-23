/*
 * s0P0wn3d — Implémentation du gestionnaire de sessions.
 */

#include "sessions.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/rand.h>

static session_t sessions[S0P_MAX_SESSIONS];
static int session_count = 0;
static pthread_mutex_t sessions_lock = PTHREAD_MUTEX_INITIALIZER;

static void generate_uuid(char *out) {
    uint8_t bytes[16];
    RAND_bytes(bytes, sizeof(bytes));
    /* Version 4 + variant */
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;
    snprintf(out, S0P_MAX_SESSION_ID,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5],
        bytes[6], bytes[7],
        bytes[8], bytes[9],
        bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]
    );
}

int sessions_init(void) {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
    return 0;
}

session_t *sessions_register(const host_info_t *host_info,
                              const uint8_t *aes_key,
                              char *session_id_out) {
    pthread_mutex_lock(&sessions_lock);

    if (session_count >= S0P_MAX_SESSIONS) {
        pthread_mutex_unlock(&sessions_lock);
        return NULL;
    }

    session_t *s = &sessions[session_count];
    memset(s, 0, sizeof(session_t));

    generate_uuid(s->session_id);
    memcpy(&s->host_info, host_info, sizeof(host_info_t));
    memcpy(s->aes_key, aes_key, AES_KEY_SIZE);
    s->registered_at = time(NULL);
    s->last_seen = s->registered_at;
    s->active = 1;

    if (session_id_out)
        strncpy(session_id_out, s->session_id, S0P_MAX_SESSION_ID);

    session_count++;
    pthread_mutex_unlock(&sessions_lock);

    printf("[+] Nouvel implant : %s (%s@%s)\n",
           s->session_id, s->host_info.username, s->host_info.hostname);

    return s;
}

session_t *sessions_get(const char *session_id) {
    pthread_mutex_lock(&sessions_lock);
    session_t *result = NULL;
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active &&
            strcmp(sessions[i].session_id, session_id) == 0) {
            result = &sessions[i];
            break;
        }
    }
    pthread_mutex_unlock(&sessions_lock);
    return result;
}

void sessions_touch(const char *session_id) {
    pthread_mutex_lock(&sessions_lock);
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active &&
            strcmp(sessions[i].session_id, session_id) == 0) {
            sessions[i].last_seen = time(NULL);
            break;
        }
    }
    pthread_mutex_unlock(&sessions_lock);
}

int sessions_list(session_t *out, int max) {
    pthread_mutex_lock(&sessions_lock);
    int count = 0;
    for (int i = 0; i < session_count && count < max; i++) {
        if (sessions[i].active) {
            memcpy(&out[count], &sessions[i], sizeof(session_t));
            count++;
        }
    }
    pthread_mutex_unlock(&sessions_lock);
    return count;
}

void sessions_cleanup(void) {
    /* Rien à faire — les sessions sont en mémoire statique */
}
