# INSTRUCTIONS — Test C256 Validation GPU HANG

**Date**: 2026-05-17  
**Objectif**: Valider si le timeout 650ms cause un GPU HANG et si on peut dépasser 27 dispatches SANS réouverture

---

## 🎯 CE QUE CE TEST VA PROUVER

### Hypothèse 1: Timeout 650ms cause GPU HANG au premier dispatch?
- **Si OUI**: GPU HANG détecté dès le dispatch 1-3
- **Si NON**: Aucun GPU HANG même avec temps d'exécution ~700ms

### Hypothèse 2: Peut-on dépasser 27 dispatches sans réouverture?
- **Si OUI**: Test atteint 50 dispatches sans crash
- **Si NON**: Crash au dispatch 27-28

---

## 📋 PRÉREQUIS

1. **Binaire compilé**: `test_c255v12_reopen` doit exister
2. **Accès sudo**: Nécessaire pour lire dmesg
3. **GPU Gen9**: Intel UHD 620 ou équivalent

---

## 🚀 COMMANDES À EXÉCUTER

### Étape 1: Aller dans le répertoire
```bash
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
```

### Étape 2: Rendre le script exécutable
```bash
chmod +x scripts/test_gpu_hang_no_sudo.sh
```

### Étape 3: Lancer le test avec sudo
```bash
sudo ./scripts/test_gpu_hang_no_sudo.sh
```

**IMPORTANT**: Le test va:
- Vider dmesg pour avoir des logs propres
- Lancer 50 dispatches SANS réouverture périodique
- Monitorer dmesg en temps réel pour détecter GPU HANG
- Générer un rapport complet avec analyse

---

## 📊 PENDANT LE TEST

Vous verrez:
```
╔═══════════════════════════════════════════════════════════════╗
║  Test C256 — Validation GPU HANG (SANS Réouverture)          ║
╚═══════════════════════════════════════════════════════════════╝

[INFO] Configuration du test:
  - Dispatches: 50 (pour dépasser limite 27)
  - Réouverture: DÉSACTIVÉE
  - Batch size: 40M nonces (temps ~700ms > timeout 650ms)

[Dispatch 1/50] batch_size=40265318 (40.27 M nonces)
  ✅ Succès: 0.702 sec, 57.38 MH/s

[Dispatch 2/50] batch_size=40265318 (40.27 M nonces)
  ✅ Succès: 0.703 sec, 57.28 MH/s
...
```

### Scénarios possibles

#### Scénario A: GPU HANG au premier dispatch
```
[Dispatch 1/50] batch_size=40265318 (40.27 M nonces)
  ❌ ERREUR: GPU HANG détecté
[GPU HANG DÉTECTÉ] Dispatch 1
```
→ **Hypothèse timeout CONFIRMÉE**

#### Scénario B: Crash au dispatch 27-28
```
[Dispatch 27/50] ✅ Succès: 0.703 sec
[Dispatch 28/50] ❌ CRASH: Context reset limit exceeded
```
→ **Limite 27 confirmée, timeout NON responsable**

#### Scénario C: 50 dispatches sans problème
```
[Dispatch 50/50] ✅ Succès: 0.771 sec

Dispatches réussis: 50/50
Crashes: 0
GPU HANG détectés: 0
```
→ **Hypothèse timeout RÉFUTÉE, limite 27 DÉPASSÉE**

---

## 📄 RÉSULTATS

### Fichiers générés

Le test créera 3 fichiers dans `logs/validation_c256/`:

1. **`test_no_reopen_YYYYMMDD_HHMMSS.log`**
   - Log complet de l'exécution
   - Tous les dispatches avec timing

2. **`dmesg_YYYYMMDD_HHMMSS.log`**
   - Messages kernel (GPU HANG, timeouts, resets)
   - Preuve forensique des erreurs driver

3. **`result_YYYYMMDD_HHMMSS.md`** ⭐
   - **RAPPORT FINAL** avec analyse complète
   - Réponse définitive aux 2 hypothèses
   - Recommandations techniques

### Lire le rapport final
```bash
cat logs/validation_c256/result_*.md
```

---

## 🔍 INTERPRÉTATION DES RÉSULTATS

### Si GPU HANG au dispatch 1-3
```markdown
### ❌ HYPOTHÈSE TIMEOUT CONFIRMÉE

**Preuves**:
1. GPU HANG détecté dans les premiers dispatches
2. Temps d'exécution ~700ms > timeout 650ms
3. Driver i915 a déclenché GPU HANG rapidement

**Recommandation**: 
- Augmenter timeout driver i915 (650ms → 5000ms)
- OU utiliser solution C255V13 (réouverture périodique)
```

### Si 50 dispatches sans GPU HANG
```markdown
### ✅ HYPOTHÈSE TIMEOUT RÉFUTÉE

**Preuves**:
1. Aucun GPU HANG détecté sur 50 dispatches
2. Limite 27 dispatches dépassée sans réouverture
3. Temps d'exécution ~700ms > timeout 650ms sans crash

**Recommandation**: 
- La solution C255V13 n'est peut-être pas nécessaire
- Le système peut fonctionner sans workaround
```

---

## ⚠️ EN CAS DE PROBLÈME

### Erreur: Binaire non trouvé
```bash
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
make -f Makefile.gen9_native test_c255v12_reopen
```

### Erreur: Permission denied
```bash
# Vérifier que le script est exécutable
chmod +x scripts/test_gpu_hang_no_sudo.sh

# Lancer avec sudo
sudo ./scripts/test_gpu_hang_no_sudo.sh
```

### Test interrompu
```bash
# Vérifier dmesg pour voir ce qui s'est passé
sudo dmesg | tail -n 100

# Relancer le test
sudo ./scripts/test_gpu_hang_no_sudo.sh
```

---

## 📞 APRÈS LE TEST

### Partager les résultats

Envoyez-moi le contenu de:
```bash
cat logs/validation_c256/result_*.md
```

Ou si vous voulez tous les logs:
```bash
tar -czf validation_c256_results.tar.gz logs/validation_c256/
```

### Questions à répondre

1. **GPU HANG détecté au premier dispatch?** OUI / NON
2. **Nombre de dispatches réussis**: X/50
3. **Limite 27 dépassée?** OUI / NON
4. **Messages dmesg critiques**: (copier-coller)

---

## 🎯 OBJECTIF FINAL

Ce test va **DÉFINITIVEMENT** répondre à votre question:

> "Le problème dispatch est-il lié au timeout?"

**Réponse attendue**:
- Si GPU HANG au dispatch 1 → **OUI, timeout est la cause**
- Si 50 dispatches OK → **NON, timeout n'est PAS la cause**
- Si crash au dispatch 27 → **NON, c'est la limite context reset**

---

**Prêt à lancer le test?**
```bash
cd src/advanced_calculations/bitcoin_quantum_mining/level_zero_native
sudo ./scripts/test_gpu_hang_no_sudo.sh
```

**Durée estimée**: 30-60 secondes (selon si crash ou non)