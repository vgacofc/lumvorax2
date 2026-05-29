---
name: MDBAI secrets mapping
description: Nommage des secrets MDBAI dans Doppler vs config.js
---

**Règle :** Les secrets GitHub App dans Doppler `dev_lumvorax` utilisent le préfixe `MDBAI_*` (pas `GITHUB_*`).

**Mapping :**
- `MDBAI_APP_ID` → `config.github.appId` (via `process.env.GITHUB_APP_ID || process.env.MDBAI_APP_ID`)
- `MDBAI_PRIVATE_KEY` → `config.github.privateKey` (via `process.env.GITHUB_PRIVATE_KEY || process.env.MDBAI_PRIVATE_KEY`)
- `MDBAI_CLIENT_ID` → `config.github.clientId`
- `MDBAI_CLIENT_SECRET` → `config.github.clientSecret`
- `MDBAI_WEBHOOK_SECRET` → `config.github.webhookSecret`

**Pourquoi :** config.js gère le fallback pour les deux nommages. Ne jamais hardcoder les secrets.

**Comment appliquer :** Toujours vérifier que config.js a le fallback `GITHUB_*||MDBAI_*` pour tous les secrets GitHub App.
