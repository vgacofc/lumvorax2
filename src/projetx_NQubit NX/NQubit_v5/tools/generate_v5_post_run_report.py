#!/usr/bin/env python3
import argparse
import csv
import json
import subprocess
from pathlib import Path


def run_verify(verify_script: Path, manifest: Path):
    p = subprocess.run(
        ['python', str(verify_script), '--manifest', str(manifest)],
        capture_output=True,
        text=True,
        check=False,
    )
    out = p.stdout.strip().splitlines()
    stats = {'artifact_count': 'n/a', 'missing': 'n/a', 'mismatches': 'n/a', 'exit_code': p.returncode}
    for line in out:
        if '=' in line:
            k, v = line.split('=', 1)
            if k in stats:
                stats[k] = v
    return stats, p.stdout.strip()


def load_stats_v4(v4_stats_path: Path):
    data = {}
    if not v4_stats_path.exists():
        return data
    with v4_stats_path.open(newline='') as f:
        for r in csv.DictReader(f):
            data[r['metric']] = r['value']
    return data


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument('--manifest', required=True)
    p.add_argument('--output', required=True)
    args = p.parse_args()

    manifest = Path(args.manifest)
    output = Path(args.output)
    root = Path(__file__).resolve().parents[1]
    verify_script = root / 'tools' / 'verify_manifest_v5.py'

    stats, verify_raw = run_verify(verify_script, manifest)
    mdata = json.loads(manifest.read_text(encoding='utf-8')) if manifest.exists() else {}
    v4_stats = load_stats_v4(root.parent / 'NQubit_v4' / 'results' / 'nqbit_stats_v2.csv')

    lines = []
    lines.append('# RAPPORT REPLIT EXECUTION V5 FINAL')
    lines.append('')
    lines.append('## 1) Résumé exécution')
    lines.append(f"- Manifest: `{manifest.as_posix()}`")
    lines.append(f"- Artifacts déclarés: {stats['artifact_count']}")
    lines.append(f"- Missing: {stats['missing']}")
    lines.append(f"- Mismatches: {stats['mismatches']}")
    lines.append(f"- Verify exit code: {stats['exit_code']}")
    lines.append('')
    lines.append('## 2) Source d’autorité d’audit')
    lines.append(f"- source_of_truth: `{mdata.get('source_of_truth', 'n/a')}`")
    lines.append('- Décision: audit final basé sur manifest signé V5 (et non sha256 list seule).')
    lines.append('')
    lines.append('## 3) Statut ADC')
    lines.append('- ADC physique réel validé: NON (preuves matérielles/certification manquantes).')
    lines.append('- Pipeline forensic logiciel validé: OUI (si verify=0 mismatch et tests V5 pass).')
    lines.append('')
    lines.append('## 4) KPI V4 de référence (lecture post-run)')
    if v4_stats:
        for k in ['scenarios', 'wins', 'win_rate', 'avg_nx_score', 'avg_classical_score', 'avg_margin', 'avg_energy_delta']:
            lines.append(f"- {k}: {v4_stats.get(k, 'n/a')}")
    else:
        lines.append('- `nqbit_stats_v2.csv` non trouvé.')
    lines.append('')
    lines.append('## 5) Détail brut verify')
    lines.append('```')
    lines.append(verify_raw if verify_raw else '(no output)')
    lines.append('```')
    lines.append('')
    lines.append('## 6) GO/NO-GO')
    if str(stats['missing']) == '0' and str(stats['mismatches']) == '0' and int(stats['exit_code']) == 0:
        lines.append('- GO technique V5: OUI (intégrité manifest cohérente).')
    else:
        lines.append('- GO technique V5: NON (corriger intégrité avant diffusion).')

    output.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'report_written={output}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
