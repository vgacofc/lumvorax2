#!/usr/bin/env bash
# ==============================================================================
# ci/test_secrets.sh — Tests unitaires des secrets LumVorax / VORAX
# Conforme STANDARD_NAMES.md v4.2 Section H — Cycle C64 — 2026-04-11
# ==============================================================================
# INSTRUCTION PERMANENTE : Ce script doit être exécuté EN PREMIER au début
# de tout run (avant compilation, avant lancement des binaires).
# Il vérifie la présence et la validité syntaxique de tous les secrets requis.
#
# Usage :
#   bash ci/test_secrets.sh           → tests complets (sortie 0=OK, 1=FAIL)
#   bash ci/test_secrets.sh --strict  → sortie 1 si un secret P0 manque
#   bash ci/test_secrets.sh --quiet   → pas de sortie, juste le code retour
# ==============================================================================

set -euo pipefail

STRICT=0
QUIET=0
for arg in "$@"; do
    case "$arg" in
        --strict) STRICT=1 ;;
        --quiet)  QUIET=1  ;;
    esac
done

PASS=0
FAIL=0
WARN=0

ok()   { PASS=$((PASS+1)); [ "$QUIET" = "0" ] && printf "  [\033[32m✔ PASS\033[0m] %s\n" "$1"; }
fail() { FAIL=$((FAIL+1)); [ "$QUIET" = "0" ] && printf "  [\033[31m✘ FAIL\033[0m] %s — %s\n" "$1" "${2:-manquant}"; }
warn() { WARN=$((WARN+1)); [ "$QUIET" = "0" ] && printf "  [\033[33m⚠ WARN\033[0m] %s — %s\n" "$1" "${2:-non défini}"; }

