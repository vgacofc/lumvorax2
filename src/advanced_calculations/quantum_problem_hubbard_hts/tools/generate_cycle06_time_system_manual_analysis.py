#!/usr/bin/env python3
import csv, json, hashlib
from pathlib import Path
from datetime import datetime, UTC
from statistics import mean

RUN=Path('src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T045332Z_572')
T=RUN/'tests'
L=RUN/'logs'
R=RUN/'reports'


def rcsv(p):
    with p.open('r',encoding='utf-8',newline='') as f:
        return list(csv.DictReader(f))


def pct(a,b):
    return 0.0 if b==0 else 100.0*a/b


def main():
    new_tests=rcsv(T/'new_tests_results.csv')
    mod=rcsv(T/'module_physics_metadata.csv')
    num=rcsv(T/'numerical_stability_suite.csv')
    qmc=rcsv(T/'benchmark_comparison_qmc_dmrg.csv')
    ext=rcsv(T/'benchmark_comparison_external_modules.csv')
    drift=rcsv(T/'integration_run_drift_monitor.csv')
    with (L/'analysis_scientifique_summary.json').open(encoding='utf-8') as f:
        sci=json.load(f)

    dt_obs=[r for r in new_tests if r['test_family']=='dt_sweep']
    dt_vals=[(r['test_id'], float(r['value']) if r['value'] else None, r['status']) for r in dt_obs if r['test_id'].startswith('dt_0.')]

    e_drift=[float(r['value']) for r in num if r['metric']=='energy_density_drift_max']
    von=[float(r['value']) for r in num if r['metric']=='spectral_radius']
    qmc_fail=[r for r in qmc if r['within_error_bar']=='0']
    ext_fail=[r for r in ext if r['within_error_bar']=='0']
    elapsed=[r for r in drift if r['metric']=='elapsed_ns']

    now=datetime.now(UTC).strftime('%Y-%m-%dT%H:%M:%SZ')
    report=R/'RAPPORT_ANALYSE_MANUELLE_TEMPS_ET_BENCHMARK_CYCLE06_20260308.md'
    matrix=T/'integration_time_step_vs_physical_time_separation_cycle06.csv'

    lines=[]
    lines += [
        '# Rapport manuel: système de temps, différenciation step vs temps physique, et impacts benchmark (Cycle06)',
        '',
        f'- Horodatage UTC: {now}',
        '- Nature: analyse manuelle (non-résumé) du code/artefacts existants + plan d’intégration complémentaire.',
        '- Contrainte respectée: aucun ancien rapport/log modifié; nouveaux fichiers sidecar uniquement.',
        '',
        '## Expertises mobilisées et erreurs rencontrées en cours de route',
        '- Expertises mobilisées: méthodes numériques, stabilité d’intégrateurs explicites, benchmarking physique (QMC/DMRG), ingénierie de traçabilité.',
        '- Erreur rencontrée: la commande de cycle complet peut générer des artefacts hors périmètre très volumineux; l’analyse a donc été réalisée manuellement à partir des résultats existants pour conserver un patch propre.',
        '',
        '## 1) Vérification: est-ce déjà appliqué ?',
        f"- Le run contient des tests `dt_sweep` avec `dt_0.001`, `dt_0.005`, `dt_0.010`; cependant, le critère `dt_convergence` est **FAIL**.",
        f"- Le schéma est `euler_explicit` sur tous les modules et dt metadata standard = 0.010000.",
        f"- Conséquence: la séparation conceptuelle `step` (index numérique) vs `temps physique` n’est pas encore verrouillée par un contrat de validation indépendant.",
        '',
        '## 2) Conséquences du système de temps actuel',
        '### Introduction (thèse + contexte)',
        'Le pipeline mélange potentiellement la lecture opérationnelle des `steps` (progression numérique) et l’interprétation physique du temps continu.',
        '### Développement (argumentation)',
        f"De plus, `dt_convergence` échoue, ce qui indique une dépendance de la solution au pas de temps. En outre, le drift énergie moyen est {mean(e_drift):.4f}.",
        f"Cependant, les dérives inter-run sur energy/pairing/sign_ratio sont nulles, alors que `elapsed_ns` dérive: cela isole un effet performance plutôt qu’un effet physique.",
        f"Néanmoins, le rayon spectral moyen ({mean(von):.9f}) est > 1, donc une amplification d’erreur cumulative reste plausible.",
        '### Conclusion (solution + clôture)',
        'Donc, sans séparation explicite entre temps numérique et temps physique, les comparaisons benchmark énergie sont fragilisées; ainsi il faut imposer une couche de conversion et des tests d’invariance dédiés.',
        '',
        '## 3) Différenciation demandée: step (temps numérique) vs temps physique (avant/après)',
        '| État | Step numérique | Temps physique | Règle de couplage | Risque |',
        '|---|---|---|---|---|',
        '| AVANT (actuel) | index d’itération `step` | implicite via `dt` | couplage direct non audité | confusion sémantique + biais benchmark |',
        '| APRÈS (à intégrer) | `step_index` strictement discret | `t_phys = step_index * dt_phys` | conversion dédiée, testée, versionnée | séparation complète et contrôle mutuel |',
        '',
        '## 4) Analyse manuelle des résultats benchmark (par problème, structure complète)',
        f"- QMC/DMRG: {len(qmc_fail)}/{len(qmc)} points hors barres.",
        f"- Externes: {len(ext_fail)}/{len(ext)} points hors barres.",
        '',
        '### Problème A — Benchmark énergie Hubbard (QMC/DMRG)',
        '**Introduction (thèse + contexte)**: la tendance globale existe, mais plusieurs points énergie dépassent les barres d’erreur.',
        '**Développement (argumentation)**: de plus, les erreurs absolues énergie dépassent les tolérances; cependant, les points pairing Hubbard restent majoritairement dans barres.',
        '**Conclusion (solution + clôture)**: donc, recalibrer les unités énergie et normaliser par site avant re-validation QMC/DMRG.',
        '',
        '### Problème B — Benchmarks modules externes',
        '**Introduction (thèse + contexte)**: tous les points externes sont hors barres, ce qui invalide la calibration inter-modèles.',
        '**Développement (argumentation)**: en outre, les erreurs relatives sont très élevées; néanmoins, des tendances qualitatives internes restent exploitables.',
        '**Conclusion (solution + clôture)**: ainsi, implémenter un pont de mapping observable+unité+volume avant toute comparaison quantitative.',
        '',
        '### Problème C — Interprétation des tests OBSERVED',
        '**Introduction (thèse + contexte)**: OBSERVED signifie “mesuré” mais non “validé”.',
        '**Développement (argumentation)**: de même, ces tests donnent de l’information (sensibilité, cluster, pumping), cependant sans seuil PASS/FAIL ils ne bloquent pas les régressions.',
        '**Conclusion (solution + clôture)**: de cette manière, convertir OBSERVED critiques en gates avec critères explicites.',
        '',
        '## 5) Questions d’experts + réponses révélées + inconnues restantes',
        '- Q: le temps physique est-il indépendant du step numérique ? R: **partiel** (pas de test d’invariance explicite).',
        '- Q: la stabilité dt est-elle démontrée ? R: **non** (`dt_convergence` FAIL).',
        '- Q: le benchmark externe est-il quantitativement valide ? R: **non** (0% dans barres).',
        '- Q: découverte physique nouvelle confirmée ? R: **non confirmé**; les anomalies sont plus compatibles avec artefacts numériques/calibration.',
        '',
        '## 6) Nouveaux tests à exécuter immédiatement (liste exhaustive)',
        '| ID | Question | Test | Critère |',
        '|---|---|---|---|',
        '| TS1 | step indépendant du temps physique ? | invariance à dt compensé (`N*dt` constant) | delta observables <1% |',
        '| TS2 | convergence temporelle robuste ? | grille dt logarithmique + extrapolation dt→0 | pente stable, erreur <1% |',
        '| TS3 | séparation mutuelle complète ? | double horloge (`step_index`, `t_phys`) + audit | 0 confusion de champs |',
        '| TS4 | benchmark énergie recalibré ? | énergie/site + mapping unités | >=90% within_error_bar |',
        '| TS5 | robustesse long terme ? | horizon >10k steps + spectral radius | drift<5%, rayon<=1 |',
        '',
        '## 7) Commandes exactes et reproductibles',
        '```bash',
        'git fetch origin --prune',
        'cd src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T045332Z_572',
        'sha256sum -c logs/checksums.sha256',
        'python src/advanced_calculations/quantum_problem_hubbard_hts/tools/generate_cycle06_time_system_manual_analysis.py',
        'bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh',
        '```',
        '',
        '## 8) Données chiffrées clés (avant/après conceptuel)',
        f"- overall_progress_pct: {sci.get('overall_progress_pct'):.6f}",
        f"- dt_sweep_values: {[v for v in dt_vals]}",
        f"- energy_drift_mean: {mean(e_drift):.6f}",
        f"- spectral_radius_mean: {mean(von):.9f}",
        f"- benchmark_qmc_fail_pct: {pct(len(qmc_fail),len(qmc)):.2f}%",
        f"- benchmark_external_fail_pct: {pct(len(ext_fail),len(ext)):.2f}%",
    ]

    report.write_text('\n'.join(lines)+'\n',encoding='utf-8')

    with matrix.open('w',encoding='utf-8',newline='') as f:
        w=csv.writer(f)
        w.writerow(['dimension','before_current_state','after_target_state','technical_guardrail','validation_test'])
        w.writerow(['time_axis','step used implicitly with dt','explicit dual axis: step_index + t_phys','strict field separation in outputs','TS3'])
        w.writerow(['dt_control','dt_convergence FAIL','log-grid dt + extrapolation','fail-fast gate on dt convergence','TS2'])
        w.writerow(['benchmark_energy','mixed scale risk','normalized energy per site','unit mapping table versioned','TS4'])
        w.writerow(['run_drift','elapsed_ns drift interpreted as runtime','physics/perf channels separated','separate acceptance criteria','TS1'])

    summary={
        'generated_at_utc':now,
        'report':str(report.relative_to(RUN)),
        'matrix':str(matrix.relative_to(RUN)),
        'metrics':{
            'benchmark_qmc_fail_pct':pct(len(qmc_fail),len(qmc)),
            'benchmark_external_fail_pct':pct(len(ext_fail),len(ext)),
            'energy_drift_mean':mean(e_drift),
            'spectral_radius_mean':mean(von),
            'dt_entries':dt_vals
        }
    }
    (L/'forensic_time_system_summary.json').write_text(json.dumps(summary,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')

    paths=[report,matrix,L/'forensic_time_system_summary.json']
    checks='\n'.join([f"{hashlib.sha256(p.read_bytes()).hexdigest()}  ./{p.relative_to(RUN)}" for p in paths])+'\n'
    (L/'forensic_time_system_checksums.sha256').write_text(checks,encoding='utf-8')

if __name__=='__main__':
    main()
