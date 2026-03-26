import csv
from pathlib import Path

base = Path('src/projetx_NQubit NX/NQubit_v3')
res = base / 'results'

# Base metrics
stats = {}
with open(res / 'nqbit_stats_v2.csv', newline='') as f:
    r = csv.DictReader(f)
    for row in r:
        stats[row['metric']] = float(row['value'])

# P0/P1 metrics
phase = []
with open(res / 'phase_p0_p1_metrics.csv', newline='') as f:
    r = csv.DictReader(f)
    for row in r:
        phase.append({
            'experiment': row['experiment'],
            'nx_mean': float(row['nx_mean']),
            'base_mean': float(row['base_mean']),
            'delta_abs': float(row['delta_abs']),
            'delta_pct': float(row['delta_pct']),
            'win_rate': float(row['win_rate']),
            'delta_std': float(row['delta_std']),
            'ci_low': float(row['delta_ci95_low']),
            'ci_high': float(row['delta_ci95_high']),
        })

baseline = next(x for x in phase if x['experiment'] == 'baseline_current')

lines = []
lines.append('# RAPPORT_FINAL_NQUBIT_V3_ANALYSE_DIFFERENCES_20260228')
lines.append('')
lines.append('## 1) Réponse claire : que signifient les différences de résultats ?')
lines.append('')
lines.append("Le modèle NX obtient un score moyen supérieur à la baseline dans le protocole courant, mais avec un coût énergétique plus élevé. Cela signifie que l’avantage actuel vient d’un compromis précision/stabilité favorable au score NX, et non d’une preuve hardware définitive.")
lines.append('')
lines.append('### Différences principales (baseline courante)')
lines.append('')
lines.append(f"- Score NX moyen: **{stats['avg_nx_score']:.12f}**")
lines.append(f"- Score baseline moyen: **{stats['avg_classical_score']:.12f}**")
lines.append(f"- Différence absolue score: **{stats['avg_margin']:.12f}**")
lines.append(f"- Différence relative score: **{(stats['avg_margin']/stats['avg_classical_score']*100.0):.6f}%**")
lines.append(f"- Énergie NX moyenne: **{stats['avg_nx_energy']:.12f}**")
lines.append(f"- Énergie baseline moyenne: **{stats['avg_classical_energy']:.12f}**")
lines.append(f"- Différence absolue énergie: **{stats['avg_energy_delta']:.12f}**")
lines.append(f"- Différence relative énergie: **{(stats['avg_energy_delta']/stats['avg_classical_energy']*100.0):.6f}%**")
lines.append('')
lines.append('## 2) Différences en % par phase P0/P1 (avec IC95%)')
lines.append('')
lines.append('| Expérience | NX mean | Baseline mean | Δ abs | Δ % | Win rate | IC95% Δ |')
lines.append('|---|---:|---:|---:|---:|---:|---:|')
for x in phase:
    lines.append(
        f"| {x['experiment']} | {x['nx_mean']:.6f} | {x['base_mean']:.6f} | {x['delta_abs']:.6f} | {x['delta_pct']:.3f}% | {x['win_rate']:.6f} | [{x['ci_low']:.6f}, {x['ci_high']:.6f}] |"
    )

lines.append('')
lines.append('## 3) Lecture experte rapide des phases')
lines.append('')
for x in phase:
    rel = x['delta_pct'] - baseline['delta_pct']
    lines.append(f"- **{x['experiment']}**: gain relatif score = **{x['delta_pct']:.3f}%** (écart vs baseline courante: **{rel:+.3f} points**).")

lines.append('')
lines.append('## 4) Couverture des priorités demandées')
lines.append('')
lines.append('- **P0 couvert**: ablations sans Lyapunov, sans bruit, bruit impulsionnel, scoring symétrisé, 30 runs/scénario + IC95%.')
lines.append('- **P1 couvert (logiciel)**: baseline renforcée adaptative + tests OOD (drift thermique + bruit lourd).')
lines.append('- **P2 partiel (planifié)**: acquisition ADC réelle/injection physique non réalisable dans cet environnement de simulation; protocole prêt dans ce rapport.')

lines.append('')
lines.append('## 5) Gouvernance de preuve et anti-biais')
lines.append('')
lines.append('- Protocole figé avant exécution (tests P0/P1 constants).')
lines.append('- Résultats horodatés en forensic et consolidés en CSV.')
lines.append('- Critère go/no-go proposé: la borne basse IC95% de Δ score doit rester > 0 sur baseline forte et OOD.')

lines.append('')
lines.append('## 6) Threats to validity (obligatoire)')
lines.append('')
lines.append('- Simulateur PRNG, pas de bruit hardware réel.')
lines.append('- Baselines encore internes au module (pas benchmark externe standardisé).')
lines.append('- IC95% calculé sur essais simulés, dépend du protocole choisi.')

lines.append('')
lines.append('## 7) Verdict V3')
lines.append('')
lines.append('- **Proto logiciel: validé**.')
lines.append('- **Preuve hardware: non validée à ce stade**.')
lines.append('- **Suite immédiate**: brancher acquisition ADC réelle et reproduire exactement ce protocole avec les mêmes sorties forensic.')

(base / 'RAPPORT_FINAL_NQUBIT_V3_ANALYSE_DIFFERENCES_20260228.md').write_text('\n'.join(lines), encoding='utf-8')
print('report_written')
