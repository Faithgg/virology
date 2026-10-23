#ifndef S0P_SESSIONS_H
#define S0P_SESSIONS_H

#include "../shared/protocol.h"

void sessions_init(void);
void sessions_cleanup(void);
session_t *sessions_register(const host_info_t *host, const uint8_t *aes_key, char *session_id);
session_t *sessions_get(const char *session_id);
void sessions_touch(const char *session_id);
int sessions_list(session_t *buf, int max_count);

#endif
