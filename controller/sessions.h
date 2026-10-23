/*
 * s0P0wn3d — Gestionnaire de sessions (côté contrôleur).
 *
 * Suit les implants enregistrés de manière thread-safe.
 */

#ifndef S0P_SESSIONS_H
#define S0P_SESSIONS_H

#include <pthread.h>
#include <time.h>
#include "../shared/protocol.h"

/* Initialise le gestionnaire de sessions. */
int sessions_init(void);

/* Enregistre un nouvel implant. Retourne un pointeur vers la session
 * ou NULL en cas d'erreur. Le session_id est généré et copié dans
 * session_id_out. */
session_t *sessions_register(const host_info_t *host_info,
                              const uint8_t *aes_key,
                              char *session_id_out);

/* Récupère une session par son ID. Retourne NULL si introuvable. */
session_t *sessions_get(const char *session_id);

/* Met à jour le timestamp de dernière activité. */
void sessions_touch(const char *session_id);

/* Liste toutes les sessions actives. Retourne le nombre de sessions. */
int sessions_list(session_t *out, int max);

/* Détruit le gestionnaire. */
void sessions_cleanup(void);

#endif /* S0P_SESSIONS_H */
