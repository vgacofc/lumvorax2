---
name: MDBAI GitHub App credentials
description: GitHub App credentials for MDBAI — what's public vs secret
---

GitHub App for MDBAI is already created.

**Public values (set as Replit env vars in shared environment):**
- GITHUB_APP_ID=3888479
- GITHUB_APP_CLIENT_ID=Iv23liM06X4pQnng7oFm
- App URL: https://github.com/apps/mdbai-master-debug-ai

**Must be Replit Secrets (sensitive, never hardcode):**
- GITHUB_APP_CLIENT_SECRET (from GitHub App settings)
- GITHUB_PRIVATE_KEY (full PEM content of downloaded .pem file)
- GITHUB_WEBHOOK_SECRET (suggested: mdbai_wh_s3cr3t_lumvorax_2026)
- TELEGRAM_BOT_TOKEN (from @BotFather)
- REDIS_URL (from Redis Cloud free tier)

**Why:** App ID and Client ID are public (visible in URL), but private key + client secret + webhook secret are cryptographic credentials.

**How to apply:** Use requestEnvVar() with requestType="secret" for the sensitive ones. Never pass them as env vars.
