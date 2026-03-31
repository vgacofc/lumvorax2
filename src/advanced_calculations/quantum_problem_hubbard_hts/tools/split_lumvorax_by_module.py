#!/usr/bin/env python3
"""
C37-SPLITLOG : split_lumvorax_by_module.py
Sépare le fichier lumvorax (multi-parts) en un fichier CSV par module.
Détecte les changements de module via les lignes MODULE_START.
Applique une rotation par fichier à 20 MB (comme le runner C).
Usage : python3 split_lumvorax_by_module.py <run_dir>
"""
import sys
import os
import glob
import re

ROTATION_BYTES = 20 * 1024 * 1024  # 20 MB

def get_part_suffix(n):
    """Génère un suffixe type _part_aa, _part_ab, ..."""
    alphabet = "abcdefghijklmnopqrstuvwxyz"
    first = alphabet[n // 26 % 26]
    second = alphabet[n % 26]
    return f"_part_{first}{second}"

def split_lumvorax(run_dir):
    logs_dir = os.path.join(run_dir, "logs")
    if not os.path.isdir(logs_dir):
        print(f"[C37-SPLITLOG] logs/ absent dans {run_dir}", flush=True)
        return

    # Trouver tous les fichiers lumvorax (parts inclus), triés
    pattern = os.path.join(logs_dir, "lumvorax_*.csv*")
    all_parts = sorted(glob.glob(pattern))
    # Exclure les fichiers déjà splittés par module (ex: lumvorax_hubbard_hts_core_*.csv)
    # On garde uniquement les fichiers du run principal (lumvorax_hubbard_hts_fullscale_*.csv)
    main_parts = [f for f in all_parts if re.search(r"lumvorax_hubbard_hts_(fullscale|research)", f)]
    if not main_parts:
        # Prendre tous les lumvorax_*.csv qui ne sont pas déjà splittés par module connu
        main_parts = [f for f in all_parts]

    if not main_parts:
        print(f"[C37-SPLITLOG] Aucun fichier lumvorax trouvé dans {logs_dir}", flush=True)
        return

    print(f"[C37-SPLITLOG] {len(main_parts)} part(s) trouvée(s) — début du split par module", flush=True)

    # Lire toutes les lignes de tous les parts dans l'ordre
    current_module = "unknown"
    module_lines = {}  # module_name -> list of lines
    module_order = []

    pat_module = re.compile(r"MODULE_START,.*?,.*?,.*?,.*?,(\w+)$")

    for part_file in main_parts:
        try:
            with open(part_file, "r", errors="replace") as fh:
                for line in fh:
                    # Détection MODULE_START
                    m = pat_module.search(line.rstrip())
                    if m:
                        current_module = m.group(1)
                        if current_module not in module_lines:
                            module_lines[current_module] = []
                            module_order.append(current_module)
                    if current_module not in module_lines:
                        module_lines[current_module] = []
                        if current_module not in module_order:
                            module_order.append(current_module)
                    module_lines[current_module].append(line)
        except Exception as e:
            print(f"[C37-SPLITLOG] Erreur lecture {part_file}: {e}", flush=True)

    if not module_lines:
        print("[C37-SPLITLOG] Aucune ligne trouvée — split ignoré", flush=True)
        return

    # Extraire le PID depuis le nom de fichier (ex: lumvorax_hubbard_hts_fullscale_1234567.csv)
    pid_match = re.search(r"_(\d+)(?:_part_\w+)?\.csv", main_parts[0])
    pid_str = pid_match.group(1) if pid_match else "0"

    print(f"[C37-SPLITLOG] Split de {len(module_order)} modules : {module_order}", flush=True)

    for mod in module_order:
        lines = module_lines[mod]
        part_idx = 0
        byte_count = 0
        out_path = os.path.join(logs_dir, f"lumvorax_{mod}_{pid_str}{get_part_suffix(part_idx)}.csv")
        fh_out = open(out_path, "w")
        print(f"[C37-SPLITLOG] Module {mod} : {len(lines)} lignes → {out_path}", flush=True)

        for line in lines:
            encoded = line.encode("utf-8", errors="replace")
            if byte_count + len(encoded) > ROTATION_BYTES and byte_count > 0:
                fh_out.close()
                part_idx += 1
                out_path = os.path.join(logs_dir, f"lumvorax_{mod}_{pid_str}{get_part_suffix(part_idx)}.csv")
                fh_out = open(out_path, "w")
                byte_count = 0
            fh_out.write(line)
            byte_count += len(encoded)

        fh_out.close()

    print(f"[C37-SPLITLOG] Split terminé — {len(module_order)} fichiers créés dans {logs_dir}", flush=True)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: split_lumvorax_by_module.py <run_dir>", flush=True)
        sys.exit(1)
    split_lumvorax(sys.argv[1])
