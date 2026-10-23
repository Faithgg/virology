# s0P0wn3d | Virology

Framework Command & Control (C2) modulaire, écrit en C, conçu dans le cadre
pédagogique du projet **Virology** (Epitech).

## Equipe & rôles

| Membre   | Rôle                     | Responsabilités                                     |
| -------- | ------------------------ | --------------------------------------------------- |
| Bruno Junior | Architecte & Intégrateur | Architecture, Gestion de l'infra, serveur HTTP contrôleur, intégrateur de modules  |
| Serge | Développeur & Controlleur      | Implant Windows, modules, reverse shell (Beacon), lab VM |
| Faithgot | Sécurité & Integrateur & Documentation | Crypto, intégrateur de modules, tests, Docs         |

## Outils 

[Trello](https://trello.com/invite/b/6ab140ddfc6bf43978af9bb8/ATTI97aaef3d727e99b56f05ed166ff04ee07ECD7336/%F0%9F%9B%A1%EF%B8%8F-projet-encadre-equipe-labo-et-livrables)


## Architecture

s0P0wn3d suit le pattern **implant-to-controller** :

- **Contrôleur** (Linux, C) : serveur HTTP multi-thread en C avec OpenSSL
- **Implant** (Windows, C) : agent utilisant WinHTTP + OpenSSL
- **Transport** : tunnel HTTP (se fond dans le trafic normal)
- **Chiffrement** : hybride RSA-4096 (échange de clé) + AES-256-GCM (payloads)

[https://draw.io](https://viewer.diagrams.net/?tags=%7B%7D&lightbox=1&highlight=0000ff&edit=_blank&layers=1&nav=1&title=Archi%20Virology.drawio&dark=auto#R%3Cmxfile%3E%3Cdiagram%20name%3D%22Page-1%22%20id%3D%22XytZE8Nh3yGcPe5Y9C8t%22%3E7Vrdb6M4EP9b7iHS7QMI8xXy2KSb7uq0q%2BqyH4%2BVAYdYBcwZJ2n3r78xmAQD3abdsGq1TaXGzNhje34z45mYibPI7q44LjafWEzSiW3FdxPncmLbyHIC%2BJKUe0WZBXZNSTiNFe1IWNEfpBmqqFsak1LrKBhLBS10YsTynERCo2HO2V7vtmapPmuBE9IjrCKc9qnfaSw2NTWwp0f6B0KTTTMz8mc1J8NNZ7WTcoNjtm%2BRnPcTZ8EZE3Uru1uQVGqv0Us9bvkA97AwTnJxyoAfy7lDbXe6T6zlx2B29c%2F66ydjQIoileK%2B0QHIAXXDw3y%2FoYKsChxJzh4gB9pGZCk8IWiuaZouWMp4Nc5Ze%2FIP6KXg7Ja0OH71kSNYLlr0%2BgP0eg07nG7VGhSBcEHuWutUW70iLCOC30OXTQsN11O63x%2Bhm%2FqKpsS46lEZaMPFym6Sg%2BSjaqGhtPsETbtn1fSAfvwUxM5hsVi6wwbzktTz%2BP9tpY3Nt2JtBMdHaCXquxpZFjjXVtN0jGp8LoDJk%2FBv5E1sUIElza369t9VUiyJpbHGGU3v694gAmdy4UrQRYZ%2FMDnH%2BywkvMutpXWpH0i6I4JGclOfCex2cNS%2FLGSC1e0LTnFaN0ucl0ZJOF23VlhWMUauD7nFXZtR71xycsYzEHLk7TBIhe%2BUJlhsuYxIP%2B0X4eKhLntloJLpWlbNSYkQhBuAQUTzpD%2BS8WIDm6kZdk0DTxAGhhXlNbkUmIsWi%2BZxZWqSZzU7rTiCg6w1iG8myknNBT%2BpImZrlj3jsb6ugyzYSnhLQZyUWfu4oTxN61fZciXjoF5l0dAOcXSbcLbNY6NjaJVrVhbmoabhvWttIyYR41hQlhtiQ6PbnJRq7TSngja66%2FZt4fzTfq3laP1iWhYpvm84KQXt2dZfNCsYFzhXEKxThkVHv13XQ47pIdN2AhP5QeOHEF1qV2zc8xmBDw0EPrsT%2BBDSIx8aLfR5p4Q%2BqX09xg2FQP0kUYptHzuKpNziMoIZIdQ4c6lAiCLphWJkNI7l1PPK9IhcvjUUV79dVzHl%2B%2BpZODh9GNBUh8FxPA0GdzoWDP4pMNCsSojmBaO5qFbgzSfepVRpWdQJ1preSX21oeqp%2B5DsSKXGuNwcNKwmuKRZArtIaQj%2FMRx7JL4RJCUlLYEQsazYgqQbnEsyz2iOU8m4Jrxk0L5ZqB5muUvOli%2F4yAys9sftwed6pq%2B7USeBQEFg2iMhOD0FwV%2FJIZ4TaqauiR4NNu6soyXPM2cjaSl4s%2FMR7NxzkIYgnO8j4Td7xcfFivAd2UJua30Nt7nYnuvUmHXO7pl%2BaNjT8dypSRNeOR7fIR2Wue25jnGkI%2BL5HURmY7kHOqlmf6F4vBWbj9dtEhsjY%2FG5iqVe1eGbaIpM5NhmlfOfr%2BywB%2FzE6vhJJxGwvdH8xH7zkz%2FUT3oWP7VNBwrtwJxBUrNsbSgPSzXkdzrBIXcaPZdCJ%2F3s2CTDvdS3AJhgIdKUL1VdcX0kPTMzVj8mfWGg%2BUvnoVQ5EnRHbmLKYUVMamJJpeSciJskZSE5a0KMkDdwwnsaaI7na6AZ4%2F1WjE4q9F546JJ%2BPRi6HnL4rtd%2BVHi3nLOWeV7n9DqVqq3XOeBC3lgwn1SpvnCY306oM55Qjmd6s5dwQvXSNHs0J3jN5f6bE4zgBJYJO1ki%2FzebvDPVA787WmUydHf%2FoMkP2XmKQ5LOD7Vg6wJ9sVjCZ8hUabI3rDCKcRSGnh0gK3DD%2BFm6POVSw9PDh%2BGMpsuTElzdjZ%2FwFsPh6q6jzWNuYl1hQfb4vrHWmO66N%2Bsh71Jgq1q%2FZ6AwdMPXtehuKjOeRavTgcS9t2l%2BelWRxxfyHZ2jolswPBp6T9UQJymWxYS2ioEEcEgraoZreUvQyhE7V6c9xZZsyyOiRh11%2B7igw21SIwmCaUJET1KF0mHfvwCc%2F0cD163gnozb74LpNReE3z4vznVmu3pJ3rsGGS%2FAveYsdbX6cC4APB0Ab2abvZuoJ0MAj8fXIGuXOb5N6rz%2FHw%3D%3D%3C%2Fdiagram%3E%3C%2Fmxfile%3E)

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
s0p0wn3d_implant.exe
```

### 3. Interagir avec les implants

curl / reverse shell (ici le beacon)

# Lister les sessions actives
curl http://<controller_ip>:8443/sessions

# Assigner une tâche
curl -X POST http://<controller_ip>:8443/sessions/<session_id>/task \
  -H "Content-Type: application/json" \
  -d '{"command":"sysinfo","args":""}'

# Récupérer les résultats
curl http://<controller_ip>:8443/sessions/<session_id>/results
```

Nous utilisons ici ngrok pour avoir un lien du controlleur accessible sur internet donc dans le même réseau que l'implant.


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

## Mapping MITRE ATT&CK

Voir [docs/mitre_mapping.md](docs/mitre_mapping.md).

## Structure du projet


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
├── docs/                # Documentation
└── README.md


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
