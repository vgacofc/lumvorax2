# RAPPORT FORENSIQUE C52 — VÉRIFICATION INTÉGRALE + SESSION REPLIT KIRK + COMMANDES UBUNTU
## analysechatgpt91.52.md — Session 2026-04-17 ~00h00 UTC

---

## 1. CONTEXTE & OBJECTIF

Continuation directe de C51 (analysechatgpt91.51.md).  
Ce rapport documente :
1. La **vérification ligne par ligne** de l'intégration de toutes les corrections C51 dans le code source Replit.
2. La **nouvelle session Replit** (nœud `kirk`) avec URL et token mis à jour.
3. Les **commandes exactes** pour mettre à jour Doppler manuellement depuis Ubuntu et relancer l'agent + run illimité.
4. Le **push GitHub** permettant à Ubuntu de faire `git pull` et relancer sans limite.

---

## 2. VÉRIFICATION DES CORRECTIONS C51 — LIGNE PAR LIGNE

### 2.1 C50-FIX-P1 — Mutex global_mutex pour data race NX48

**Fichier** : `src/advanced_calculations/bitcoin_quantum_mining/src/btc_mining_engine.c`  
**Lignes vérifiées** : 555–569

```
161:  Écriture record toujours protégée par global_mutex + double-check.
555:  /* C50-FIX-P1 : Copie atomique des scalaires NX48 sous global_mutex.
556:   * BUG ROOT CAUSE : eng->nx48->batch_size_scale et exploration_bias sont
560:   * CORRECTION : lecture sous global_mutex une fois par itération while
566:      pthread_mutex_lock(&eng->global_mutex);
567:      batch       = (int)(cfg->batch_size * eng->nx48->batch_size_scale);
568:      exploration = eng->nx48->exploration_bias;
569:      pthread_mutex_unlock(&eng->global_mutex);
610:  /* exploration : variable locale copiée sous mutex en début de batch (C50-FIX-P1) */
```

**Statut** : ✅ PRÉSENT ET CONFORME

---

### 2.2 C50-FIX-P1b — Null-terminator btc_global_csv

**Fichier** : `src/advanced_calculations/bitcoin_quantum_mining/src/btc_mining_engine.c`  
**Ligne vérifiée** : 861

```
861:  btc_global_csv[sizeof(btc_global_csv)-1] = '\0'; /* C50-FIX-P1b : null-terminator */
```

**Statut** : ✅ PRÉSENT ET CONFORME

---

### 2.3 C50-FIX-P1 (main) — Null-terminator csv_path

**Fichier** : `src/advanced_calculations/bitcoin_quantum_mining/src/main_btc_mining.c`  
**Ligne vérifiée** : 241

```
240:  strncpy(nx48_cfg.csv_path, cfg.nx48_csv, sizeof(nx48_cfg.csv_path)-1);
241:  nx48_cfg.csv_path[sizeof(nx48_cfg.csv_path)-1] = '\0'; /* C50-FIX-P1 : null-terminator explicite */
```

**Statut** : ✅ PRÉSENT ET CONFORME

---

### 2.4 C50-FIX-P2 — Logs horodatés btc_run_ubuntu.sh

**Fichier** : `tools/btc_run_ubuntu.sh`  
**Lignes vérifiées** : 34–38

```bash
# C50-FIX-P2 : Log horodaté — prompt.txt §17.1 INTERDICTION écrasement.
LOG_FILE="$HOME/btc_ubuntu_${STAMP}.log"
ln -sf "$LOG_FILE" "$HOME/btc_ubuntu.log"
```

**Statut** : ✅ PRÉSENT ET CONFORME

---

### 2.5 C50-FIX-P1 — Suppression lecture redondante exploration_bias

**Fichier** : `src/advanced_calculations/bitcoin_quantum_mining/src/btc_mining_engine.c`  
**Ligne 596 vérifiée** : lecture hors mutex supprimée, variable locale `exploration` utilisée.

**Statut** : ✅ PRÉSENT ET CONFORME

---

### Tableau récapitulatif C51

| # | Correction | Fichier | Statut |
|---|-----------|---------|--------|
| P1-main | null-terminator strncpy csv_path | main_btc_mining.c:241 | ✅ INTÉGRÉ |
| P1-engine-mutex | data race batch_size_scale+exploration_bias | btc_mining_engine.c:555-569 | ✅ INTÉGRÉ |
| P1-engine-rdnd | suppression lecture redondante exploration_bias | btc_mining_engine.c:610 | ✅ INTÉGRÉ |
| P1b-csv | null-terminator btc_global_csv | btc_mining_engine.c:861 | ✅ INTÉGRÉ |
| P2-logs | logs horodatés + symlink | btc_run_ubuntu.sh:34-38 | ✅ INTÉGRÉ |

**Conclusion** : 5/5 corrections C51 intégrées — le code est prêt pour un run Ubuntu illimité.

---

## 3. SESSION REPLIT ACTUELLE — NŒUD KIRK

| Paramètre | Valeur |
|-----------|--------|
| URL Replit | `https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev` |
| Token agent | `0d679c8ce780ecd034da1a183d2125df` |
| Nœud | `kirk.replit.dev` (remplace `janeway.replit.dev` C51) |
| Commit HEAD | `a03e528` (chat91.51.2) — poussé sur `origin/main` ✅ |
| Token endpoint | `/agent/token` (localhost uniquement) |

