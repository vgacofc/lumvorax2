# Intégration des suggestions (version suivante dédiée)

## Intégré dans cette copie `v4_staging_next`
1. Copie isolée complète de la version précédente pour itération sans régression.
2. Nouveau mode RNG: `baseline_neutralized`.
3. Ajout métrique explicite `max_qubits_width` dans run_config/summary/exports.
4. Runner Replit dédié: `run_on_replit_v4_next.sh`.
5. Garde-fou de dérive: `throughput_regression_flag` dans `campaign_summary.json`.
6. Campagne 3 modes (`hardware_preferred`, `deterministic_seeded`, `baseline_neutralized`).
7. Test automatique de contrat CLI+summary pour valider `max_qubits_width` et nouveau mode.

## Restant à traiter ensuite
- Benchmark externe apples-to-apples réellement exécuté pour les 10 concurrents.
- Calibration fine des seuils de regression throughput sur historique long.
- Ajout d'un mapping strict vers métriques Quantum Volume / MQTBench.
