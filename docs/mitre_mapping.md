# s0P0wn3d — Mapping MITRE ATT&CK

Chaque capacité implémentée est mappée à sa technique MITRE ATT&CK
avec une note de détection blue team.

## Phase 1 — Plan de contrôle

### Beaconing HTTP

| Champ          | Valeur                                    |
|----------------|-------------------------------------------|
| Tactique       | Command and Control (TA0011)               |
| Technique      | T1071.001 — Web Protocols                 |
| Description    | L'implant communique avec le contrôleur via HTTP |
| Blue Team Note | Détectable par analyse de fréquence de requêtes vers un même endpoint. Un SIEM peut corréler les périodes de polling régulières. Le beaconing avec jitter aléatoire complique la détection par analyse temporelle. |

### Chiffrement des communications

| Champ          | Valeur                                    |
|----------------|-------------------------------------------|
| Tactique       | Command and Control (TA0011)               |
| Technique      | T1573.002 — Encrypted Channel: Asymmetric Cryptography |
| Description    | Chiffrement hybride RSA-4096 + AES-256-GCM |
| Blue Team Note | Le trafic chiffré est illisible sans la clé privée. La détection nécessite l'analyse du pattern de handshake (échange de clé RSA initial) ou la corrélation avec des indicateurs de compromission (IOC) réseau. |

### Énumération système

| Champ          | Valeur                                    |
|----------------|-------------------------------------------|
| Tactique       | Discovery (TA0007)                         |
| Technique      | T1082 — System Information Discovery      |
| Description    | Collecte d'informations système (hostname, OS, utilisateur) |
| Blue Team Note | Détectable par EDR surveillant les appels API GetComputerName, GetUserName, GetVersionEx, GetSystemInfo. Ces appels sont légitimes mais inhabituels en provenance d'un processus inconnu. |

### Énumération de fichiers

| Champ          | Valeur                                    |
|----------------|-------------------------------------------|
| Tactique       | Discovery (TA0007)                         |
| Technique      | T1083 — File and Directory Discovery      |
| Description    | Liste le contenu d'un répertoire           |
| Blue Team Note | Détectable par monitoring des appels FindFirstFile/FindNextFile. Un EDR peut alerter sur une énumération massive ou inhabituelle du système de fichiers. |

### Shell restreint

| Champ          | Valeur                                    |
|----------------|-------------------------------------------|
| Tactique       | Execution (TA0002)                         |
| Technique      | T1059 — Command and Scripting Interpreter |
| Description    | Shell avec commandes intégrées (pas de spawn de cmd.exe) |
| Blue Team Note | Plus difficile à détecter qu'un shell classique car aucun processus n'est créé. La détection repose sur le monitoring des API appelées par le shell intégré (GetComputerName, FindFirstFile, etc.) et la corrélation avec le pattern de beaconing C2. |

## Roadmap MITRE (phases futures)

| Phase | Module        | Technique MITRE | Tactique         |
|-------|---------------|-----------------|------------------|
| 3     | Persistence   | T1547            | Persistence      |
| 4     | Creds         | T1003            | Credential Access|
| 5     | Keylog        | T1056.001        | Collection       |
| 5     | Loot          | T1005            | Collection       |
| 5     | Propagate     | T1021            | Lateral Movement |
| 5     | Privesc       | T1068            | Privilege Escalation |
| 6     | Log rollback  | T1070            | Defense Evasion  |
| 6     | AV evasion    | T1027            | Defense Evasion  |
| 6     | Direct syscall| T1106            | Execution        |
