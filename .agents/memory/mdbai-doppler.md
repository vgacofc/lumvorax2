---
name: MDBAI Doppler config
description: Quelle variable et config Doppler utiliser pour démarrer MDBAI Server
---

**Règle :** Le workflow MDBAI Server doit utiliser `$DOPPLER_TOKEN` avec `--project lumvorax --config dev_lumvorax`.

**Pourquoi :** `DOPPLER_MDBAI_TOKEN` a été révoqué/expiré. `DOPPLER_TOKEN` (62ch) fonctionne avec `dev_lumvorax`. La commande correcte est déjà dans `.replit` ligne 53 :
```
cd src/MDBAI && doppler run --token $DOPPLER_TOKEN --project lumvorax --config dev_lumvorax --no-check-version -- node src/server.js
```

**Comment appliquer :** Si le workflow MDBAI Server tombe en FAILED avec "Invalid Auth token", vérifier que le workflow utilise `$DOPPLER_TOKEN` et `dev_lumvorax` (pas `$DOPPLER_MDBAI_TOKEN` et `dev_debugai`). Un redémarrage du workflow suffit car `.replit` a la bonne commande.

**Vérification :** `doppler secrets download --token "$DOPPLER_TOKEN" --project lumvorax --config dev_lumvorax --no-file --format env | grep TELEGRAM` doit retourner `TELEGRAM_BOT_TOKEN=...`
