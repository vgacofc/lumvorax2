"""
supabase_client.py — Module centralisé d'accès Supabase pour le projet HTS.

Usage :
    from tools.supabase_client import SupabaseClient
    sb = SupabaseClient()
    rows = sb.select("quantum_benchmarks", limit=100)
    sb.upsert("run_scores", [{"run_id": 42, "score": 0.98}])

Toutes les interactions avec Supabase passent par ce module unique.
Authentification via variables d'environnement (aucun secret hardcodé) :
    SUPABASE8_API_URL    ou dérivé de SUPABASE_DB_HOST
    SUPABASE_SERVICE_ROLE_KEY

Réf. architecture : C37 session plan — module centralisé POST_MERGE
"""

import os
import json
import urllib.request
import urllib.error
from typing import Any


def _resolve_url() -> str:
    url = os.environ.get("SUPABASE8_API_URL", "").strip().rstrip("/")
    if url:
        return url
    host = os.environ.get("SUPABASE_DB_HOST", "").strip()
    if host:
        project_id = host.split(".")[0]
        return f"https://{project_id}.supabase.co"
    raise RuntimeError(
        "Aucune URL Supabase trouvée. Définissez SUPABASE8_API_URL "
        "ou SUPABASE_DB_HOST dans les secrets Replit."
    )


def _resolve_key() -> str:
    key = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "").strip()
    if not key:
        raise RuntimeError(
            "SUPABASE_SERVICE_ROLE_KEY absent des secrets Replit."
        )
    return key


