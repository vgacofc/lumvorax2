# COMMANDES CONFIGURATION DOPPLER - EXÉCUTION IMMÉDIATE

**Date**: 2026-05-27  
**Objectif**: Envoyer tokens sur Doppler automatiquement  
**Source**: TOKEN_CLE_PLATFORME.txt

---

## ⚠️ AVERTISSEMENT SÉCURITÉ

**TOUS CES TOKENS SONT COMPROMIS** (exposés publiquement dans ce chat).  
Ils doivent être **RÉVOQUÉS** après upload sur Doppler.

---

## 🚀 ÉTAPE 1: Installation Doppler CLI

```bash
# Installation
curl -Ls https://cli.doppler.com/install.sh | sh

# Vérification
doppler --version
```

---

## 🔐 ÉTAPE 2: Authentification Doppler

```bash
# Login interactif (ouvre navigateur)
doppler login

# OU avec service token (si disponible)
# doppler configure set token dp.st.YOUR_TOKEN_HERE
```

---

## 📦 ÉTAPE 3: Configuration Projet

```bash
# Se placer dans répertoire MDBAI
cd /home/lvx/LVX/lumvorax2/src/MDBAI

# Configurer projet Doppler
doppler setup \
  --project lumvorax \
  --config dev_debugai

# Vérifier
doppler configure
```

---

## 📤 ÉTAPE 4: Upload Secrets (TOKENS COMPROMIS)

### Telegram Bot (✅ VALIDE - À VÉRIFIER)

```bash
doppler secrets set TELEGRAM_BOT_TOKEN="8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8"
```

### Redis Cloud (⚠️ COMPROMIS - À RÉGÉNÉRER)

```bash
# Redis URL complète
doppler secrets set REDIS_URL="redis://default:lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068"

# Redis Host
doppler secrets set REDIS_HOST="redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com"

# Redis Port
doppler secrets set REDIS_PORT="17068"

# Redis Password
doppler secrets set REDIS_PASSWORD="lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip"

# Redis Account Key
doppler secrets set REDIS_ACCOUNT_KEY="Azmsewjo0c37px9v338p5r83jr677w6sro6ni8d2nx29zy85p"
```

### Prisma Postgres (❌ COMPROMIS - À RÉGÉNÉRER)

```bash
# Database URL (pooled)
doppler secrets set DATABASE_URL="postgres://b61c282aa55d28966cbd3cdcb45ca86681712c063eab8556cd544f212eed859a:sk_3TBG35XoftnxSiHZsdzN4@pooled.db.prisma.io:5432/postgres?sslmode=require"

# Database URL (direct)
doppler secrets set DATABASE_URL_DIRECT="postgres://b61c282aa55d28966cbd3cdcb45ca86681712c063eab8556cd544f212eed859a:sk_3TBG35XoftnxSiHZsdzN4@db.prisma.io:5432/postgres?sslmode=require"

# Prisma API Key
doppler secrets set PRISMA_API_KEY="eyJraWQiOiJUa0hEN1ltOUNaQ2xESHYwazEyTEFhWjk4NTdGOE16dWxYTXJBMFpqbWVrIiwiYWxnIjoiUlMyNTYifQ.eyJzdWIiOiJ3b3Jrc3BhY2U6Y21wb2Ewb2pjMDdweHp5ZjltODdrZmpmNyIsImp0aSI6ImFzdGRycmxoZzJmcDV0bWNtaHprNnQ3aSIsImlhdCI6MTc3OTg5OTIwNzE1NX0.ohDYBHDWyNMRcBp5nf-op07HZWtb5c6mLREHJ1UU80xcrTRrDK2gfsZWmGM0Bwp8Mwc4zyImWicjDvOhAiOQ7DBbQ_oqjxVJGJbeqvIaO2_w00LJJow4f1k-VxDhQD6CBcPvXN1Z_g7yZ2vPSa36gIu5czlVRGBqvGOdb9UZlsyZYBBXICgan5kdnVdWCyJ_hRaOhhvDLi1LFyJfElUQpuBp4ezmOOoXZ2IhSst8jzHRitChh7qcK5aVKM1yM2FN_3Rm8Dyeu5-snQRu_sv44whX6RfCf_HzS2XmLF-ztDnVKl0EL6OHclqSc5pQT_-hRF_9nHpais5eGJkAhP838A"

# Database ID
doppler secrets set PRISMA_DATABASE_ID="db_cmpoa13he07z51gfa0tgvpbri"
```

