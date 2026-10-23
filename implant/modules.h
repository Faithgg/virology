/*
 * s0P0wn3d — Modules intégrés de l'implant (Windows).
 *
 * Chaque module implémente une commande exécutable à distance.
 * Phase 1 : commandes de diagnostic sûres (lecture seule).
 */

#ifndef S0P_MODULES_H
#define S0P_MODULES_H

#include "../shared/protocol.h"

/* Retourne le tableau des modules disponibles. */
const module_def_t *modules_get_all(int *count);

/* Recherche un module par nom. Retourne NULL si introuvable. */
const module_def_t *modules_find(const char *name);

#endif /* S0P_MODULES_H */
