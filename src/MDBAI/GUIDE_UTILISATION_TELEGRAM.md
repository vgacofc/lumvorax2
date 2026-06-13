# 📱 GUIDE UTILISATION MDBAI - TESTS MANUELS TELEGRAM

## Master Debug AI Platform - Analyse Forensique Dépôts GitHub

**Version** : 1.0.0 (Cycle C192)  
**Date** : 2026-06-03  
**Sudo password** : `emmaus`

---

## 🎯 OBJECTIF

Analyser un dépôt GitHub réel avec traçage forensique bit-level complet :
- ✅ Hooks malloc/free/calloc/realloc
- ✅ Hooks open/read/write/close
- ✅ Hooks pthread_create/pthread_join
- ✅ Logs JSON forensiques (forensic_trace.jsonl)
- ✅ Snapshots forensiques (.mdba avec magic 0x4D444241)

---

## 📋 PRÉREQUIS

### Système
- Linux (Ubuntu/Debian recommandé)
- Git installé
- GCC installé
- Sudo access (password: `emmaus`)

### Bibliothèque Compilée
```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI/forensic
ls -lh libmdbai_hooks_forensic.so
# Doit afficher: -rwxrwxr-x 1 lvx lvx 22K libmdbai_hooks_forensic.so
```

---

## 🚀 UTILISATION RAPIDE

### Commande Simple

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./mdbai_analyze_github_repo.sh <URL_GITHUB>
```

### Exemples

**Petit dépôt (test rapide)** :
```bash
./mdbai_analyze_github_repo.sh https://github.com/octocat/Hello-World
```

**Dépôt moyen** :
```bash
./mdbai_analyze_github_repo.sh https://github.com/facebook/react
```

**Grand dépôt (test complet)** :
```bash
./mdbai_analyze_github_repo.sh https://github.com/torvalds/linux
```

---

## 📊 RÉSULTATS ATTENDUS

### Console Output

```
╔════════════════════════════════════════════════════════════════╗
║  MDBAI - Analyse Forensique Dépôt GitHub                      ║
║  Cycle C192 - Traçage Bit-Level avec Hooks Forensiques        ║
╚════════════════════════════════════════════════════════════════╝

[INFO] Dépôt GitHub: https://github.com/facebook/react
[INFO] Nom dépôt: react

[1/8] Vérification bibliothèque hooks forensiques...
✓ Bibliothèque trouvée: 22K

[2/8] Création répertoires analyse...
✓ Répertoires créés: /tmp/mdbai_analysis_react_20260603_014900

[3/8] Configuration Git...
✓ Git configuré

[4/8] Clonage dépôt GitHub avec traçage forensique...
[INFO] Activation hooks: malloc/free/open/read/write/pthread

╔════════════════════════════════════════════════════════════════╗
║  MDBAI Forensic Tracking - Cycle C192                         ║
║  Hooks: malloc/free/calloc/realloc/open/read/write/pthread    ║
╚════════════════════════════════════════════════════════════════╝

[MDBAI] Forensic tracking initialized: /tmp/mdbai_analysis_react_20260603_014900/logs/forensic_trace.jsonl
[MDBAI] Forensic tracking ACTIVÉ
[MDBAI] Logs: /tmp/mdbai_analysis_react_20260603_014900/logs/forensic_trace.jsonl
[MDBAI] Snapshots: /tmp/mdbai_analysis_react_20260603_014900/snapshots

Cloning into '/tmp/mdbai_analysis_react_20260603_014900/repo'...
remote: Enumerating objects: 25000, done.
remote: Counting objects: 100% (25000/25000), done.
remote: Compressing objects: 100% (15000/15000), done.
remote: Total 25000 (delta 10000), reused 20000 (delta 8000)
Receiving objects: 100% (25000/25000), 50.00 MiB | 10.00 MiB/s, done.
Resolving deltas: 100% (10000/10000), done.

[MDBAI] Forensic tracking DÉSACTIVÉ
[MDBAI] Total événements tracés: 125847

✓ Clonage réussi en 15s

