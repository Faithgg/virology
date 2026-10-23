# s0P0wn3d — Architecture

## Vue d'ensemble

s0P0wn3d est un framework Command & Control (C2) modulaire écrit en **C**,
conçu dans un cadre pédagogique (projet Epitech Virology). Il suit le pattern
**implant-to-controller** : un implant (agent) s'exécute sur la machine cible
et communique périodiquement avec un contrôleur central qui lui assigne des
tâches et reçoit les résultats.

```
┌─────────────────────────────────────────────────────────┐
│              CONTRÔLEUR (attaquant, Linux, C)             │
│  ┌──────────┐  ┌───────────┐  ┌──────────┐  ┌─────────┐ │
│  │ HTTP     │  │  Sessions │  │ TaskQueue│  │  Crypto │ │
│  │ Server   │  │  Manager  │  │  (FIFO)  │  │ OpenSSL │ │
│  │ (POSIX)  │  │           │  │          │  │ RSA+AES │ │
│  └────┬─────┘  └─────┬─────┘  └────┬─────┘  └────┬────┘ │
│       └──────────────┴─────────────┴──────────────┘       │
│                         HTTP(S)                            │
└────────────────────────────┬────────────────────────────┘
                             │
                    ┌────────▼────────┐
                    │   Réseau / Lab   │
                    │   (HTTP tunnel)  │
                    └────────┬────────┘
                             │
┌────────────────────────────▼────────────────────────────┐
│              IMPLANT (cible, Windows, C)                   │
│  ┌──────────┐  ┌───────────┐  ┌──────────┐  ┌─────────┐ │
│  │ Beacon   │  │ Transport │  │  Crypto  │  │ Task    │ │
│  │ Loop     │→ │ (WinHTTP) │→ │ OpenSSL  │→ │ Runner  │ │
│  └──────────┘  └───────────┘  └──────────┘  └────┬────┘ │
│                                                  │       │
│              ┌───────────────────────────────────┘       │
│              ▼                                            │
│  ┌──────────────────────────────────────────┐            │
│  │           MODULES (extensibles)           │            │
│  │  shell │ keylog │ creds │ persistence │   │            │
│  │  loot  │ privesc│ pth   │ propagate  │   │            │
│  └──────────────────────────────────────────┘            │
└──────────────────────────────────────────────────────────┘
```

## Choix techniques

| Composant       | Langage | Bibliothèques                  | Plateforme  |
|----------------|---------|-------------------------------|-------------|
| Contrôleur     | C (C11) | POSIX sockets, pthreads, OpenSSL, cJSON | Linux       |
| Implant        | C (C11) | WinHTTP, OpenSSL, Win32 API    | Windows     |
| Transport      | C       | HTTP via WinHTTP / POSIX       | Tunnel HTTP |
| Chiffrement    | C       | OpenSSL EVP (RSA-4096, AES-256-GCM) | Hybride  |

### Pourquoi le C

- Compilation native → binaire autonome, faible empreinte mémoire
- Accès direct aux API système Windows (Win32, syscalls)
- Pas de runtime interprété (contrairement à Python) → plus difficile à détecter
- Contrôle fin de la mémoire et des processus
- Aligné avec les exigences du projet (syscall direct, AV evasion)

### Pourquoi HTTP comme tunnel v1

- Trafic HTTP(S) omniprésent → difficile à distinguer du trafic légitime
- Traversée de proxies et pare-feu (port 443/8080)
- Polling naturel = beaconing périodique
- Facile à démontrer avec Wireshark (capture + déchiffrement prouvé)

### Pourquoi le chiffrement hybride

- **RSA-4096 (OAEP)** : échange de clé de session (asymétrique, prouvable)
- **AES-256-GCM** : chiffrement symétrique des payloads (rapide, authentifié)
- L'implant génère une clé éphémère → chiffrement à sens unique initial
- Démontrable Wireshark : le trafic capturé est illisible sans la clé privée

## Protocole de communication

### Endpoints du contrôleur

| Méthode | Route                  | Description                              |
|---------|------------------------|------------------------------------------|
| GET     | `/pubkey`              | Récupère la clé publique RSA du contrôleur|
| POST    | `/register`            | Enregistre un nouvel implant (session)   |
| GET     | `/tasks/{session_id}`  | Récupère les tâches en attente            |
| POST    | `/results/{session_id}`| Soumet le résultat d'une tâche            |
| GET     | `/sessions`            | Liste des sessions (admin)               |
| POST    | `/sessions/{id}/task`  | Assigner une tâche (admin)               |
| GET     | `/sessions/{id}/results` | Récupérer les résultats (admin)       |

### Cycle de vie de l'implant

