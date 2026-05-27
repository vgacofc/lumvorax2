#!/usr/bin/env python3
"""
LumVorax — Module 17 — Bitcoin Quantum Mining Engine
scripts/clean_supabase.py — Nettoyage Supabase : garder uniquement le dernier run

Usage:
    python3 scripts/clean_supabase.py [--dry-run]

Description:
    Supprime toutes les données Supabase sauf celles du dernier run BTC.
    Conforme : STANDARD_NAMES.md v4.2 §M-BTC17-C38

Variables d'environnement:
    SUPABASE8_API_URL          URL postgresql:// ou https:// du projet
    SUPABASE_SERVICE_ROLE_KEY  JWT service role

Ref : analysechatgpt91.38.md §7 — 2026-04-12
"""

import os
import sys

# Dernier run BTC (run avec record 24 bits)
LAST_BTC_RUN_ID = "btc_20260412T231035Z_2170"

DRY_RUN = "--dry-run" in sys.argv


def get_db_params():
    """Parse l'URL postgresql:// avec urllib pour éviter l'ambiguïté du @ dans le mot de passe."""
    from urllib.parse import urlparse, unquote
    raw = os.environ.get("SUPABASE8_API_URL", "").strip()
    if not raw:
        return None
    if raw.startswith("postgresql://") or raw.startswith("postgres://"):
        parsed = urlparse(raw)
        return {
            "host":     parsed.hostname,
            "port":     parsed.port or 5432,
            "dbname":   parsed.path.lstrip("/") or "postgres",
            "user":     unquote(parsed.username or "postgres"),
            "password": unquote(parsed.password or ""),
        }
    return None


def clean_supabase(dry_run: bool = False):
    try:
        import psycopg2
    except ImportError:
        print("[FATAL] psycopg2 non disponible — pip install psycopg2-binary")
        sys.exit(1)

    params = get_db_params()
    if not params:
        print("[ERREUR] SUPABASE8_API_URL non défini ou invalide")
        sys.exit(1)

    print(f"[CLEAN] Connexion Supabase → {params['host']}:{params['port']}/{params['dbname']}")
    print(f"[CLEAN] Dernier run BTC conservé : {LAST_BTC_RUN_ID}")
    if dry_run:
        print("[CLEAN] MODE DRY-RUN — aucune suppression réelle")

    try:
        conn = psycopg2.connect(connect_timeout=10, **params)
        conn.autocommit = False
        cur = conn.cursor()

        # Tables à nettoyer (garder uniquement le dernier run BTC)
        tables = [
            "quantum_run_files",
            "quantum_csv_rows",
            "run_scores",
            "benchmark_runtime",
        ]

        total_deleted = 0
        for table in tables:
            try:
                cur.execute(f"SELECT COUNT(*) FROM {table}")
                count_before = cur.fetchone()[0]

                cur.execute(
                    f"SELECT COUNT(*) FROM {table} WHERE run_id != %s",
                    (LAST_BTC_RUN_ID,)
                )
                count_to_delete = cur.fetchone()[0]

                if count_to_delete == 0:
                    print(f"[CLEAN] {table} : déjà propre (0 ligne à supprimer) ✅")
                    continue

                if not dry_run:
                    cur.execute(
                        f"DELETE FROM {table} WHERE run_id != %s",
                        (LAST_BTC_RUN_ID,)
                    )
                    deleted = cur.rowcount
                    print(f"[CLEAN] {table} : {count_before} lignes → {count_before - deleted} gardées "
                          f"({deleted} supprimées) ✅")
                    total_deleted += deleted
                else:
                    print(f"[CLEAN] [DRY-RUN] {table} : {count_to_delete} lignes à supprimer "
                          f"(sur {count_before})")
                    total_deleted += count_to_delete

            except Exception as e:
                print(f"[CLEAN] {table} : erreur — {e} (table peut ne pas exister)")
                conn.rollback()
                conn.autocommit = False

        if not dry_run:
            conn.commit()
            print(f"\n[CLEAN] TERMINÉ — {total_deleted} lignes anciennes supprimées ✅")
            print(f"[CLEAN] Données conservées : run_id='{LAST_BTC_RUN_ID}' (24 bits — RECORD C38)")
        else:
            conn.rollback()
            print(f"\n[CLEAN] DRY-RUN — {total_deleted} lignes seraient supprimées")

        cur.close()
        conn.close()

    except Exception as e:
        print(f"[ERREUR] Connexion Supabase : {e}")
        sys.exit(1)


if __name__ == "__main__":
    clean_supabase(dry_run=DRY_RUN)