### Taskforce (BullMQ Pro) (❌ COMPROMIS)

```bash
doppler secrets set TASKFORCE_API_TOKEN="d7102033-d4bf-4b91-95df-2e24ab08d2da"
```

### Upstash Box (❌ COMPROMIS)

```bash
doppler secrets set UPSTASH_BOX_API_KEY="box_fbc484766ea9bfdb8bc6ff3c4ca74bee4ad9b9605c90fa07ab11526aaea8a422"
```

### UpCloud (❌ COMPROMIS)

```bash
doppler secrets set UPCLOUD_API_TOKEN="ucat_01KSN6YG7A5CBB8ZA9YAS2K5JB"
```

### OVH Cloud (❌ COMPROMIS)

```bash
doppler secrets set OVH_APPLICATION_KEY="180e017e7ea658b4"
doppler secrets set OVH_APPLICATION_SECRET="09161e9fcb54bbc7a98fab5d6eaa8128"
doppler secrets set OVH_CONSUMER_KEY="f36ebdf91d2be8fbd43f151f41870615"
```

### Configuration Application

```bash
doppler secrets set NODE_ENV="development"
doppler secrets set LOG_LEVEL="debug"
doppler secrets set API_PORT="3000"
```

---

## ✅ ÉTAPE 5: Vérification

```bash
# Lister tous les secrets
doppler secrets

# Vérifier nombre de secrets
doppler secrets | wc -l

# Télécharger en .env (pour vérification locale)
doppler secrets download --no-file --format env > .env.doppler

# Afficher (ATTENTION: secrets visibles)
cat .env.doppler
```

---

## 🔄 ÉTAPE 6: Régénération Immédiate (CRITIQUE)

### Redis Cloud

1. Aller sur https://app.redislabs.com/#/databases
2. Sélectionner `database-MNTBGRVB`
3. Security → Regenerate password
4. Copier nouveau password
5. Mettre à jour Doppler:

```bash
doppler secrets set REDIS_PASSWORD="NOUVEAU_PASSWORD"
doppler secrets set REDIS_URL="redis://default:NOUVEAU_PASSWORD@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068"
```

### Prisma Postgres

1. Aller sur https://console.prisma.io
2. Régénérer API key
3. Mettre à jour Doppler:

```bash
doppler secrets set PRISMA_API_KEY="NOUVELLE_CLE"
```

### Upstash Box

1. Aller sur https://console.upstash.com
2. Régénérer API key
3. Mettre à jour Doppler:

```bash
doppler secrets set UPSTASH_BOX_API_KEY="NOUVELLE_CLE"
```

---

## 🤖 SCRIPT AUTOMATIQUE COMPLET

Copier-coller dans terminal:

