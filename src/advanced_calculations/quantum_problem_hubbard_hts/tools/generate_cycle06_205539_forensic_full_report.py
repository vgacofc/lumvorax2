#!/usr/bin/env python3
import csv, json, hashlib
from pathlib import Path
from datetime import datetime, UTC
from statistics import mean
from collections import Counter, defaultdict

RUN=Path('src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T205539Z_465')
T=RUN/'tests'; L=RUN/'logs'; R=RUN/'reports'

def rcsv(p):
    with p.open('r',encoding='utf-8',newline='') as f:
        return list(csv.DictReader(f))

def pct(a,b): return 0.0 if b==0 else 100.0*a/b

def main():
    new=rcsv(T/'new_tests_results.csv')
    progress=rcsv(T/'integration_problem_solution_progress.csv')
    qmc=rcsv(T/'benchmark_comparison_qmc_dmrg.csv')
    ext=rcsv(T/'benchmark_comparison_external_modules.csv')
    expert=rcsv(T/'expert_questions_matrix.csv')
    openq=rcsv(T/'integration_open_questions_backlog.csv')
    terms=rcsv(T/'integration_terms_glossary.csv')
    meta=rcsv(T/'module_physics_metadata.csv')
    num=rcsv(T/'numerical_stability_suite.csv')
    drift=rcsv(T/'integration_run_drift_monitor.csv')
    with (L/'analysis_scientifique_summary.json').open(encoding='utf-8') as f:
        sci=json.load(f)
    with (L/'independent_log_review_summary.json').open(encoding='utf-8') as f:
        indep=json.load(f)

    for r in progress:
        r['p']=float(r['solution_progress_percent'])
        r['remain']=100-r['p']
    progress=sorted(progress,key=lambda x:x['p'],reverse=True)

    status=Counter(r['status'] for r in new)
    fam=defaultdict(lambda:Counter())
    for r in new:
        fam[r['test_family']]['total']+=1
        fam[r['test_family']][r['status']]+=1

    qmc_fail=[r for r in qmc if r['within_error_bar']=='0']
    ext_fail=[r for r in ext if r['within_error_bar']=='0']
    obs=[r for r in new if r['status']=='OBSERVED']

    e_drift=[float(r['value']) for r in num if r['metric']=='energy_density_drift_max']
    vns=[float(r['value']) for r in num if r['metric']=='spectral_radius']

    report=R/'RAPPORT_FORENSIQUE_COMPLET_VALIDATION_205539Z_465_20260308.md'
    question_test=T/'integration_open_questions_to_targeted_tests_205539Z_465.csv'
    summary=L/'forensic_validation_205539_summary.json'
    chks=L/'forensic_validation_205539_checksums.sha256'

    now=datetime.now(UTC).strftime('%Y-%m-%dT%H:%M:%SZ')
    lines=[]
    lines += [
        '# Rapport forensique complet — validation/invalidation des nouveaux tests (run research_20260308T205539Z_465)',
        '',
        f'- Horodatage UTC: {now}',
        '- Méthode: lecture directe des CSV/JSON locaux, extraction manuelle assistée (sans modification des anciens rapports).',
        '',
        '## Phase 1 — Synchronisation & intégrité',
        f"- Progression globale (sci): {sci.get('overall_progress_pct',0):.6f}%.",
        f"- Progression indépendante: {indep.get('progress_score_pct',0):.6f}%.",
        f"- Couverture nouveaux tests: PASS={status['PASS']}, FAIL={status['FAIL']}, OBSERVED={status['OBSERVED']}, TOTAL={len(new)}.",
        '',
        '## Phase 2-3 — Analyse exhaustive A→Z module par module (%, reste)',
        '| Module | Progression % | Reste % | Evidence |',
        '|---|---:|---:|---|'
    ]
    for r in progress:
        lines.append(f"| {r['problem']} | {r['p']:.2f} | {r['remain']:.2f} | {r['evidence']} |")
    lines += [
        '',
        f"- Moyenne inter-modules: {mean(r['p'] for r in progress):.2f}% ; reste moyen {100-mean(r['p'] for r in progress):.2f}%.",
        '',
        '## Phase 4 — Analyse scientifique (énergie, corrélations, pairing, sign ratio)',
        f"- Drift énergie (energy_density_drift_max): min={min(e_drift):.6f}, max={max(e_drift):.6f}, moyenne={mean(e_drift):.6f}.",
        f"- Rayon spectral Von Neumann: min={min(vns):.9f}, max={max(vns):.9f}, moyenne={mean(vns):.9f} (>1 indique risque d’amplification).",
        f"- Benchmark QMC/DMRG invalidé partiellement: {len(qmc_fail)}/{len(qmc)} hors barres ({pct(len(qmc_fail),len(qmc)):.2f}%).",
        f"- Benchmark modules externes invalidé: {len(ext_fail)}/{len(ext)} hors barres ({pct(len(ext_fail),len(ext)):.2f}%).",
        '',
        '## Phase 5 — Cours pédagogique des termes (niveau débutant)',
    ]
    for t in terms:
        lines.append(f"- **{t['term']}**: {t['simple_explanation']}. De plus, lecture: {t['how_to_read_value']}. Cependant, risque: {t['risk_of_misinterpretation']}.")

    lines += ['', '## Phase 6 — Questions expertes, réponses, solutions',
              '| question_id | question | statut | réponse de ce run | solution |',
              '|---|---|---|---|---|']
    for q in expert:
        stat=q['response_status']
        rep='Réponse complète dans les artefacts' if stat=='complete' else 'Réponse partielle: validation supplémentaire requise'
        sol='Conserver gate' if stat=='complete' else 'Ajouter test ciblé + critère quantitatif'
        lines.append(f"| {q['question_id']} | {q['question']} | {stat} | {rep} | {sol} |")

    lines += ['', '## Phase 7 — Benchmark failures: explication individuelle (Introduction / Développement / Conclusion)']
    idx=1
    for grp,rows in [('QMC/DMRG',qmc_fail),('External modules',ext_fail)]:
        lines.append(f"### Groupe {grp}")
        for r in rows:
            lines += [
                f"#### BF{idx:03d} — {r['module']} / {r['observable']} (T={r['T']}, U={r['U']})",
                '**Introduction (thèse + contexte)**: ce point de benchmark est hors tolérance.',
                f"**Développement (argumentation)**: de plus, référence={r['reference']} et modèle={r['model']}; abs_error={r['abs_error']} > error_bar={r['error_bar']}. Cependant, rel_error={float(r['rel_error'])*100:.2f}%, ce qui empêche la validation quantitative.",
                '**Conclusion (solution + clôture)**: donc, recalibrer unités+normalisation, puis relancer benchmark ciblé avant interprétation physique finale.',
                ''
            ]
            idx+=1

    lines += ['## Phase 7bis — Pour chaque OBSERVED: points forts et critiques',
              '| test_family | test_id | point fort | critique | action |',
              '|---|---|---|---|---|']
    for r in obs:
        fami=r['test_family']
        if fami in ('sensitivity','dynamic_pumping'):
            pf='tendance utile mesurée'
            cr='pas de seuil de succès explicite'
            ac='formaliser critère PASS/FAIL'
        elif fami in ('cluster_scale','dt_sweep','exact_solver'):
            pf='couverture exploratoire riche'
            cr='validation incomplète/instable'
            ac='convertir en gate bloquant'
        else:
            pf='observation informative'
            cr='niveau de preuve limité'
            ac='ajouter campagne de confirmation'
        lines.append(f"| {fami} | {r['test_id']} | {pf} | {cr} | {ac} |")

    lines += ['', '## Phase 8-10 — Nouvelles choses non intégrées + tests à lancer',
              '### Questions ouvertes non intégrées',
              '| question_id | question | action recommandée |',
              '|---|---|---|']
    for q in openq:
        lines.append(f"| {q['question_id']} | {q['question']} | {q['recommended_action']} |")

    lines += ['', '### Algorithmes utilisés par simulation (source metadata)',
              '| module | integration_scheme | dt | field_type | utilité |',
              '|---|---|---:|---|---|']
    for m in meta:
        util='intégration temporelle fullscale des observables corrélées'
        lines.append(f"| {m['module']} | {m['integration_scheme']} | {m['dt']} | {m['field_type']} | {util} |")

    lines += ['', '### Commandes exactes reproductibles', '```bash',
              'git fetch origin --prune',
              'cd src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260308T205539Z_465',
              'sha256sum -c logs/checksums.sha256',
              'python src/advanced_calculations/quantum_problem_hubbard_hts/tools/generate_cycle06_205539_forensic_full_report.py',
              'bash src/advanced_calculations/quantum_problem_hubbard_hts/run_research_cycle.sh',
              '```']

    report.write_text('\n'.join(lines)+'\n',encoding='utf-8')

    with question_test.open('w',encoding='utf-8',newline='') as f:
        w=csv.writer(f)
        w.writerow(['question_id','question','targeted_test','metric','acceptance_criteria','priority'])
        for q in openq:
            qid=q['question_id']
            if 'units' in qid:
                row=[qid,q['question'],'unit_consistency_gate','unit_mismatch_count','=0','P1']
            elif 'dt' in qid:
                row=[qid,q['question'],'dt_half_dt_double','observable_delta_pct','<1%','P1']
            elif 'crosscheck' in qid:
                row=[qid,q['question'],'independent_solver_large_lattice','rel_error','<10%','P1']
            elif 'phase' in qid:
                row=[qid,q['question'],'finite_size_scaling_suite','critical_fit_rmse','<0.05','P2']
            else:
                row=[qid,q['question'],'rollback_drill','rollback_latency_s','<60','P2']
            w.writerow(row)

    s={
        'generated_at_utc':now,
        'overall_progress_pct':sci.get('overall_progress_pct'),
        'independent_progress_pct':indep.get('progress_score_pct'),
        'new_tests_status':dict(status),
        'benchmark_qmc_fail_pct':pct(len(qmc_fail),len(qmc)),
        'benchmark_external_fail_pct':pct(len(ext_fail),len(ext)),
        'module_avg_progress_pct':mean(r['p'] for r in progress)
    }
    summary.write_text(json.dumps(s,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')

    dig=[]
    for p in [report,question_test,summary]:
        dig.append(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  ./{p.relative_to(RUN)}")
    chks.write_text('\n'.join(dig)+'\n',encoding='utf-8')

if __name__=='__main__':
    main()