class SupabaseClient:
    """Client REST Supabase minimal — wraps l'API PostgREST v1."""

    def __init__(self) -> None:
        self.url = _resolve_url()
        self.key  = _resolve_key()
        self._headers = {
            "apikey":        self.key,
            "Authorization": f"Bearer {self.key}",
            "Content-Type":  "application/json",
            "Prefer":        "return=representation",
        }

    # ------------------------------------------------------------------
    # Lecture
    # ------------------------------------------------------------------

    def select(
        self,
        table: str,
        *,
        columns: str = "*",
        filters: dict[str, Any] | None = None,
        limit: int = 1000,
    ) -> list[dict]:
        """SELECT sur une table Supabase.

        Args:
            table   : nom de la table PostgREST
            columns : liste de colonnes (ex: "id,value,status") ou "*"
            filters : dict de filtres égalité {colonne: valeur}
            limit   : nombre max de lignes (max 1000 par défaut)

        Returns:
            Liste de dicts JSON.

        Raises:
            RuntimeError si la requête échoue (HTTP != 200).
        """
        params = f"select={columns}&limit={limit}"
        if filters:
            for k, v in filters.items():
                params += f"&{k}=eq.{v}"
        req = urllib.request.Request(
            f"{self.url}/rest/v1/{table}?{params}",
            headers=self._headers,
            method="GET",
        )
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = resp.read()
                return json.loads(data)
        except urllib.error.HTTPError as e:
            body = e.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"Supabase SELECT {table} → HTTP {e.code}: {body}"
            ) from e

    # ------------------------------------------------------------------
    # Écriture
    # ------------------------------------------------------------------

    def upsert(self, table: str, rows: list[dict]) -> list[dict]:
        """UPSERT (insert or update) dans une table Supabase.

        Args:
            table : nom de la table PostgREST
            rows  : liste de dicts JSON à insérer/mettre à jour

        Returns:
            Lignes insérées/mises à jour (JSON).

        Raises:
            RuntimeError si HTTP != 200/201.
        """
        body = json.dumps(rows).encode("utf-8")
        headers = dict(self._headers)
        headers["Prefer"] = "resolution=merge-duplicates,return=representation"
        req = urllib.request.Request(
            f"{self.url}/rest/v1/{table}",
            data=body,
            headers=headers,
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = resp.read()
                return json.loads(data) if data else []
        except urllib.error.HTTPError as e:
            body_err = e.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"Supabase UPSERT {table} → HTTP {e.code}: {body_err}"
            ) from e

    def insert(self, table: str, rows: list[dict]) -> list[dict]:
        """INSERT dans une table Supabase (sans conflit de clé).

        Args:
            table : nom de la table PostgREST
            rows  : liste de dicts JSON à insérer

        Returns:
            Lignes insérées (JSON).

        Raises:
            RuntimeError si HTTP != 201.
        """
        body = json.dumps(rows).encode("utf-8")
        req = urllib.request.Request(
            f"{self.url}/rest/v1/{table}",
            data=body,
            headers=self._headers,
            method="POST",
        )
        try:
            with urllib.request.urlopen(req, timeout=30) as resp:
                data = resp.read()
                return json.loads(data) if data else []
        except urllib.error.HTTPError as e:
            body_err = e.read().decode("utf-8", errors="replace")
            raise RuntimeError(
                f"Supabase INSERT {table} → HTTP {e.code}: {body_err}"
            ) from e

    # ------------------------------------------------------------------
    # Utilitaire benchmark
    # ------------------------------------------------------------------

    def generate_benchmark_runtime_csv(
        self,
        out_dir: str = "benchmarks",
    ) -> dict[str, int]:
        """Génère les fichiers benchmark runtime au format canonique (STANDARD_NAMES.md).

        Lit quantum_benchmarks depuis Supabase et écrit :
          - qmc_dmrg_reference_runtime.csv       (dataset=qmc_dmrg)
          - external_module_benchmarks_runtime.csv (dataset=external_modules)

        Format de sortie (7 colonnes, NOM D'ORIGINE) :
          source,module,observable,t_k,u_eV,reference_value,error_bar
        Compatible avec load_benchmark_rows() (sscanf 7-colonnes) dans le code C.

        Returns:
            Dict {filename: nombre_de_lignes_écrites}.
        """
        import os as _os

        rows = self.select("quantum_benchmarks", limit=500)
        counts: dict[str, int] = {}

        dataset_map = {
            "qmc_dmrg": "qmc_dmrg_reference_runtime.csv",
            "external": "external_module_benchmarks_runtime.csv",
        }

        for dataset, filename in dataset_map.items():
            subset = [r for r in rows if r.get("dataset", "") == dataset]
            if not subset:
                continue
            path = _os.path.join(out_dir, filename)
            written = 0
            with open(path, "w") as f:
                f.write("source,module,observable,t_k,u_eV,reference_value,error_bar\n")
                for r in subset:
                    try:
                        source = r.get("source", "external")
                        f.write(
                            f"{source},{r['module']},{r['observable']},"
                            f"{float(r['t_k']):.6f},{float(r['u_over_t']):.6f},"
                            f"{float(r['reference_value']):.10f},{float(r['error_bar']):.10f}\n"
                        )
                        written += 1
                    except (KeyError, TypeError, ValueError):
                        continue
            counts[filename] = written

        return counts


# ------------------------------------------------------------------
# CLI minimal (usage: python supabase_client.py select quantum_benchmarks)
# ------------------------------------------------------------------
if __name__ == "__main__":
    import sys

    sb = SupabaseClient()
    if len(sys.argv) >= 3 and sys.argv[1] == "select":
        table = sys.argv[2]
        limit = int(sys.argv[3]) if len(sys.argv) > 3 else 20
        rows  = sb.select(table, limit=limit)
        print(f"[{table}] {len(rows)} lignes")
        for r in rows[:5]:
            print(" ", r)
    elif len(sys.argv) >= 2 and sys.argv[1] == "gen_benchmarks":
        out = sys.argv[2] if len(sys.argv) > 2 else "benchmarks"
        result = sb.generate_benchmark_runtime_csv(out)
        print("Fichiers générés :", result)
    else:
        print("Usage: python supabase_client.py select <table> [limit]")
        print("       python supabase_client.py gen_benchmarks [out_dir]")
