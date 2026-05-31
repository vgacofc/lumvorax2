# GUIDE PROTOCOLES PRÉVENTION MDBAI
## Utilisation Scripts Git et Backup

**Version**: 1.0.0  
**Date création**: 2026-05-31T02:23Z  
**Objectif**: Guide complet d'utilisation des protocoles de prévention  
**Contexte**: Suite incident perte 157 tests (C172-C175)  

---

## 📋 TABLE DES MATIÈRES

1. [Vue d'Ensemble](#vue-densemble)
2. [Protocole Git Strict](#protocole-git-strict)
3. [Protocole Backup Automatique](#protocole-backup-automatique)
4. [Workflow Complet Cycle](#workflow-complet-cycle)
5. [Exemples Pratiques](#exemples-pratiques)
6. [Dépannage](#dépannage)
7. [Checklist Pré/Post-Cycle](#checklist-prépost-cycle)

---

## 1. VUE D'ENSEMBLE

### 1.1 Objectif des Protocoles

**Prévenir la perte de travail** suite à:
- Reboot système (répertoires `/tmp/` volatiles)
- Crash application
- Erreurs manipulation fichiers
- Oubli sauvegarde manuelle

### 1.2 ROI Prévention

| Action | Temps | Fréquence | Coût Total |
|--------|-------|-----------|------------|
| Git commit | 2 min | Par cycle | 2 min |
| Backup | 1 min | Par cycle | 1 min |
| **Total prévention** | **3 min** | **Par cycle** | **3 min** |
| **Récupération (si perte)** | **16h** | **Par incident** | **16h** |
| **ROI** | **-** | **-** | **320×** |

**Conclusion**: Prévention = **320× plus efficace** que récupération

### 1.3 Scripts Disponibles

| Script | Fonction | Temps | Obligatoire |
|--------|----------|-------|-------------|
| `git_commit_cycle.sh` | Commit Git automatique | 2 min | ✅ OUI |
| `backup_cycle.sh` | Backup .tar.gz | 1 min | ✅ OUI |

---

## 2. PROTOCOLE GIT STRICT

### 2.1 Script: git_commit_cycle.sh

**Emplacement**: `/home/lvx/LVX/lumvorax2/src/MDBAI/git_commit_cycle.sh`  
**Permissions**: `-rwxrwxr-x` (exécutable)  
**Taille**: 4.3 KB  

### 2.2 Usage

```bash
./git_commit_cycle.sh <cycle_number> <module_name> <coverage_gain>
```

**Paramètres**:
- `cycle_number`: Numéro du cycle (ex: 172)
- `module_name`: Nom du module sans extension (ex: rpc_interface)
- `coverage_gain`: Gain de couverture en % (ex: 49.95)

### 2.3 Fonctionnalités

1. **Vérification modifications**: Détecte changements dans module
2. **Aperçu diff**: Affiche statistiques modifications
3. **Confirmation interactive**: Demande validation avant commit
4. **Message formaté**: Crée message commit standardisé
5. **Push optionnel**: Propose push vers remote
6. **Suggestion backup**: Rappelle de créer backup

### 2.4 Message Commit Généré

```
C<NUM>: Add tests for <module> (+X.XX% coverage)

- Module: <module>.rs
- Tests added: Multiple unit tests
- Coverage gain: +X.XX%
- Lines added: +NNN
- Lines removed: -NNN

Cycle: C<NUM>
Date: <ISO 8601>
```

### 2.5 Exemple Utilisation

```bash
# Après avoir créé et validé tests C172
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./git_commit_cycle.sh 172 rpc_interface 49.95

# Output attendu:
# 🔄 Démarrage commit Git Cycle C172
# ✓ Modifications détectées dans src/vmm/src/rpc_interface.rs
# 
# 📋 Aperçu des modifications:
#    +514 lignes ajoutées
#    -0 lignes supprimées
# 
# ❓ Confirmer le commit ? (Y/n)
# [Appuyer sur Entrée]
# 
# ✅ Commit créé avec succès !
#    📝 Hash: a1b2c3d
#    📄 Fichier: src/vmm/src/rpc_interface.rs
#    📊 Couverture: +49.95%
# 
# ❓ Pousser vers remote ? (Y/n)
# [Appuyer sur Entrée]
# 
# ✅ Push réussi !
# 
# 💡 N'oubliez pas de créer un backup:
#    /home/lvx/LVX/lumvorax2/src/MDBAI/backup_cycle.sh 172 rpc_interface
```

---

## 3. PROTOCOLE BACKUP AUTOMATIQUE

### 3.1 Script: backup_cycle.sh

**Emplacement**: `/home/lvx/LVX/lumvorax2/src/MDBAI/backup_cycle.sh`  
**Permissions**: `-rwxrwxr-x` (exécutable)  
**Taille**: 4.2 KB  

### 3.2 Usage

```bash
./backup_cycle.sh <cycle_number> <module_name>
```

**Paramètres**:
- `cycle_number`: Numéro du cycle (ex: 172)
- `module_name`: Nom du module sans extension (ex: rpc_interface)

### 3.3 Fichiers Sauvegardés

Le script recherche et archive automatiquement:

1. **Fichier source modifié**: `firecracker/src/vmm/src/<module>.rs`
2. **Backup original**: `firecracker/src/vmm/src/<module>.rs.backup`
3. **Scripts Python**: `generate_tests_c<NUM>_*.py`
4. **Fichiers Rust générés**: `c<NUM>_tests_*.rs`
5. **Rapports markdown**: `RAPPORT*C<NUM>*.md`
6. **Mappings**: `MAPPING*C<NUM>*.md`

### 3.4 Fonctionnalités

1. **Détection automatique**: Trouve tous les fichiers liés au cycle
2. **Archive .tar.gz**: Compression efficace
3. **Log centralisé**: Enregistre dans `backups/BACKUPS.log`
4. **Statistiques**: Affiche taille et nombre de fichiers
5. **Contenu visible**: Liste fichiers archivés

### 3.5 Exemple Utilisation

```bash
# Après commit Git
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./backup_cycle.sh 172 rpc_interface

# Output attendu:
# 🔄 Démarrage backup Cycle C172 - Module rpc_interface
# ✓ Trouvé: rpc_interface.rs
# ✓ Trouvé: rpc_interface.rs.backup
# ✓ Trouvé: generate_tests_c172_rpc.py
# ✓ Trouvé: c172_tests_generated.rs
# ✓ Trouvé: RAPPORT_C172_TESTS_RPC_INTERFACE_FINAL_2026-05-30T1938Z.md
# ✓ Trouvé: RAPPORT_FINAL_C172_SYNTHESE_2026-05-30T1951Z.md
# 
# 📦 Création archive...
# ✅ Backup créé avec succès !
#    📁 Fichier: backup_C172_rpc_interface_20260531_022300.tar.gz
#    📊 Taille: 487K
#    📄 Fichiers: 6
# 
# 📋 Contenu de l'archive:
#    firecracker/src/vmm/src/rpc_interface.rs
#    firecracker/src/vmm/src/rpc_interface.rs.backup
#    generate_tests_c172_rpc.py
#    c172_tests_generated.rs
#    RAPPORT_C172_TESTS_RPC_INTERFACE_FINAL_2026-05-30T1938Z.md
#    RAPPORT_FINAL_C172_SYNTHESE_2026-05-30T1951Z.md
# 
# ✅ Backup terminé - Fichier sauvegardé dans:
#    /home/lvx/LVX/lumvorax2/src/MDBAI/backups/backup_C172_rpc_interface_20260531_022300.tar.gz
# 
# 📊 Statistiques backups:
#    Total backups: 1
#    Espace utilisé: 487K
```

---

## 4. WORKFLOW COMPLET CYCLE

### 4.1 Workflow Standard

```bash
# 1. Créer tests (selon méthodologie habituelle)
cd /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker
# ... création tests ...

# 2. Compiler et valider
cargo test --package vmm --lib <module>::tests

# 3. Mesurer couverture (optionnel)
cargo llvm-cov --package vmm --lib --html

# 4. Commit Git (OBLIGATOIRE)
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./git_commit_cycle.sh <cycle> <module> <coverage>

# 5. Backup (OBLIGATOIRE)
./backup_cycle.sh <cycle> <module>

# 6. Créer rapport final
# ... documentation ...
```

### 4.2 Temps Total

| Étape | Temps |
|-------|-------|
| Création tests | Variable (1-4h) |
| Compilation | 1-2 min |
| Validation | 0-5 min |
| **Git commit** | **2 min** |
| **Backup** | **1 min** |
| Rapport | 15-30 min |
| **Total prévention** | **3 min** |

**ROI**: 3 minutes investies évitent 16h de récupération

---

## 5. EXEMPLES PRATIQUES

### 5.1 Exemple Complet: Cycle C172

```bash
# Contexte: 60 tests RPC créés, +49.95% couverture

# 1. Vérifier tests passent
cd /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker
cargo test --package vmm --lib rpc_interface::tests
# ✅ 60/60 tests passent

# 2. Commit Git
cd /home/lvx/LVX/lumvorax2/src/MDBAI
./git_commit_cycle.sh 172 rpc_interface 49.95
# Confirmer: Y
# Push: Y
# ✅ Commit a1b2c3d créé et poussé

# 3. Backup
./backup_cycle.sh 172 rpc_interface
# ✅ backup_C172_rpc_interface_20260531_022300.tar.gz créé (487K)

# 4. Vérifier backup
ls -lh backups/backup_C172_*.tar.gz
# -rw-rw-r-- 1 lvx lvx 487K May 31 02:23 backup_C172_rpc_interface_20260531_022300.tar.gz

# 5. Vérifier commit Git
cd firecracker
git log -1 --oneline
# a1b2c3d C172: Add tests for rpc_interface (+49.95% coverage)

# ✅ Cycle C172 sécurisé !
```

### 5.2 Exemple Récupération depuis Backup

```bash
# Scénario: Fichier rpc_interface.rs corrompu

# 1. Lister backups disponibles
ls -lh /home/lvx/LVX/lumvorax2/src/MDBAI/backups/backup_C172_*.tar.gz

# 2. Extraire backup
cd /home/lvx/LVX/lumvorax2/src/MDBAI
tar -xzf backups/backup_C172_rpc_interface_20260531_022300.tar.gz

# 3. Restaurer fichier
cp firecracker/src/vmm/src/rpc_interface.rs \
   firecracker/src/vmm/src/rpc_interface.rs.restored

# 4. Vérifier intégrité
cd firecracker
cargo test --package vmm --lib rpc_interface::tests

# ✅ Fichier restauré !
```

### 5.3 Exemple Récupération depuis Git

```bash
# Scénario: Modifications accidentelles

# 1. Vérifier historique
cd /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker
git log --oneline src/vmm/src/rpc_interface.rs

# 2. Restaurer version spécifique
git checkout a1b2c3d -- src/vmm/src/rpc_interface.rs

# 3. Vérifier restauration
cargo test --package vmm --lib rpc_interface::tests

# ✅ Version restaurée !
```

---

## 6. DÉPANNAGE

### 6.1 Problème: Script non exécutable

**Symptôme**:
```bash
./git_commit_cycle.sh 172 rpc_interface 49.95
# bash: ./git_commit_cycle.sh: Permission denied
```

**Solution**:
```bash
chmod +x /home/lvx/LVX/lumvorax2/src/MDBAI/git_commit_cycle.sh
chmod +x /home/lvx/LVX/lumvorax2/src/MDBAI/backup_cycle.sh
```

### 6.2 Problème: Fichier module non trouvé

**Symptôme**:
```bash
./git_commit_cycle.sh 172 rpc_interface 49.95
# ❌ Erreur: Fichier src/vmm/src/rpc_interface.rs non trouvé
```

**Solution**:
```bash
# Vérifier chemin Firecracker
ls -la /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker/src/vmm/src/

# Vérifier nom module exact
ls -la /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker/src/vmm/src/ | grep rpc
```

### 6.3 Problème: Aucune modification détectée

**Symptôme**:
```bash
./git_commit_cycle.sh 172 rpc_interface 49.95
# ⚠ Aucune modification détectée dans src/vmm/src/rpc_interface.rs
```

**Cause**: Fichier déjà commité ou pas de changements

**Solution**:
```bash
# Vérifier statut Git
cd /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker
git status src/vmm/src/rpc_interface.rs

# Vérifier diff
git diff src/vmm/src/rpc_interface.rs
```

### 6.4 Problème: Push échoue

**Symptôme**:
```bash
# ❌ Erreur lors du push
```

**Causes possibles**:
1. Pas de remote configuré
2. Pas d'accès réseau
3. Authentification requise

**Solution**:
```bash
# Vérifier remote
git remote -v

# Configurer remote si nécessaire
git remote add origin https://github.com/user/repo.git

# Push manuel
git push origin HEAD
```

---

## 7. CHECKLIST PRÉ/POST-CYCLE

### 7.1 Checklist Pré-Cycle

**Avant de commencer un nouveau cycle**:

- [ ] Lire LEÇONS_APPRISES_MDBAI.md (version actuelle)
- [ ] Lire PROTOCOLE_MDBAI.md
- [ ] Vérifier répertoire de travail persistant (`/home/lvx/LVX/lumvorax2/src/MDBAI/`)
- [ ] Vérifier clone Firecracker dans répertoire persistant
- [ ] Noter commit Git Firecracker utilisé
- [ ] Créer rapport horodaté dès début cycle

### 7.2 Checklist Post-Cycle

**Après avoir terminé un cycle**:

- [ ] ✅ **Tests compilent** (0 erreur)
- [ ] ✅ **Tests passent** (100% succès)
- [ ] ✅ **Couverture mesurée** (cargo llvm-cov)
- [ ] ✅ **Git commit** (`./git_commit_cycle.sh`)
- [ ] ✅ **Backup créé** (`./backup_cycle.sh`)
- [ ] ✅ **Rapport final** créé et horodaté
- [ ] ✅ **Leçons documentées** (si nouvelles)
- [ ] ✅ **Commit visible** sur GitHub (si push)

### 7.3 Checklist Validation Sécurité

**Vérifier que le travail est sauvegardé**:

```bash
# 1. Vérifier commit Git local
cd /home/lvx/LVX/lumvorax2/src/MDBAI/firecracker
git log -1 --oneline
# ✅ Commit visible

# 2. Vérifier commit Git remote (si push)
git log origin/HEAD -1 --oneline
# ✅ Commit poussé

# 3. Vérifier backup existe
ls -lh /home/lvx/LVX/lumvorax2/src/MDBAI/backups/backup_C*_*.tar.gz | tail -1
# ✅ Backup créé

# 4. Vérifier contenu backup
tar -tzf /home/lvx/LVX/lumvorax2/src/MDBAI/backups/backup_C*_*.tar.gz | head -10
# ✅ Fichiers présents

# 5. Vérifier rapport existe
ls -lh /home/lvx/LVX/lumvorax2/src/MDBAI/RAPPORT_C*_*.md | tail -1
# ✅ Rapport créé
```

**Si TOUTES les vérifications passent**: ✅ **Travail sécurisé !**

---

## 8. RÉFÉRENCES

### 8.1 Documents Liés

- [LEÇONS_APPRISES_MDBAI.md](LEÇONS_APPRISES_MDBAI.md) - Version 2.0.0
- [PROTOCOLE_MDBAI.md](PROTOCOLE_MDBAI.md)
- [RAPPORT_C177_LECONS_APPRISES_MISE_A_JOUR_2026-05-31T0217Z.md](RAPPORT_C177_LECONS_APPRISES_MISE_A_JOUR_2026-05-31T0217Z.md)
- [RAPPORT_SITUATION_CRITIQUE_2026-05-30T2350Z.md](RAPPORT_SITUATION_CRITIQUE_2026-05-30T2350Z.md)

### 8.2 Scripts

- `git_commit_cycle.sh` - Commit Git automatique (4.3 KB)
- `backup_cycle.sh` - Backup .tar.gz automatique (4.2 KB)

### 8.3 Répertoires

- `/home/lvx/LVX/lumvorax2/src/MDBAI/` - Répertoire principal (persistant)
- `/home/lvx/LVX/lumvorax2/src/MDBAI/firecracker/` - Clone Firecracker (persistant)
- `/home/lvx/LVX/lumvorax2/src/MDBAI/backups/` - Archives backups

---

## ⚠️ RÈGLES ABSOLUES

### RÈGLE #1: Jamais Travailler dans /tmp/
❌ **INTERDIT**: Tout travail dans `/tmp/` (volatile)  
✅ **OBLIGATOIRE**: Tout dans `/home/lvx/LVX/lumvorax2/src/MDBAI/`

### RÈGLE #2: Git Commit Après Chaque Cycle
✅ **OBLIGATOIRE**: `./git_commit_cycle.sh` après validation tests  
⏱️ **Temps**: 2 minutes  
💰 **ROI**: Évite 16h récupération

### RÈGLE #3: Backup Après Chaque Cycle
✅ **OBLIGATOIRE**: `./backup_cycle.sh` après commit Git  
⏱️ **Temps**: 1 minute  
💰 **ROI**: Double sécurité

### RÈGLE #4: Vérifier Sauvegarde
✅ **OBLIGATOIRE**: Checklist validation sécurité  
⏱️ **Temps**: 30 secondes  
💰 **ROI**: Tranquillité d'esprit

---

**Fin du guide** — Version 1.0.0 — 2026-05-31T02:23Z

**⚠️ RAPPEL**: Ces protocoles auraient évité la perte de 157 tests (6h de travail). **Utilisation OBLIGATOIRE pour tous les cycles futurs.**