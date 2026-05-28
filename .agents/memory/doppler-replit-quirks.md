---
name: Doppler CLI Replit quirks
description: Problèmes Doppler CLI en env Replit/Codespaces et comment les contourner
---

# Doppler CLI quirks dans Replit

**Règle 1 — Prompt mise à jour bloquant**
Doppler CLI v3.75 affiche un prompt interactif de mise à jour qui bloque toute commande.
Contournements (par ordre de préférence):
- Utiliser les flags `--no-check-version --token $DOPPLER_TOKEN` dans la commande (meilleur)
- `printf "n\n" | doppler ...` pour répondre non au prompt via stdin
- NE PAS utiliser `DOPPLER_UPDATE_CHECK=false` (ignoré dans cette version)

**Why:** L'upgrade Doppler nécessite sudo non disponible dans Replit NixOS.

**Règle 2 — Noms GITHUB_* bloqués**
Dans l'env Replit/Codespaces, Doppler refuse d'uploader des secrets dont le nom commence par `GITHUB_` (détection Codespaces).
→ Utiliser les noms `MDBAI_*` à la place: MDBAI_WEBHOOK_SECRET, MDBAI_CLIENT_SECRET, MDBAI_APP_ID, MDBAI_CLIENT_ID, MDBAI_PRIVATE_KEY
→ config.js MDBAI a déjà les fallbacks: `process.env.GITHUB_APP_CLIENT_SECRET || process.env.MDBAI_CLIENT_SECRET`

**Why:** Erreur Doppler: "Secret name cannot start with GITHUB_ in GitHub Codespaces"

**Règle 3 — Commande workflow MDBAI correcte**
`cd src/MDBAI && doppler run --token $DOPPLER_TOKEN --project lumvorax --config dev_lumvorax --no-check-version -- node src/server.js`
Config Doppler utilisée: dev_lumvorax (PAS dev_debugai — dev_debugai n'existe pas vraiment)

**How to apply:** À chaque démarrage de session, vérifier que node_modules existe (npm install) et forensic compilée (make dans forensic/). Le workflow utilise déjà les bons flags.
