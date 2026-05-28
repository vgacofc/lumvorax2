---
name: MDBAI secrets location
description: Où sont stockés les secrets MDBAI — Doppler vs Replit env vars
---

## Règle

Doppler `dev_lumvorax` est à **100/100 secrets** (limite atteinte). Les nouveaux secrets vont dans **Replit shared env vars**.

**Why:** Tentative de push de 13 secrets a échoué avec "may not contain more than 100 secrets".

**How to apply:**
- Secrets MDBAI core → Doppler dev_lumvorax (MDBAI_*, TELEGRAM_BOT_TOKEN, REDIS_*, SESSION_SECRET, DATABASE_URL)
- Nouveaux secrets (Stripe, Redis AgentMemory, LangCache, Taskforce, Upstash, UpCloud, OVH, Prisma) → Replit shared env vars via `setEnvVars`
- Pour libérer de la place dans Doppler : supprimer les secrets Azure inutilisés (AZURE_CLI_INSTALLED, AZURE_CLI_VERSION, etc.) après validation utilisateur

## Secrets dans Replit shared env vars (ajoutés 2026-05-28)
STRIPE_PUBLISHABLE_KEY, STRIPE_SECRET_KEY, REDIS_AGENT_MEMORY_STORE_ID, REDIS_AGENT_MEMORY_ENDPOINT, REDIS_LANGCACHE_ID, REDIS_LANGCACHE_ENDPOINT, REDIS_LANGCACHE_KEY, TASKFORCE_API_TOKEN, UPSTASH_BOX_API_KEY, UPCLOUD_API_TOKEN, OVH_APPLICATION_KEY, OVH_APPLICATION_SECRET, OVH_CONSUMER_KEY, PRISMA_API_KEY
