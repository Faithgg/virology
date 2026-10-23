/*
 * s0P0wn3d — Boucle de beaconing de l'implant.
 *
 * Enregistre l'implant auprès du contrôleur, puis boucle :
 *   1. Poll les tâches
 *   2. Déchiffre et exécute chaque tâche
 *   3. Chiffre et soumet les résultats
 *   4. Sleep avec jitter aléatoire
 */

#ifndef S0P_BEACON_H
#define S0P_BEACON_H

#include "../shared/protocol.h"
#include <stdint.h>

/* Démarre la boucle de beaconing. Bloquant. */
int beacon_loop(const char *controller_host, int controller_port,
                int interval, int jitter);

#endif /* S0P_BEACON_H */