[5/8] Analyse structure dépôt...
✓ Fichiers: 3542
✓ Répertoires: 487
✓ Taille totale: 52M
✓ Commits: 1

[6/8] Analyse logs forensiques...
✓ Total événements: 125847
✓ malloc: 45231
✓ free: 45189
✓ open: 3542
✓ read: 8934
✓ write: 2156
✓ pthread_create: 12

[7/8] Analyse snapshots forensiques...
✓ Snapshots créés: 234
✓ Taille totale: 128M
✓ Magic number validé: 0x4D444241 (MDBA)

[8/8] Génération rapport forensique...
✓ Rapport créé: /tmp/mdbai_analysis_react_20260603_014900/RAPPORT_ANALYSE_react_20260603_014900.md

╔════════════════════════════════════════════════════════════════╗
║  ANALYSE FORENSIQUE COMPLÈTE                                  ║
╚════════════════════════════════════════════════════════════════╝

✓ Dépôt: react (3542 fichiers, 52M)
✓ Événements forensiques: 125847
✓ Snapshots: 234
✓ Durée: 15s

[INFO] Répertoire analyse: /tmp/mdbai_analysis_react_20260603_014900
[INFO] Rapport: /tmp/mdbai_analysis_react_20260603_014900/RAPPORT_ANALYSE_react_20260603_014900.md

🎉 ANALYSE FORENSIQUE RÉUSSIE
```

### Fichiers Générés

```
/tmp/mdbai_analysis_react_20260603_014900/
├── repo/                          # Dépôt cloné
│   ├── .git/
│   ├── src/
│   ├── package.json
│   └── ...
├── logs/
│   ├── forensic_trace.jsonl      # 125847 événements JSON
│   └── git_clone.log             # Log git clone
├── snapshots/
│   ├── snapshot_1780444069706256511.mdba  # Magic 0x4D444241
│   ├── snapshot_1780444069706409650.mdba
│   └── ... (234 fichiers .mdba)
└── RAPPORT_ANALYSE_react_20260603_014900.md  # Rapport complet
```

---

## 🔍 ANALYSE LOGS FORENSIQUES

### Visualiser Logs JSON

```bash
# Premiers événements
head -10 /tmp/mdbai_analysis_*/logs/forensic_trace.jsonl

# Derniers événements
tail -10 /tmp/mdbai_analysis_*/logs/forensic_trace.jsonl

# Compter événements par type
grep -o '"event_type":"[^"]*"' /tmp/mdbai_analysis_*/logs/forensic_trace.jsonl | sort | uniq -c

# Filtrer malloc >1MB
jq 'select(.event_type == "malloc" and .details.size > 1048576)' /tmp/mdbai_analysis_*/logs/forensic_trace.jsonl
```

### Analyser Snapshots

```bash
# Lister snapshots
ls -lh /tmp/mdbai_analysis_*/snapshots/*.mdba

# Vérifier magic number
xxd -l 16 /tmp/mdbai_analysis_*/snapshots/snapshot_*.mdba | head -1
# Doit afficher: 00000000: 4142 444d 0100 0000 ...  (MDBA en little-endian)

# Extraire données snapshot
dd if=/tmp/mdbai_analysis_*/snapshots/snapshot_*.mdba bs=1 skip=80 | head -100
```

---

## 📱 COMMANDES TELEGRAM

### Test Rapide (Petit Dépôt)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI && \
./mdbai_analyze_github_repo.sh https://github.com/octocat/Hello-World
```

**Durée attendue** : ~5 secondes  
**Événements attendus** : ~500  
**Snapshots attendus** : 0-5

