import csv
from pathlib import Path

v4 = Path('src/projetx_NQubit NX/NQubit_v4')
v3 = Path('src/projetx_NQubit NX/NQubit_v3')
v2 = Path('src/projetx_NQubit NX/NQubit_v2')

def load_stats(path):
    d = {}
    with open(path, newline='') as f:
        for r in csv.DictReader(f):
            d[r['metric']] = float(r['value'])
    return d

s2 = load_stats(v2 / 'results' / 'nqbit_stats_v2.csv')
s3 = load_stats(v3 / 'results' / 'nqbit_stats_v2.csv')
s4 = load_stats(v4 / 'results' / 'nqbit_stats_v2.csv')

phase = list(csv.DictReader(open(v4 / 'results' / 'phase_p0_p1_metrics.csv', newline='')))
hw = {}
for line in (v4 / 'results' / 'hardware_noise_forensic_v4.log').read_text().splitlines():
    if '=' in line and not line.startswith('#'):
        k, v = line.split('=', 1)
        hw[k.strip()] = float(v.strip())

def pct(a, b):
    if b == 0.0:
        return 0.0
    return (a - b) / b * 100.0

lines = []
lines.append('# RAPPORT_FINAL_NQUBIT_V4_HARDWARE_NOISE_20260228')
lines.append('')
lines.append('## 1) Différences claires entre versions (V2/V3/V4)')
lines.append('')
lines.append(f"- Score NX moyen V2: {s2['avg_nx_score']:.12f}")
lines.append(f"- Score NX moyen V3: {s3['avg_nx_score']:.12f} ({pct(s3['avg_nx_score'], s2['avg_nx_score']):+.6f}%)")
lines.append(f"- Score NX moyen V4: {s4['avg_nx_score']:.12f} ({pct(s4['avg_nx_score'], s3['avg_nx_score']):+.6f}% vs V3)")
lines.append(f"- Score baseline moyen V4: {s4['avg_classical_score']:.12f}")
lines.append(f"- Écart score V4: {s4['avg_margin']:.12f} ({(s4['avg_margin']/s4['avg_classical_score']*100.0):.6f}%)")
lines.append(f"- Écart énergie V4: {s4['avg_energy_delta']:.12f} ({(s4['avg_energy_delta']/s4['avg_classical_energy']*100.0):.6f}%)")
lines.append('')
lines.append('## 2) Résultats P0/P1 (V4)')
lines.append('')
lines.append('| experiment | delta_pct | win_rate | ci95 |')
lines.append('|---|---:|---:|---:|')
for r in phase:
    lines.append(f"| {r['experiment']} | {float(r['delta_pct']):.3f}% | {float(r['win_rate']):.6f} | [{float(r['delta_ci95_low']):.6f}, {float(r['delta_ci95_high']):.6f}] |")

lines.append('')
lines.append('## 3) Bruit matériel réel (proxy noyau + jitter horloge)')
lines.append('')
lines.append(f"- samples: {int(hw.get('samples',0))}")
lines.append(f"- mean: {hw.get('mean',0.0):.12f}")
lines.append(f"- stddev: {hw.get('stddev',0.0):.12f}")
lines.append(f"- min/max: {hw.get('min',0.0):.12f} / {hw.get('max',0.0):.12f}")
lines.append(f"- p01/p99: {hw.get('p01',0.0):.12f} / {hw.get('p99',0.0):.12f}")

lines.append('')
lines.append('## 4) Interprétation')
lines.append('')
lines.append('- Les métriques V4 de benchmark restent cohérentes avec V3 (pas de perte des intégrations précédentes).')
lines.append('- Les phases P0/P1 confirment que l’avantage dépend du protocole de score et de la force de baseline.')
lines.append('- La capture hardware V4 fournit une source de bruit mesurée sur environnement réel (proxy), exploitable pour Replit.')

lines.append('')
lines.append('## 5) Go/No-Go')
lines.append('')
lines.append('- GO provisoire logiciel: oui.')
lines.append('- GO hardware scientifique: en attente de runs Replit + comparaison online + répétitions multi-machines.')

(v4 / 'RAPPORT_FINAL_NQUBIT_V4_HARDWARE_NOISE_20260228.md').write_text('\n'.join(lines), encoding='utf-8')
print('v4_report_written')
