---
name: Doppler CLI Replit quirks
description: Problèmes Doppler CLI en env Replit/Codespaces et comment les contourner
---

# Doppler CLI quirks dans Replit

**Règle 1 — Prompt mise à jour bloquant**
Doppler CLI v3.75 affiche un prompt interactif de mise à jour qui bloque toute commande.
Contournement : utiliser `--no-check-version --token $TOKEN` dans chaque commande Doppler.
NE PAS utiliser `DOPPLER_UPDATE_CHECK=false` ni `echo "n" |` (les deux ignorés/bloquent).

**Why:** Upgrade Doppler nécessite sudo, non disponible dans Replit NixOS.

**Règle 2 — Noms GITHUB_* bloqués**
Dans l'env Replit, Doppler refuse d'uploader des secrets dont le nom commence par GITHUB_
(détection GitHub Codespaces). Utiliser les noms MDBAI_* à la place.
config.js a les fallbacks : `process.env.GITHUB_APP_CLIENT_SECRET || process.env.MDBAI_CLIENT_SECRET`

**Why:** Erreur Doppler : "Secret name cannot start with GITHUB_ in GitHub Codespaces"

**Règle 3 — Deux configs Doppler séparées (architecture)**
- `dev_lumvorax` + secret Replit `DOPPLER_TOKEN` → secrets BTC/IBM Quantum
- `dev_debugai` + secret Replit `DOPPLER_MDBAI_TOKEN` → secrets MDBAI uniquement

Workflow MDBAI Server (command exacte) :
`cd src/MDBAI && doppler run --token $DOPPLER_MDBAI_TOKEN --project lumvorax --config dev_debugai --no-check-version -- node src/server.js`

**Règle 4 — Secrets dans dev_debugai (14 clés MDBAI)**
TELEGRAM_BOT_TOKEN, REDIS_URL/HOST/PORT/PASSWORD/USERNAME,
MDBAI_APP_ID=3888479, MDBAI_CLIENT_ID, MDBAI_CLIENT_SECRET, MDBAI_PRIVATE_KEY (RSA PEM),
MDBAI_WEBHOOK_SECRET (64 chars hex), NODE_ENV, LOG_LEVEL, API_PORT=3001

**Règle 5 — dev_debugai ne peut PAS être créé via service token**
Seul un token admin Doppler (personnel) peut créer de nouvelles configs.
Le service token (`dp.st.*`) ne peut que lire/écrire dans la config à laquelle il est scopé.

**How to apply:** À chaque session, npm install + make forensic + workflow démarre automatiquement.
