---
name: MDBAI production state
description: Full production status of MDBAI server as of 2026-05-28, all services live
---

# MDBAI Production State — 2026-05-28T16:01Z

## Status: 99% MVP — PRODUCTION LIVE

### Services Running
| Service | State | Detail |
|---------|-------|--------|
| Express server | ✅ port 3001 | Workflow "MDBAI Server" |
| Redis Cloud | ✅ connected | database-MNTBGRVB, GCP europe-west1, 30MB |
| BullMQ worker | ✅ x3 concurrent | queue "analysis-jobs" |
| Telegram bot | ✅ @masterdebugai_bot | /start /help /analyze /status |
| OAuth GitHub | ✅ 302 redirect | /auth/github → github.com OAuth |
| Forensic lib | ✅ 17KB | /home/runner/workspace/src/MDBAI/forensic/libmdbai_forensic.so |
| Dashboard | ✅ SSE live | /dashboard + /dashboard/api/metrics |
| Tests | ✅ 170/170 | 20 suites, 0 failed |

### Health Endpoint
`GET /health` returns: `{"status":"healthy","redis":"connected","telegram":true,"worker":true}`

### What Remains (E2E — non-blocking)
- Test with a real GitHub repo install (requires user to install App on a repo)
- Video demo documentation
- Beta testers recruitment

### Key URLs
- Health: `http://localhost:3001/health`
- Dashboard: `http://localhost:3001/dashboard`
- OAuth: `http://localhost:3001/auth/github` → 302 to GitHub
- Webhook: `POST /webhook/github` (secret: MDBAI_WEBHOOK_SECRET)

**Why saved:** Full production deployment required 3 sessions; state is non-obvious from code alone.
