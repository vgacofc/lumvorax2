# analysechatgpt91.49 — Correctif C49 Agent Ubuntu / Doppler / Mining illimité

Date : 2026-04-16

## Résumé exécutif

Le problème des runs Ubuntu qui s'arrêtaient à 60 secondes venait de deux couches différentes :

1. `btc_mining_runner` a un défaut interne `--duration-s 60` si aucun argument n'est passé.
2. `tools/agent_ubuntu.sh` appliquait aussi `timeout_s=60` si le job reçu ne précisait pas de timeout.

C49 corrige les deux :

- `tools/btc_run_ubuntu.sh` passe maintenant explicitement `--duration-s $BTC_DURATION_S`.
- `BTC_DURATION_S=0` par défaut signifie mining illimité.
- `tools/agent_ubuntu.sh` utilise `DEFAULT_JOB_TIMEOUT_S=0` par défaut, donc aucun timeout agent si le job ne précise rien.
- `timeout_s=0`, `none`, `unlimited` ou `inf` désactive aussi le timeout pour un job spécifique.
- URL Replit et token agent ne doivent plus être figés dans Ubuntu : ils sont synchronisés vers Doppler depuis Replit via `tools/update_doppler_agent_env.sh`.

## Fichiers modifiés

- `src/visualization/server.py`
  - `_agent_token()` priorise maintenant `AGENT_TOKEN`, puis `LUMVORAX_AGENT_TOKEN`, puis dérivation depuis `SESSION_SECRET`.
- `tools/agent_ubuntu.sh`
  - Lecture dynamique de `REPLIT_URL`, `AGENT_TOKEN`, `LUMVORAX_REPLIT_URL`, `LUMVORAX_AGENT_TOKEN`.
  - Si `REPLIT_URL` ou `AGENT_TOKEN` sont absents et que Doppler est disponible, auto-relance via `doppler run --config dev_lumvorax -- bash ...`.
  - Aucun fallback URL/token codé en dur : si ces variables sont absentes, l’agent s’arrête avec une erreur claire.
  - Timeout par défaut désactivé : `DEFAULT_JOB_TIMEOUT_S=0`.
  - Exécution sans `timeout` quand timeout vaut `0`, `none`, `unlimited`, `inf`.
- `tools/btc_run_ubuntu.sh`
  - `BTC_DURATION_S=0` par défaut.
  - `BTC_THREADS` permet de remplacer `nproc`.
  - `BTC_MODE` permet de changer le mode du run.
  - Le binaire reçoit explicitement `--duration-s $BTC_DURATION_S`.
- `tools/update_doppler_agent_env.sh`
  - Nouveau helper Replit pour pousser vers Doppler : `REPLIT_URL`, `AGENT_TOKEN`, `DEFAULT_JOB_TIMEOUT_S`, `BTC_DURATION_S`.
- `STANDARD_NAMES.md`
  - Entrées C49 ajoutées.
- `replit.md`
  - Mémoire projet mise à jour sur les commandes C49.

## URL Replit actuelle

```bash
https://27a39504-b4c2-4915-a364-dc2602b5b12d-00-b19ese52ni0t.worf.replit.dev
```

Cette URL change à chaque nouvelle session/restart Replit. Ne pas la figer côté Ubuntu : synchroniser Doppler depuis Replit.

## Commande shell Replit pour mettre Doppler à jour

Depuis le shell Replit :

```bash
bash tools/update_doppler_agent_env.sh
```

Cette commande récupère automatiquement :

- l'URL Replit via `REPLIT_DEV_DOMAIN`,
- le token live via `http://127.0.0.1:5000/agent/token`,
- `DEFAULT_JOB_TIMEOUT_S` avec défaut `0`,
- `BTC_DURATION_S` avec défaut `0`,

puis pousse tout vers Doppler.

## Commande Ubuntu après synchronisation Doppler

Depuis Ubuntu :

```bash
cd ~/LVX/lumvorax2
git pull origin main
bash tools/agent_ubuntu.sh
```

Si Doppler ne relance pas automatiquement :

```bash
doppler run --config dev_lumvorax -- bash tools/agent_ubuntu.sh
```

Pour lancer le mining BTC directement en illimité :

```bash
cd ~/LVX/lumvorax2
git pull origin main
doppler run -- bash tools/btc_run_ubuntu.sh
```

## Commandes Ubuntu sans Doppler

Depuis fish, utiliser toujours `env ... bash ...` :

