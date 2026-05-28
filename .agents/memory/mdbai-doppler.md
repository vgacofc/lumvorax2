---
name: MDBAI Doppler config
description: MDBAI Server workflow Doppler config — token dev_debugai expiré, workaround via dev_lumvorax
---

## Règle

Le workflow MDBAI Server doit utiliser `DOPPLER_TOKEN` (dev_lumvorax), PAS `DOPPLER_MDBAI_TOKEN` (dev_debugai).

**Why:** Le service token `dev_debugai` (`DOPPLER_MDBAI_TOKEN`) était invalide/expiré. Tous les secrets MDBAI (MDBAI_APP_ID, MDBAI_CLIENT_SECRET, MDBAI_PRIVATE_KEY, TELEGRAM_BOT_TOKEN, REDIS_URL, SESSION_SECRET, etc.) sont déjà présents dans `dev_lumvorax`.

**How to apply:** Commande workflow MDBAI Server :
```
cd src/MDBAI && doppler run --token $DOPPLER_TOKEN --project lumvorax --config dev_lumvorax --no-check-version -- node src/server.js
```

Si l'utilisateur fournit un nouveau `DOPPLER_MDBAI_TOKEN` valide, revenir à `--config dev_debugai --token $DOPPLER_MDBAI_TOKEN`.
