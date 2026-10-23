#include "sessions.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static session_t sessions[S0P_MAX_SESSIONS];
static int session_count = 0;

void sessions_init(void) {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
}

void sessions_cleanup(void) {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
}

session_t *sessions_register(const host_info_t *host, const uint8_t *aes_key, char *session_id) {
    if (!host || !aes_key || !session_id) return NULL;
    if (session_count >= S0P_MAX_SESSIONS) return NULL;

    session_t *sess = &sessions[session_count];
    memset(sess, 0, sizeof(session_t));

    /* Generate session ID */
    snprintf(sess->session_id, S0P_MAX_SESSION_ID, "sess_%d_%ld",
             session_count, time(NULL));
    strcpy(session_id, sess->session_id);

    sess->host = *host;
    memcpy(sess->aes_key, aes_key, AES_KEY_SIZE);
    sess->created_at = time(NULL);
    sess->last_seen = time(NULL);

    session_count++;
    return sess;
}

session_t *sessions_get(const char *session_id) {
    if (!session_id) return NULL;
    for (int i = 0; i < session_count; i++) {
        if (strcmp(sessions[i].session_id, session_id) == 0) {
            return &sessions[i];
        }
    }
    return NULL;
}

void sessions_touch(const char *session_id) {
    session_t *sess = sessions_get(session_id);
    if (sess) {
        sess->last_seen = time(NULL);
    }
}

int sessions_list(session_t *buf, int max_count) {
    if (!buf || max_count <= 0) return 0;
    int count = (session_count < max_count) ? session_count : max_count;
    memcpy(buf, sessions, count * sizeof(session_t));
    return count;
}
