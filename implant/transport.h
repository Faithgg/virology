#ifndef S0P_TRANSPORT_H
#define S0P_TRANSPORT_H

#include "../shared/protocol.h"

int transport_init(const char *host, int port);
void transport_cleanup(void);
char *transport_get_pubkey(void);
char *transport_register(const char *encrypted_key, const host_info_t *host);
char *transport_fetch_tasks(const char *session_id);
int transport_submit_result(const char *session_id, const char *nonce,
                            const char *payload, const char *tag);

#endif