```bash
#!/bin/bash
set -e

echo "🚀 Configuration Doppler MDBAI"
echo "================================"

# Vérifier Doppler installé
if ! command -v doppler &> /dev/null; then
    echo "❌ Doppler CLI non installé"
    echo "Installation..."
    curl -Ls https://cli.doppler.com/install.sh | sh
fi

# Vérifier authentification
if ! doppler configure get token &> /dev/null; then
    echo "❌ Non authentifié"
    echo "Lancement authentification..."
    doppler login
fi

# Configuration projet
echo "📦 Configuration projet..."
cd /home/lvx/LVX/lumvorax2/src/MDBAI
doppler setup --project lumvorax --config dev_debugai --no-interactive

# Upload secrets
echo "📤 Upload secrets..."

# Telegram
doppler secrets set TELEGRAM_BOT_TOKEN="8820756284:AAEaeBUd2PIRbPq-_V6gAD0v2sb-lAl-rr8" --silent

# Redis
doppler secrets set REDIS_URL="redis://default:lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip@redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com:17068" --silent
doppler secrets set REDIS_HOST="redis-17068.c327.europe-west1-2.gce.cloud.redislabs.com" --silent
doppler secrets set REDIS_PORT="17068" --silent
doppler secrets set REDIS_PASSWORD="lAOtAuizLK2LyIVlrSdEpPF8CwEaWZip" --silent

# Prisma
doppler secrets set DATABASE_URL="postgres://b61c282aa55d28966cbd3cdcb45ca86681712c063eab8556cd544f212eed859a:sk_3TBG35XoftnxSiHZsdzN4@pooled.db.prisma.io:5432/postgres?sslmode=require" --silent

# Taskforce
doppler secrets set TASKFORCE_API_TOKEN="d7102033-d4bf-4b91-95df-2e24ab08d2da" --silent

# Upstash
doppler secrets set UPSTASH_BOX_API_KEY="box_fbc484766ea9bfdb8bc6ff3c4ca74bee4ad9b9605c90fa07ab11526aaea8a422" --silent

# UpCloud
doppler secrets set UPCLOUD_API_TOKEN="ucat_01KSN6YG7A5CBB8ZA9YAS2K5JB" --silent

# OVH
doppler secrets set OVH_APPLICATION_KEY="180e017e7ea658b4" --silent
doppler secrets set OVH_APPLICATION_SECRET="09161e9fcb54bbc7a98fab5d6eaa8128" --silent
doppler secrets set OVH_CONSUMER_KEY="f36ebdf91d2be8fbd43f151f41870615" --silent

# Config app
doppler secrets set NODE_ENV="development" --silent
doppler secrets set LOG_LEVEL="debug" --silent
doppler secrets set API_PORT="3000" --silent

echo "✅ Upload terminé!"
echo ""
echo "📊 Secrets configurés:"
doppler secrets | grep -E "TELEGRAM|REDIS|DATABASE|TASKFORCE|UPSTASH|UPCLOUD|OVH|NODE_ENV"

echo ""
echo "⚠️  ATTENTION: Tous ces tokens sont COMPROMIS"
echo "🔄 Régénérer immédiatement sur chaque plateforme"
echo ""
echo "✅ Configuration Doppler terminée"
```

---

## 📋 CHECKLIST POST-CONFIGURATION

- [ ] Doppler CLI installé
- [ ] Authentifié sur Doppler
- [ ] Projet configuré (lumvorax/dev_debugai)
- [ ] 13+ secrets uploadés
- [ ] Vérification `doppler secrets` OK
- [ ] **CRITIQUE**: Régénérer TOUS les tokens compromis
- [ ] Mettre à jour Doppler avec nouveaux tokens
- [ ] Supprimer TOKEN_CLE_PLATFORME.txt du repo
- [ ] Ajouter .env* à .gitignore
- [ ] Tester connexion services (Redis, Telegram, etc)

---

## 🆘 EN CAS DE PROBLÈME

### Erreur "Not authenticated"

```bash
doppler login
# Suivre instructions navigateur
```

### Erreur "Project not found"

```bash
# Vérifier projets disponibles
doppler projects

# Si lumvorax n'existe pas, le créer
doppler projects create lumvorax
doppler configs create dev_debugai --project lumvorax
```

### Erreur "Permission denied"

```bash
# Vérifier permissions
doppler whoami

# Contacter admin Doppler si besoin
```

---

## 📞 SUPPORT

**Doppler**: https://docs.doppler.com  
**Projet MDBAI**: /home/lvx/LVX/lumvorax2/src/MDBAI/  
**Guide complet**: GUIDE_DOPPLER_SECRETS.md

---

**⚠️ RAPPEL SÉCURITÉ**: Après upload, RÉVOQUER immédiatement tous les tokens sur chaque plateforme!