```
1. Démarrage de l'implant
2. GET /pubkey → récupère la clé publique RSA du contrôleur
3. Génère une clé AES-256 éphémère
4. Chiffre la clé AES avec la clé publique RSA (échange de clé)
5. POST /register { host_info, encrypted_aes_key } → reçoit session_id
6. Boucle de beaconing :
   a. GET /tasks/{session_id} → reçoit tâches chiffrées (AES-GCM)
   b. Déchiffre, exécute la commande
   c. POST /results/{session_id} → envoie résultat chiffré (AES-GCM)
   d. Sleep (jitter aléatoire pour éviter la détection de pattern)
```

### Format des messages

**Enveloppe AES-GCM (tâches et résultats) :**

```json
{
  "session_id": "uuid-v4",
  "nonce": "base64(Nonce AES-GCM)",
  "payload": "base64(AES-GCM(plaintext, nonce, key))",
  "tag": "base64(Auth tag AES-GCM)"
}
```

**Message déchiffré — Tâche :**

```json
{"type": "task", "task_id": "uuid-v4", "command": "hostname", "args": []}
```

**Message déchiffré — Résultat :**

```json
{"type": "result", "task_id": "uuid-v4", "status": "success", "output": "DESKTOP-LAB01"}
```

## Système de modules

Chaque module est une fonction C implémentant le protocole de dispatch :

```c
typedef char* (*module_execute_fn)(const char *args);

typedef struct {
    const char *name;         // nom de la commande (ex: "hostname")
    const char *description;  // description courte
    const char *mitre_id;     // technique MITRE ATT&CK
    const char *blue_note;    // note détection blue team
    module_execute_fn execute;
} module_def_t;
```

Le `TaskRunner` de l'implant maintient un tableau des modules disponibles
et dispatch les tâches entrantes vers le module approprié.

## Structure du projet

```
s0p0wn3d/
├── controller/           # Serveur C2 (Linux, C)
│   ├── Makefile
│   ├── main.c            # Point d'entrée + serveur HTTP
│   ├── http_server.h/.c   # Serveur HTTP POSIX multi-thread
│   ├── sessions.h/.c      # Gestion des sessions
│   ├── task_queue.h/.c    # File de tâches (FIFO thread-safe)
│   └── crypto.h/.c        # Crypto OpenSSL (RSA + AES-GCM)
├── implant/              # Agent (Windows, C)
│   ├── Makefile          # Cross-compile MinGW
│   ├── main.c            # Point d'entrée
│   ├── beacon.h/.c        # Boucle de beaconing
│   ├── transport.h/.c     # Transport HTTP (WinHTTP)
│   ├── crypto.h/.c        # Crypto OpenSSL
│   ├── task_runner.h/.c   # Exécution des tâches
│   └── modules.h/.c       # Modules intégrés (sysinfo, shell, etc.)
├── shared/               # Code partagé
│   ├── protocol.h        # Constantes, structures, types
│   ├── json.h/.c         # Helpers JSON minimaux
│   └── base64.h/.c       # Encodage/décodage Base64
├── prototype/             # Prototype Python (Phase 1 — référence)
│   └── python/
├── docs/                  # Documentation
│   ├── architecture.md
│   ├── mitre_mapping.md
│   ├── blue_team_notes.md
│   └── lab_manifest.md
├── tests/
├── README.md
└── .gitignore
```

## Plan de développement itératif

| Phase | Objectif                              | Modules                |
|-------|---------------------------------------|------------------------|
| 1     | Plan de contrôle (beacon + crypto)    | ping, hostname, whoami, sysinfo |
| 2     | Shell restreint                       | shell (built-in, pas de spawn) |
| 3     | Persistance (simulé d'abord)          | persistence (scheduled task) |
| 4     | Extraction d'identifiants             | creds (LSASS/SAM — lab only) |
| 5     | Modules étendus                       | keylog, loot, propagate, etc. |
| 6     | Durcissement / furtivité              | AV evasion, syscall, log rollback |

## Lab environment

- **Cible** : Windows 10/11 VM (VirtualBox ou VMware)
- **Contrôleur** : Linux (Kali ou Ubuntu) sur le même réseau virtuel (host-only)
- **Wireshark** : capture sur l'interface du contrôleur pour preuve de chiffrement
- **Snapshot** : VM cible snapshotée avant chaque test
- Voir `docs/lab_manifest.md` pour le manifest reproductible

## Considérations de sécurité et cadre légal

Ce projet est strictement pédagogique. L'outil ne doit être déployé que
sur des machines de laboratoire contrôlées. Toute utilisation contre un
système sans consentement écrit explicite est illégale et strictement
interdite.