**Note** : le token est généré dynamiquement par `hashlib.sha256(f"agent:{SESSION_SECRET}".encode()).hexdigest()[:32]`. Il change à chaque redémarrage du workflow si `SESSION_SECRET` change. La commande `update_doppler_agent_env.sh` (ci-dessous) récupère toujours le token courant.

---

## 4. COMMANDES DOPPLER — MISE À JOUR MANUELLE DEPUIS UBUNTU

### Option A — Commande de mise à jour rapide (une seule commande)

```bash
doppler secrets set --config dev_lumvorax \
  REPLIT_URL="https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev" \
  AGENT_TOKEN="0d679c8ce780ecd034da1a183d2125df" \
  DEFAULT_JOB_TIMEOUT_S="0" \
  BTC_DURATION_S="0"
```

### Option B — Vérification après mise à jour

```bash
doppler run --config dev_lumvorax -- env | grep -E 'REPLIT_URL|AGENT_TOKEN|BTC_DURATION'
```

Résultat attendu :
```
REPLIT_URL=https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev
AGENT_TOKEN=0d679c8ce780ecd034da1a183d2125df
BTC_DURATION_S=0
```

### Option C — Sans Doppler (si token invalide ou Doppler inaccessible)

```bash
env REPLIT_URL="https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev" \
    AGENT_TOKEN="0d679c8ce780ecd034da1a183d2125df" \
    DEFAULT_JOB_TIMEOUT_S=0 \
    bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
```

---

## 5. PROCÉDURE COMPLÈTE — GIT PULL + AGENT + RUN ILLIMITÉ (copier-coller)

### Étape 1 — Mettre à jour Doppler (une fois par session Replit)

```bash
doppler secrets set --config dev_lumvorax \
  REPLIT_URL="https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev" \
  AGENT_TOKEN="0d679c8ce780ecd034da1a183d2125df" \
  DEFAULT_JOB_TIMEOUT_S="0" \
  BTC_DURATION_S="0"
```

### Étape 2 — Pull + agent (fenêtre 1)

```bash
cd ~/LVX/lumvorax2 && git pull origin main && doppler run --config dev_lumvorax -- bash tools/agent_ubuntu.sh
```

### Étape 3 — Run BTC illimité via agent (depuis Replit — endpoint /agent/push)

Le run est lancé automatiquement par l'agent quand Replit pousse un job. Sinon, depuis Ubuntu directement :

```bash
cd ~/LVX/lumvorax2 && git pull origin main && doppler run --config dev_lumvorax -- bash tools/btc_run_ubuntu.sh
```

### Étape 4 — Commande fish-safe (si shell fish Ubuntu)

```bash
env REPLIT_URL="https://6c9f7a5a-b76e-43ca-884d-268195c88fd5-00-3ucw08503kjzp.kirk.replit.dev" AGENT_TOKEN="0d679c8ce780ecd034da1a183d2125df" DEFAULT_JOB_TIMEOUT_S=0 bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
```

---

## 6. RÈGLES FISH SHELL — ANTI-ERREURS (rappel C49)

| Situation | Mauvaise commande | Bonne commande |
|-----------|-------------------|----------------|
| Effacer une variable | `unset VAR` | `set -e VAR` |
| Lancer doppler sans config | `doppler run -- bash ...` | `doppler run --config dev_lumvorax -- bash ...` |
| Passer des variables | `VAR=val commande` | `env VAR=val bash script.sh` |
| Mettre à jour URL Replit | modifier le script .sh | `doppler secrets set --config dev_lumvorax REPLIT_URL="..."` |

---

## 7. ÉTAT DU DÉPÔT GITHUB

| Paramètre | Valeur |
|-----------|--------|
| Remote origin | `git@github.com:vgacofc/lumvorax2.git` |
| Commit HEAD | `a03e528` — `chat91.51.2` |
| Branch | `main` |
| Statut push | ✅ À jour (HEAD = origin/main) |
| Fichiers modifiés | `.pyc` non versionné + `attached_assets/` non versionnés |

> **Pour récupérer sur Ubuntu** : `git pull origin main` — suffisant, pas de rebase ni force-push nécessaire.

---

## 8. PROCHAINES ÉTAPES

1. ☐ Utilisateur : exécuter les commandes Doppler §4 depuis Ubuntu (une fois)
2. ☐ Utilisateur : lancer `git pull + agent_ubuntu.sh` — fenêtre 1
3. ☐ Replit→Agent : pousser job BTC run illimité via `/agent/push`
4. ☐ Analyser résultats du Run 3 Ubuntu (premier run illimité sans SIGSEGV C51)
5. ☐ Produire rapport C53 avec métriques Run 3 si dépassement du record 33 bits

---

## 9. MISE À JOUR STANDARD_NAMES.md

Entrées ajoutées (§3 ci-dessous, section C52) :
```
2026-04-17 00:00 - C52-VERIF-C51 — Vérification intégrale 5/5 corrections C51 dans code Replit
2026-04-17 00:00 - REPLIT_URL_C52 — https://...kirk.replit.dev (nœud kirk, session 2026-04-17)
2026-04-17 00:00 - AGENT_TOKEN_C52 — 0d679c8ce780ecd034da1a183d2125df
2026-04-17 00:00 - analysechatgpt91.52.md — Rapport C52 : vérif C51, session kirk, commandes Doppler, procédure run illimité
```

---

*Rapport généré automatiquement — session LUMVORAX C52 — 2026-04-17 ~00h00 UTC*  
*Conformité : STANDARD_NAMES.md v4.2 + prompt.txt v2.1*  
*Lignes : ~150*