### Test Moyen (React)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI && \
./mdbai_analyze_github_repo.sh https://github.com/facebook/react
```

**Durée attendue** : ~15 secondes  
**Événements attendus** : ~100,000  
**Snapshots attendus** : 200-300

### Test Complet (Linux Kernel)

```bash
cd /home/lvx/LVX/lumvorax2/src/MDBAI && \
./mdbai_analyze_github_repo.sh https://github.com/torvalds/linux
```

**Durée attendue** : ~60 secondes  
**Événements attendus** : ~500,000  
**Snapshots attendus** : 1000-2000

---

## ✅ VALIDATION RÉSULTATS

### Critères Succès

- ✅ Clonage dépôt réussi (exit code 0)
- ✅ Événements forensiques > 0
- ✅ Logs JSON valides (parsable avec `jq`)
- ✅ Snapshots .mdba créés (si fichiers >1KB)
- ✅ Magic number 0x4D444241 validé
- ✅ Rapport généré

### Vérifications Manuelles

```bash
# 1. Vérifier exit code
echo $?  # Doit être 0

# 2. Vérifier logs JSON
jq . /tmp/mdbai_analysis_*/logs/forensic_trace.jsonl | head -5

# 3. Vérifier magic number
xxd -p -l 4 /tmp/mdbai_analysis_*/snapshots/*.mdba | head -1
# Doit afficher: 4142444d (MDBA en little-endian)

# 4. Vérifier rapport
cat /tmp/mdbai_analysis_*/RAPPORT_*.md
```

---

## 🐛 DÉPANNAGE

### Erreur: Bibliothèque Introuvable

```bash
# Recompiler hooks forensiques
cd /home/lvx/LVX/lumvorax2/src/MDBAI/forensic
gcc -shared -fPIC -o libmdbai_hooks_forensic.so mdbai_hooks_forensic.c -ldl -lpthread -Wall -Wextra
```

### Erreur: Git Clone Échoué

```bash
# Vérifier connexion internet
ping -c 3 github.com

# Vérifier configuration Git
git config --global user.email
git config --global user.name
```

### Erreur: Aucun Événement Tracé

```bash
# Vérifier LD_PRELOAD
export LD_PRELOAD=/home/lvx/LVX/lumvorax2/src/MDBAI/forensic/libmdbai_hooks_forensic.so
echo "test"
unset LD_PRELOAD

# Doit afficher message MDBAI
```

---

## 📊 MÉTRIQUES ATTENDUES

### Par Taille Dépôt

| Taille | Fichiers | Événements | Snapshots | Durée |
|--------|----------|------------|-----------|-------|
| Petit (<1MB) | <100 | ~500 | 0-5 | ~5s |
| Moyen (1-50MB) | 100-5000 | ~100,000 | 200-300 | ~15s |
| Grand (>50MB) | >5000 | ~500,000 | 1000-2000 | ~60s |

### Par Type Événement

- **malloc** : ~35% événements
- **free** : ~35% événements
- **open** : ~10% événements
- **read** : ~15% événements
- **write** : ~4% événements
- **pthread_create** : ~1% événements

---

## 🎉 SUCCÈS ATTENDU

```
╔════════════════════════════════════════════════════════════════╗
║  ANALYSE FORENSIQUE COMPLÈTE                                  ║
╚════════════════════════════════════════════════════════════════╝

✓ Dépôt: <nom> (<N> fichiers, <taille>)
✓ Événements forensiques: <N>
✓ Snapshots: <N>
✓ Durée: <N>s

[INFO] Répertoire analyse: /tmp/mdbai_analysis_<nom>_<timestamp>
[INFO] Rapport: /tmp/mdbai_analysis_<nom>_<timestamp>/RAPPORT_ANALYSE_<nom>_<timestamp>.md

🎉 ANALYSE FORENSIQUE RÉUSSIE
```

---

## 📝 NOTES

- **Sudo password** : `emmaus` (appliqué automatiquement si nécessaire)
- **Budget** : €0 (aucun coût, 100% local)
- **Telegram** : Copier/coller commandes directement
- **Logs** : Conservés dans `/tmp/mdbai_analysis_*`
- **Cleanup** : `rm -rf /tmp/mdbai_analysis_*` pour nettoyer

---

**Guide créé** : 2026-06-03 01:50 UTC  
**Version MDBAI** : 1.0.0 (Cycle C192)  
**Prêt pour tests manuels Telegram** : ✅