```bash
env REPLIT_URL="https://27a39504-b4c2-4915-a364-dc2602b5b12d-00-b19ese52ni0t.worf.replit.dev" AGENT_TOKEN="<token_live>" DEFAULT_JOB_TIMEOUT_S=0 bash ~/LVX/lumvorax2/tools/agent_ubuntu.sh
```

Le token live ne doit pas être inventé ni repris d'un ancien rapport : le récupérer depuis Replit avec le script Doppler ou via `/agent/token` local uniquement.

## Où modifier les durées manuellement

### Mining BTC

Variable principale : `BTC_DURATION_S`.

- Illimité :

```bash
env BTC_DURATION_S=0 doppler run -- bash tools/btc_run_ubuntu.sh
```

- Une heure :

```bash
env BTC_DURATION_S=3600 doppler run -- bash tools/btc_run_ubuntu.sh
```

- Dix minutes :

```bash
env BTC_DURATION_S=600 doppler run -- bash tools/btc_run_ubuntu.sh
```

Le script applique ensuite :

```bash
./btc_mining_runner --duration-s "$BTC_DURATION_S"
```

Donc le défaut interne 60 secondes du binaire n'est plus utilisé par le runner Ubuntu.

### Timeout agent Ubuntu

Variable principale : `DEFAULT_JOB_TIMEOUT_S`.

- Aucun timeout par défaut :

```bash
env DEFAULT_JOB_TIMEOUT_S=0 doppler run -- bash tools/agent_ubuntu.sh
```

- Timeout de 2 heures par job sans `timeout_s` explicite :

```bash
env DEFAULT_JOB_TIMEOUT_S=7200 doppler run -- bash tools/agent_ubuntu.sh
```

Un job individuel peut aussi fixer son propre `timeout_s`. Pour désactiver explicitement sur un job :

```json
{"timeout_s": 0}
```

## Résultats Ubuntu observés C48

Runs observés dans le log fourni :

- 8 threads.
- Hashrate typique : environ 1.0 à 1.6 MH/s.
- Meilleur leading zeros observé : 28 bits.
- NX48 `update_count` observé autour de 1848 → 1964+.
- Chaque run s'arrêtait à 60 secondes car `--duration-s 0` n'était pas encore transmis au binaire.

Conclusion : le moteur fonctionnait, mais le pilotage de durée était incomplet. C49 corrige le pilotage.

## État attendu après pull Ubuntu

Après :

```bash
cd ~/LVX/lumvorax2
git pull origin main
doppler run -- bash tools/btc_run_ubuntu.sh
```

Le log doit afficher :

```text
[C48] Durée mining  : illimitée
[BTC_QM] duration  = INFINI (run jusqu'a 256 bits ou arret manuel)
```

Si le log affiche encore :

```text
[BTC_QM] duration  = 60 s
```

alors Ubuntu n'a pas encore le patch C49 ou le script lancé n'est pas le bon.

## Limite importante

Si le token Doppler Ubuntu est expiré ou invalide, il faut d'abord régénérer/configurer le token Doppler sur Ubuntu. Le correctif C49 synchronise les secrets côté projet Doppler, mais ne peut pas réparer un client Ubuntu qui n'est plus authentifié auprès de Doppler.

## Diagnostic 2026-04-16 après retour Ubuntu

Le mining direct est validé côté Ubuntu : le log affiche maintenant `Durée mining : illimitée` puis `[BTC_QM] duration = INFINI`, avec un run observé au-delà de 70 secondes.

Le problème restant venait de l'agent lancé avec `bash tools/agent_ubuntu.sh` sans environnement Doppler chargé : `REPLIT_URL` et `AGENT_TOKEN` étaient vides. Le script a donc été renforcé pour tenter automatiquement Doppler avec `--config dev` avant d'afficher l'erreur finale.

Correction finale : la config Doppler correcte du projet est `dev_lumvorax`, pas `dev`. La synchronisation Replit écrit maintenant dans la config par défaut et dans `dev_lumvorax`. L'agent Ubuntu auto-relance donc avec :

```bash
doppler run --config dev_lumvorax -- bash tools/agent_ubuntu.sh
```

Validation Replit : les secrets `REPLIT_URL`, `AGENT_TOKEN`, `DEFAULT_JOB_TIMEOUT_S=0`, `BTC_DURATION_S=0` sont visibles via `doppler run --config dev_lumvorax`.
