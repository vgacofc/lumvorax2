
# Système de Redirection Console LUM/VORAX

## Vue d'ensemble

Ce système permet de sauvegarder automatiquement toutes les sorties console (stdout et stderr) dans des fichiers de log horodatés, tout en conservant l'affichage normal dans la console.

## Structure des Logs

```
logs/console/
├── current/              # Logs de la session courante
│   ├── console_[session].log    # Sortie standard (stdout)
│   ├── errors_[session].log     # Erreurs (stderr)
│   └── combined_[session].log   # Log combiné
├── archive/              # Logs des sessions précédentes
│   └── [timestamp]/      # Dossier par session archivée
├── sessions/             # Métadonnées des sessions
└── current_session.txt   # ID de la session courante
```

## Scripts Disponibles

### 1. `setup_console_redirect.sh`
Configure le système de redirection console.
```bash
./setup_console_redirect.sh
```

### 2. `logs/console/redirect_console.sh`
Active la redirection automatique (utilisé dans les workflows).
```bash
source logs/console/redirect_console.sh
```

### 3. `logs/console/archive_logs.sh`
Archive les logs de la session courante.
```bash
./logs/console/archive_logs.sh
```

### 4. `logs/console/monitor_console.sh`
Affiche les derniers logs en temps réel.
```bash
./logs/console/monitor_console.sh
```

## Utilisation

### Automatique (Recommandé)
Le système s'active automatiquement lors de l'exécution des workflows LUM/VORAX.

### Manuel
```bash
# Configuration initiale
./setup_console_redirect.sh

# Activation de la redirection
source logs/console/redirect_console.sh

# Exécution de vos commandes (logs automatiquement sauvegardés)
make test-progressive

# Monitoring des logs
./logs/console/monitor_console.sh
```

## Avantages

- ✅ **Historique complet** : Tous les logs sont conservés
- ✅ **Horodatage** : Chaque session a un timestamp unique
- ✅ **Séparation** : stdout et stderr séparés + log combiné
- ✅ **Archivage** : Système d'archivage automatique
- ✅ **Monitoring** : Surveillance en temps réel
- ✅ **Intégration** : Compatible avec les workflows existants

## Format des Sessions

```
console_YYYYMMDD_HHMMSS_PID
Exemple: console_20250921_143022_12345
```

## Commandes Utiles

```bash
# Voir la session courante
cat logs/console/current_session.txt

# Lister tous les logs
find logs/console -name "*.log" | sort

# Taille totale des logs
du -sh logs/console/

# Nettoyer les logs anciens (>7 jours)
find logs/console/archive -type f -mtime +7 -delete
```
