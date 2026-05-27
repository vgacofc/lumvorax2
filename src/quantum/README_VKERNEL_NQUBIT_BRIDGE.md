# Bridge C: `v_kernel_quantum.c` (origine) vs copy NQubit NX intégrée

Objectif: tester **sans écraser les logs existants** si une copy C enrichie avec la technologie NQubit NX est exploitable avant intégration complète dans `src/quantum/v_kernel_quantum.c`.

## Commande Replit (copier/coller)
```bash
cd /workspace/Lumvorax && python3 src/quantum/run_vkernel_nqubit_comparison.py
```

## Ce que la commande produit
Chaque exécution crée un nouveau dossier horodaté dans:
- `src/quantum/results_vkernel_compare/<YYYYMMDD_HHMMSS>/`

Artefacts:
- `comparison_summary.json` (résumé complet avant/après + commandes + métriques)
- `comparison_summary.md` (lecture humaine)
- `system_metrics.json` (CPU/RAM/loadavg)
- `hardware_entropy_probe.json` (`/dev/hwrng`, `/dev/random`, `/dev/urandom`)
- `nqubit_forensic_ns.jsonl` (log nanoseconde du runner NX)

Fichier d'index:
- `src/quantum/results_vkernel_compare/LATEST.json`

## Métriques comparées explicitement
- qubits simulés par le kernel d'origine (baseline)
- nqubits simulés par la copy NX
- ratio nqubit/qubit
- score moyen baseline vs score moyen nqubit
- taux de victoire nqubit (`nqubit_win_rate`)

## Garantie "ne pas supprimer les logs existants"
- Les logs historiques ne sont jamais supprimés.
- Chaque run écrit dans un **nouveau dossier**.
- Le log historique legacy `logs_AIMO3/v46/hardware_metrics.log` reste en append-only.