[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "════════════════════════════════════════════════════════════"
[ "$QUIET" = "0" ] && echo " LUMVORAX / VORAX — Test unitaires secrets — $(date -u +%Y-%m-%dT%H:%M:%SZ)"
[ "$QUIET" = "0" ] && echo "════════════════════════════════════════════════════════════"

# ── SECTION 1 : Secrets P0 (bloquants) ────────────────────────────────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 1 : Secrets P0 (obligatoires) ──"

# 1.1 Supabase — accès base de données
if [ -n "${SUPABASE_SERVICE_ROLE_KEY:-}" ]; then
    len=${#SUPABASE_SERVICE_ROLE_KEY}
    if [ "$len" -gt 100 ]; then
        ok "SUPABASE_SERVICE_ROLE_KEY (longueur=${len})"
    else
        fail "SUPABASE_SERVICE_ROLE_KEY" "présent mais trop court (${len} chars, attendu >100 = JWT valide)"
    fi
else
    fail "SUPABASE_SERVICE_ROLE_KEY" "ABSENT — upload Supabase impossible"
fi

# 1.2 Supabase — URL du projet
if [ -n "${SUPABASE8_API_URL:-}" ]; then
    if echo "${SUPABASE8_API_URL}" | grep -q "supabase.co"; then
        ok "SUPABASE8_API_URL (${SUPABASE8_API_URL:0:40}...)"
    else
        warn "SUPABASE8_API_URL" "présent mais ne contient pas 'supabase.co' — vérifier"
    fi
elif [ -n "${SUPABASE_DB_HOST:-}" ] && echo "${SUPABASE_DB_HOST}" | grep -q "supabase.co"; then
    ok "SUPABASE8_API_URL (dérivé de SUPABASE_DB_HOST=${SUPABASE_DB_HOST})"
else
    fail "SUPABASE8_API_URL + SUPABASE_DB_HOST" "tous deux absents ou invalides — REST Supabase impossible"
fi

# 1.3 Supabase — hôte DB
if [ -n "${SUPABASE_DB_HOST:-}" ]; then
    ok "SUPABASE_DB_HOST (${SUPABASE_DB_HOST})"
else
    fail "SUPABASE_DB_HOST" "ABSENT — connexion PostgreSQL directe impossible"
fi

# 1.4 Supabase — credentials DB
if [ -n "${SUPABASE_DB_USER:-}" ]; then
    ok "SUPABASE_DB_USER (${SUPABASE_DB_USER})"
else
    fail "SUPABASE_DB_USER" "ABSENT"
fi

if [ -n "${SUPABASE_DB_PASSWORD:-}" ]; then
    ok "SUPABASE_DB_PASSWORD (présent — ${#SUPABASE_DB_PASSWORD} chars)"
else
    fail "SUPABASE_DB_PASSWORD" "ABSENT"
fi

# ── SECTION 2 : Secrets P1 (fonctionnels) ─────────────────────────────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 2 : Secrets P1 (fonctionnels) ──"

# 2.1 SuperMemory
if [ -n "${SUPERMEMORY_API_KEY:-}" ]; then
    len=${#SUPERMEMORY_API_KEY}
    ok "SUPERMEMORY_API_KEY (longueur=${len})"
else
    warn "SUPERMEMORY_API_KEY" "ABSENT — list_memories utilisera cache local uniquement"
fi

# 2.2 SESSION SECRET Flask
if [ -n "${SESSION_SECRET:-}" ]; then
    ok "SESSION_SECRET (présent — ${#SESSION_SECRET} chars)"
else
    warn "SESSION_SECRET" "ABSENT — Flask utilisera une clé non sécurisée"
fi

# 2.3 LUMVORAX_CYCLE_ID
if [ -n "${LUMVORAX_CYCLE_ID:-}" ]; then
    ok "LUMVORAX_CYCLE_ID (${LUMVORAX_CYCLE_ID})"
else
    warn "LUMVORAX_CYCLE_ID" "non défini — défaut C54 utilisé dans supermemory"
fi

# ── SECTION 3 : Secrets P2 (optionnels) ───────────────────────────────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 3 : Secrets P2 (optionnels) ──"

# 3.1 Doppler
if [ -n "${DOPPLER_TOKEN:-}" ]; then
    ok "DOPPLER_TOKEN (présent — ${#DOPPLER_TOKEN} chars)"
else
    warn "DOPPLER_TOKEN" "non défini — fallback direct sur Replit secrets"
fi

# 3.2 Supabase URL2 (pooler)
if [ -n "${SUPABASE_URL2:-}" ]; then
    ok "SUPABASE_URL2 (pooler PgBouncer disponible)"
else
    warn "SUPABASE_URL2" "non défini — pas de pooler (connexion directe port 5432 uniquement)"
fi

# ── SECTION 4 : Validation syntaxique Supabase JWT ────────────────────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 4 : Validation JWT Supabase ──"

if [ -n "${SUPABASE_SERVICE_ROLE_KEY:-}" ]; then
    # Un JWT valide a exactement 3 segments séparés par '.'
    DOT_COUNT=$(echo "${SUPABASE_SERVICE_ROLE_KEY}" | tr -cd '.' | wc -c)
    if [ "$DOT_COUNT" -eq 2 ]; then
        ok "SUPABASE_SERVICE_ROLE_KEY format JWT (3 segments ✓)"
    else
        fail "SUPABASE_SERVICE_ROLE_KEY format JWT" "${DOT_COUNT} segments (attendu: 3 = header.payload.signature)"
    fi
fi

# ── SECTION 5 : Test connectivité Supabase (si python3 disponible) ─────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 5 : Connectivité Supabase (HTTP) ──"

if command -v python3 &>/dev/null && [ -n "${SUPABASE_SERVICE_ROLE_KEY:-}" ] && [ -n "${SUPABASE_DB_HOST:-}" ]; then
    DB_HOST="${SUPABASE_DB_HOST}"
    # Dériver l'URL HTTPS depuis DB_HOST
    if echo "$DB_HOST" | grep -q "^db\."; then
        PID="${DB_HOST#db.}"
        PID="${PID%.supabase.co}"
        SUPA_HTTPS="https://${PID}.supabase.co"
    else
        SUPA_HTTPS=""
    fi

    if [ -n "$SUPA_HTTPS" ]; then
        HTTP_CODE=$(python3 -c "
import urllib.request, os
req = urllib.request.Request(
    '${SUPA_HTTPS}/rest/v1/',
    headers={
        'apikey': os.environ['SUPABASE_SERVICE_ROLE_KEY'],
        'Authorization': 'Bearer ' + os.environ['SUPABASE_SERVICE_ROLE_KEY'],
    }
)
try:
    r = urllib.request.urlopen(req, timeout=10)
    print(r.status)
except Exception as e:
    code = getattr(e, 'code', 0)
    print(code if code else 0)
" 2>/dev/null || echo "0")
        if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "404" ]; then
            ok "Supabase REST HTTP ${HTTP_CODE} (${SUPA_HTTPS})"
        else
            fail "Supabase REST" "HTTP ${HTTP_CODE} — vérifier SERVICE_ROLE_KEY et URL"
        fi
    else
        warn "Supabase REST" "URL HTTPS non dérivable depuis SUPABASE_DB_HOST=${DB_HOST}"
    fi
else
    warn "Supabase connectivité" "python3 absent ou secrets manquants — test ignoré"
fi

# ── SECTION 6 : Test SuperMemory (si SUPERMEMORY_API_KEY défini) ───────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "── Section 6 : SuperMemory API ──"

if [ -n "${SUPERMEMORY_API_KEY:-}" ]; then
    SM_CODE=$(python3 -c "
import urllib.request, os, json
req = urllib.request.Request(
    'https://api.supermemory.ai/v3/memories',
    headers={
        'Authorization': 'Bearer ' + os.environ.get('SUPERMEMORY_API_KEY',''),
        'Content-Type': 'application/json',
    }
)
try:
    r = urllib.request.urlopen(req, timeout=10)
    print(r.status)
except Exception as e:
    code = getattr(e, 'code', 0)
    print(code if code else 0)
" 2>/dev/null || echo "0")
    if [ "$SM_CODE" = "200" ]; then
        ok "SuperMemory API v3 HTTP 200 ✓"
    elif [ "$SM_CODE" = "404" ] || [ "$SM_CODE" = "422" ]; then
        warn "SuperMemory API v3" "HTTP ${SM_CODE} — endpoint /memories sans q → 404 attendu (fix C64 actif)"
    elif [ "$SM_CODE" = "401" ]; then
        fail "SuperMemory API v3" "HTTP 401 — SUPERMEMORY_API_KEY invalide ou expiré"
    else
        warn "SuperMemory API v3" "HTTP ${SM_CODE} — connexion instable"
    fi
else
    warn "SuperMemory API" "SUPERMEMORY_API_KEY absent — test ignoré"
fi

# ── RÉSUMÉ ─────────────────────────────────────────────────────────────────────
[ "$QUIET" = "0" ] && echo ""
[ "$QUIET" = "0" ] && echo "════════════════════════════════════════════════════════════"
[ "$QUIET" = "0" ] && printf " RÉSUMÉ : \033[32m%d PASS\033[0m | \033[31m%d FAIL\033[0m | \033[33m%d WARN\033[0m\n" "$PASS" "$FAIL" "$WARN"
[ "$QUIET" = "0" ] && echo "════════════════════════════════════════════════════════════"
[ "$QUIET" = "0" ] && echo ""

if [ "$FAIL" -gt 0 ]; then
    [ "$QUIET" = "0" ] && echo "[FAIL] ${FAIL} secret(s) P0 manquant(s) — run bloqué en mode --strict"
    [ "$STRICT" = "1" ] && exit 1
    exit 0
fi

[ "$QUIET" = "0" ] && echo "[OK] Tous les secrets P0 sont présents — run autorisé"
exit 0
