# RAPPORT_INSPECTION_SIMULATEURS_V4NEXT_FULL_CYCLE33

Inspection statique exhaustive (sans modification des répertoires simulateur).

## src/advanced_calculations/quantum_simulator_v4_staging_next
- file_count: 1166
- line_count: 38215
- stub_or_placeholder_hits: 7
- hardcoding_risk_hits: 1
- version_marker_hits: 205
- entrypoint_candidates:
  - src/advanced_calculations/quantum_simulator_v4_staging_next/README.md
- sample_version_lines:
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: # Consolidation exhaustive RAPPORT AIMO3/NX -> V3 staging
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_30_PROBLEMES_NX35_VS_NX42_20260131_004921_0001.md` | 7 | yes | Integrer seuils p95/p99 dans fusion gate 
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_NX35_VS_NX42_20260131_003849_0001.md` | 23 | review | A valider manuellement (signal non specifique) | #
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_NX35_VS_NX42_20260131_003849_0001.md` | 48 | review | A valider manuellement (signal non specifique) | -
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/mega_report/RAPPORT_MEGA_AUDIT_NX42_30_PROBLEMES.md` | 55 | review | A valider manuellement (signal non specifique) | **Poi
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/v47_arc/RAPPORT_V47_ARC_PROOF_RESULTS_20260202_173919_0001.md` | 43 | review | A valider manuellement (signal non specifiqu
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: ## 3) Synthese d'integration pour la prochaine version
  - src/advanced_calculations/quantum_simulator_v4_staging_next/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: 1. Gate latence p95/p99 + CI95 win rate dans campagne V3.
  - src/advanced_calculations/quantum_simulator_v4_staging_next/EXPLICATION_DETAILLEE_SMOKE_MANIFEST_GATE_ET_BENCHMARK_READY_20260303.md: Objectif: expliquer précisément *de quoi on parle*, ce qui est déjà inclus dans V4-next, ce qui manque encore pour des comparaisons externes
  - src/advanced_calculations/quantum_simulator_v4_staging_next/EXPLICATION_DETAILLEE_SMOKE_MANIFEST_GATE_ET_BENCHMARK_READY_20260303.md: Dans V4-next, le smoke test compile puis lance le test officiel C (`test_quantum_simulator_complete.c`) avant la campagne.

## src/advanced_calculations/quantum_simulator_v5_competitor_cpu
- file_count: 7477
- line_count: 1934395
- stub_or_placeholder_hits: 1270
- hardcoding_risk_hits: 133
- version_marker_hits: 28123
- entrypoint_candidates:
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/README.md
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/qiskit_aer/README.md
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/qiskit_aer/docs/Makefile
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/qiskit_aer/cmake/README.md
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/mqt_ddsim/README.md
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/mqt_ddsim/scripts/README.md
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/projectq/docs/Makefile
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/results/20260303_122226/clones/qutip/README.md
- sample_version_lines:
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: L'exécution des benchmarks pour Lumvorax V4-Next et les simulateurs compétiteurs (dont Qulacs) a été effectuée. Voici mon analyse détaillée 
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: 📊 Rapport d'Analyse Comparative (Quantum V5 CPU)
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: Métrique	Lumvorax V4-Next	Qulacs (Compétiteur)	Autres (Qiskit/Quimb)
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: Lumvorax V4-Next surclasse les standards industriels dans cet environnement. Il permet d'atteindre une profondeur de calcul de 36 qubits ave
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: L'analyse approfondie du rapport de preuves formelles NX-32 a été réalisée en comparaison avec le code source de Lumvorax V4-Next.
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: 📄 Rapport d'Étude : Intégration NX-32 dans Lumvorax V4-Next
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/raaportagent replit.md: Après une lecture ligne par ligne, il apparaît que Lumvorax V4-Next utilise déjà des concepts de base de la thermodynamique de l'information
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/README.md: # Quantum Simulator V5 Competitor CPU Pack
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/README.md: sans toucher aux versions V2/V3/V4 existantes.
  - src/advanced_calculations/quantum_simulator_v5_competitor_cpu/README.md: Par défaut, l'exécution V5 active maintenant un **protocole unifié strict**:

## src/advanced_calculations/quantum_simulator_v4_kaggle_pack
- file_count: 200
- line_count: 5966
- stub_or_placeholder_hits: 0
- hardcoding_risk_hits: 0
- version_marker_hits: 36
- entrypoint_candidates:
- sample_version_lines:
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/RAPPORT_IMPLEMENTATION_V4_KAGGLE_20260303.md: # Rapport implémentation V4 Kaggle + conversion Python
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/summary.md:   - Replit V3 exécuté en priorité.
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/replit_v3_results/campaign_summary.md: # Quantum Simulator V3 Staging Campaign Summary
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/replit_v3_results/campaign_summary.md: ## Intégrité manifest/signature (style V6)
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/replit_v3_results/campaign_summary.json:     "stdout": "[TEST] D\u00e9marrage test unitaire Quantum Simulator...\n[MEMORY_TRACKER] Initialized - tracking enabled\n[MEMORY_TRACKER] A
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/replit_v3_results/manifest_forensic_v3.json:   "version": "v6",
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/kaggle_submission/kernel-metadata.json:   "id": "ndarray2000/lumvorax-quantum-v4",
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040251/kaggle_submission/kernel-metadata.json:   "title": "Lumvorax Quantum V4",
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040704/summary.md:   - Replit V3 exécuté en priorité.
  - src/advanced_calculations/quantum_simulator_v4_kaggle_pack/replit_kaggle_run_20260303_040704/replit_v3_results/campaign_summary.md: # Quantum Simulator V3 Staging Campaign Summary

## src/advanced_calculations/quantum_simulator_v3_staging
- file_count: 336
- line_count: 11570
- stub_or_placeholder_hits: 9
- hardcoding_risk_hits: 1
- version_marker_hits: 140
- entrypoint_candidates:
  - src/advanced_calculations/quantum_simulator_v3_staging/README.md
- sample_version_lines:
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: # Consolidation exhaustive RAPPORT AIMO3/NX -> V3 staging
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_30_PROBLEMES_NX35_VS_NX42_20260131_004921_0001.md` | 7 | yes | Integrer seuils p95/p99 dans fusion gate 
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_NX35_VS_NX42_20260131_003849_0001.md` | 23 | review | A valider manuellement (signal non specifique) | #
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/RAPPORT_COMPARATIF_NX35_VS_NX42_20260131_003849_0001.md` | 48 | review | A valider manuellement (signal non specifique) | -
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/mega_report/RAPPORT_MEGA_AUDIT_NX42_30_PROBLEMES.md` | 55 | review | A valider manuellement (signal non specifique) | **Poi
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: | `logs_AIMO3/NX/v47_arc/RAPPORT_V47_ARC_PROOF_RESULTS_20260202_173919_0001.md` | 43 | review | A valider manuellement (signal non specifiqu
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: ## 3) Synthese d'integration pour la prochaine version
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_INTEGRATION_AIMO3_NX_V3_20260303.md: 1. Gate latence p95/p99 + CI95 win rate dans campagne V3.
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_COMPARAISON_QUBITS_VS_BENCHMARKS_CLASSIQUES_20260303.md: 1. nos résultats mesurés (Lumvorax V3 staging),
  - src/advanced_calculations/quantum_simulator_v3_staging/RAPPORT_COMPARAISON_QUBITS_VS_BENCHMARKS_CLASSIQUES_20260303.md: ### 2.2 Ce que mesure Lumvorax V3 aujourd’hui

## src/advanced_calculations/quantum_simulator_v2_staging
- file_count: 10
- line_count: 1286
- stub_or_placeholder_hits: 2
- hardcoding_risk_hits: 0
- version_marker_hits: 67
- entrypoint_candidates:
- sample_version_lines:
  - src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md: # Rapport avant/après — fusion V2 en staging (sans toucher l'officiel)
  - src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md: ## 3) Ce qui a été ajouté pour la fusion V2
  - src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md:   - exécute le benchmark fusion V2 et produit:
  - src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md: 3. génère le log forensic JSONL complet de fusion V2.
  - src/advanced_calculations/quantum_simulator_v2_staging/RAPPORT_AVANT_APRES_FUSION_V2.md: - fusion V2 isolée et exécutable,
  - src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md: ## 5) Comparaisons possibles (origine, officiel, V6, nouveau staging)
  - src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md: ## 5.3 V6 (`src/quantum/nqubit_v6_integration`)
  - src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md: - V6 = industrialisation forensic pipeline.
  - src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md: ## 6) Comparaison avec le run V2 précédent (65.3%)
  - src/advanced_calculations/quantum_simulator_v2_staging/ANALYSE_FORENSIC_RUN_ON_REPLIT_20260302.md: 7. Faut-il intégrer manifest/signature V6 au benchmark staging ?

## Commandes reproductibles
```bash
python3 src/advanced_calculations/quantum_problem_hubbard_hts/tools/inspect_quantum_simulator_stacks.py src/advanced_calculations/quantum_problem_hubbard_hts
```
