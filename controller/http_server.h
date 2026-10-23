/*
 * s0P0wn3d — Serveur HTTP simple (POSIX sockets, multi-thread).
 *
 * Sert de transport pour le C2. Parse les requêtes HTTP entrantes,
 * les route vers les handlers, et renvoie les réponses.
 */

#ifndef S0P_HTTP_SERVER_H
#define S0P_HTTP_SERVER_H

#include <stddef.h>

/* Démarre le serveur HTTP sur le port donné.
 * Bloquant — tourne dans une boucle d'accept.
 * Retourne 0 en cas d'arrêt normal. */
int http_server_start(int port);

#endif /* S0P_HTTP_SERVER_H */
