---
name: MDBAI Doppler secrets
description: Doppler GITHUB_ prefix restriction and MDBAI_ workaround for all GitHub App secrets
---

# MDBAI Doppler Secrets — Naming Convention

## Rule
Doppler blocks secrets named `GITHUB_*` in GitHub Codespaces environments. All GitHub App credentials must be stored under the `MDBAI_` prefix.

**Why:** Replit runs inside a GitHub Codespaces-like environment where Doppler enforces a restriction on `GITHUB_` prefixed secret names.

**How to apply:**
- Store as: `MDBAI_APP_ID`, `MDBAI_CLIENT_ID`, `MDBAI_CLIENT_SECRET`, `MDBAI_WEBHOOK_SECRET`, `MDBAI_PRIVATE_KEY`
- `config.js` reads both `GITHUB_*` (env vars set directly) and `MDBAI_*` (Doppler) with fallback: `process.env.GITHUB_APP_ID || process.env.MDBAI_APP_ID`
- Project: `lumvorax`, config: `dev_lumvorax`, flag: `--no-check-version`

## Current Doppler State (2026-05-28) — 12/12 complete
| Secret | Status |
|--------|--------|
| TELEGRAM_BOT_TOKEN | ✅ |
| REDIS_URL | ✅ |
| REDIS_HOST / REDIS_PORT / REDIS_PASSWORD / REDIS_USERNAME | ✅ |
| MDBAI_APP_ID | ✅ 3888479 |
| MDBAI_CLIENT_ID | ✅ Iv23liM06X4pQnng7oFm |
| MDBAI_CLIENT_SECRET | ✅ |
| MDBAI_WEBHOOK_SECRET | ✅ mdbai_wh_s3cr3t_lumvorax_2026 |
| MDBAI_PRIVATE_KEY | ✅ RSA 2048 PEM |
| API_PORT | ✅ 3001 |
| SESSION_SECRET | ✅ |
