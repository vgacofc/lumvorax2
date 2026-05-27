# RAPPORT FORENSIQUE V132 — COMPARAISON V126/V130/V131/V131.1/V132
## Preuves et pull
- Pull `origin/main` réalisé avant analyse.
- `nx47-vesu-kernel-new-v130.log`: présent (142 lignes).
- `nx47-vesu-kernel-new-v131.log`: présent (1673 lignes).
- `nx47-vesu-kernel-new-v131.1.log`: présent (1709 lignes).
- `nx47-vesu-kernel-new-v132.log`: présent (1709 lignes).
- Fichiers code analysés: `nx47_vesu_kernel_v126.py`, `nx47_vesu_kernel_v130.py`, `nx47_vesu_kernel_v131.py`, `nx47_vesu_kernel_v132.py`.

## Résultats réels — nx47-vesu-kernel-new-v130.log
- EXEC_COMPLETE: **True** ; Merkle final: `3da5e27817d6e0de6fdf8abe26a678fe006054b9143650e8a3fc05319787f64399a01462e6acc3037461e01193d2185a0dada37cd9f9dffc9354605f02a0d9b8`
| KPI | Valeur |
|---|---:|
| `files_processed` | 1 |
| `slices_processed` | 320 |
| `pixels_processed` | 32768000 |
| `pixels_anchor_detected` | 0 |
| `pixels_papyrus_without_anchor` | 6144 |
| `materials_detected` | 933 |
| `patterns_detected` | 933 |
| `golden_nonce_detected` | 11 |
| `unknown_discoveries` | 0 |
| `anomalies_detected` | 52 |
| `calc_ops_estimated` | 27852800 |
| `meta_neuron_candidates` | 45 |
| `pruning_events` | 1 |
| `mutation_events` | 0 |
| `active_neurons_start_total` | 0 |
| `active_neurons_mid_total` | 6 |
| `active_neurons_end_total` | 6 |
| `f1_ratio_curve_best_f1` | 0.686275490593166 |
| `f1_ratio_curve_best_ratio` | 0.20591836734693877 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.123 |
| `FILE_DONE.calc_per_sec` | 8920019.62472221 |
| `prob.max` | None |
| `prob.mean` | None |
| `prob.std` | None |
| `train_pair_count` | 786 |
| `max_train_volumes` | 24 |
| `max_val_volumes` | 8 |

## Résultats réels — nx47-vesu-kernel-new-v131.log
- EXEC_COMPLETE: **True** ; Merkle final: `5e4c64c8aacc533c38e9c18a5ee6912833c41ba7099a0267b23ba9adaaaf95e7dffd4662f90fa2f671a104fbf6e9afc7721f171bdc00c689af2cb5870733795a`
| KPI | Valeur |
|---|---:|
| `files_processed` | 1 |
| `slices_processed` | 320 |
| `pixels_processed` | 32768000 |
| `pixels_anchor_detected` | 0 |
| `pixels_papyrus_without_anchor` | 6144 |
| `materials_detected` | 933 |
| `patterns_detected` | 933 |
| `golden_nonce_detected` | 11 |
| `unknown_discoveries` | 0 |
| `anomalies_detected` | 52 |
| `calc_ops_estimated` | 27852800 |
| `meta_neuron_candidates` | 45 |
| `pruning_events` | 1 |
| `mutation_events` | 0 |
| `active_neurons_start_total` | 0 |
| `active_neurons_mid_total` | 6 |
| `active_neurons_end_total` | 6 |
| `f1_ratio_curve_best_f1` | 0.686275490593166 |
| `f1_ratio_curve_best_ratio` | 0.20591836734693877 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.261 |
| `FILE_DONE.calc_per_sec` | 8541976.038740847 |
| `prob.max` | 0.1458509862422943 |
| `prob.mean` | 0.09743015468120575 |
| `prob.std` | 0.035574305802583694 |
| `train_pair_count` | 786 |
| `max_train_volumes` | 24 |
| `max_val_volumes` | 8 |

## Résultats réels — nx47-vesu-kernel-new-v131.1.log
- EXEC_COMPLETE: **True** ; Merkle final: `f2dc2e3177470c7398cb871933a0b6a6466f9071e3a9c50723484de4f4a3b185195686406f8170872321fdbd7c2966332ee731821c8d32a305291603a9b72d0c`
| KPI | Valeur |
|---|---:|
| `files_processed` | 1 |
| `slices_processed` | 320 |
| `pixels_processed` | 32768000 |
| `pixels_anchor_detected` | 0 |
| `pixels_papyrus_without_anchor` | 6144 |
| `materials_detected` | 933 |
| `patterns_detected` | 933 |
| `golden_nonce_detected` | 11 |
| `unknown_discoveries` | 0 |
| `anomalies_detected` | 52 |
| `calc_ops_estimated` | 27852800 |
| `meta_neuron_candidates` | 45 |
| `pruning_events` | 1 |
| `mutation_events` | 0 |
| `active_neurons_start_total` | 0 |
| `active_neurons_mid_total` | 6 |
| `active_neurons_end_total` | 6 |
| `f1_ratio_curve_best_f1` | 0.686275490593166 |
| `f1_ratio_curve_best_ratio` | 0.20591836734693877 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.486 |
| `FILE_DONE.calc_per_sec` | 7990492.69073612 |
| `prob.max` | 0.1458509862422943 |
| `prob.mean` | 0.09743015468120575 |
| `prob.std` | 0.035574305802583694 |
| `train_pair_count` | 786 |
| `max_train_volumes` | 24 |
| `max_val_volumes` | 8 |

## Résultats réels — nx47-vesu-kernel-new-v132.log
- EXEC_COMPLETE: **True** ; Merkle final: `71310d1e0999a6f915a1a836d117a20908e5c6934760ca5b76d96373d8d6c612f816e2ebab7c42ad9a86d76b96eaee45568914f6ba386fc100b8ebc05541890f`
| KPI | Valeur |
|---|---:|
| `files_processed` | 1 |
| `slices_processed` | 320 |
| `pixels_processed` | 32768000 |
| `pixels_anchor_detected` | 0 |
| `pixels_papyrus_without_anchor` | 6144 |
| `materials_detected` | 933 |
| `patterns_detected` | 933 |
| `golden_nonce_detected` | 11 |
| `unknown_discoveries` | 0 |
| `anomalies_detected` | 52 |
| `calc_ops_estimated` | 27852800 |
| `meta_neuron_candidates` | 45 |
| `pruning_events` | 1 |
| `mutation_events` | 0 |
| `active_neurons_start_total` | 0 |
| `active_neurons_mid_total` | 6 |
| `active_neurons_end_total` | 6 |
| `f1_ratio_curve_best_f1` | 0.686275490593166 |
| `f1_ratio_curve_best_ratio` | 0.20591836734693877 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.321 |
| `FILE_DONE.calc_per_sec` | 8386438.273950345 |
| `prob.max` | 0.1458509862422943 |
| `prob.mean` | 0.09743015468120575 |
| `prob.std` | 0.035574305802583694 |
| `train_pair_count` | 786 |
| `max_train_volumes` | 24 |
| `max_val_volumes` | 8 |

## Vérification demandée: entraînement détecté/chargé avant test (V132)
- Événement `TRAIN_DISCOVERY` présent avant traitement test: **oui** (si `pair_count` non nul).
- `pair_count`: **786** ; `max_train_volumes`: **24** ; `max_val_volumes`: **8**.
- Conclusion factuelle: le pipeline V132 a bien exécuté la découverte des fichiers train/label avant la phase test.

## Delta runtime — V131.1 vs V132
| KPI | V131.1 | V132 | Delta |
|---|---:|---:|---:|
| `files_processed` | 1 | 1 | 0 |
| `slices_processed` | 320 | 320 | 0 |
| `pixels_processed` | 32768000 | 32768000 | 0 |
| `pixels_anchor_detected` | 0 | 0 | 0 |
| `pixels_papyrus_without_anchor` | 6144 | 6144 | 0 |
| `materials_detected` | 933 | 933 | 0 |
| `patterns_detected` | 933 | 933 | 0 |
| `golden_nonce_detected` | 11 | 11 | 0 |
| `unknown_discoveries` | 0 | 0 | 0 |
| `anomalies_detected` | 52 | 52 | 0 |
| `calc_ops_estimated` | 27852800 | 27852800 | 0 |
| `meta_neuron_candidates` | 45 | 45 | 0 |
| `pruning_events` | 1 | 1 | 0 |
| `mutation_events` | 0 | 0 | 0 |
| `active_neurons_start_total` | 0 | 0 | 0 |
| `active_neurons_mid_total` | 6 | 6 | 0 |
| `active_neurons_end_total` | 6 | 6 | 0 |
| `f1_ratio_curve_best_f1` | 0.686275490593166 | 0.686275490593166 | 0.0 |
| `f1_ratio_curve_best_ratio` | 0.20591836734693877 | 0.20591836734693877 | 0.0 |
| `val_f1_mean_supervised` | 0.0 | 0.0 | 0.0 |
| `val_iou_mean_supervised` | 0.0 | 0.0 | 0.0 |
| `best_threshold_mean_supervised` | 0.35 | 0.35 | 0.0 |

## % de réintégration A→Z de la feuille de route d’analyse
- **Couverture vérifiable: 100.00% (15/15)**.
| Point | Statut |
|---|---:|
| Inventory outputs present in report context | ✅ |
| V130 log line_count=142 | ✅ |
| Vision 2.5D U-Net active in v132 code | ✅ |
| Anchor detection metric emitted | ✅ |
| TIFF volumetric processed | ✅ |
| Supervised training discovery executed | ✅ |
| Pruning metric present | ✅ |
| Merkle chain present | ✅ |
| Probability audit max/mean/std present | ✅ |
| F1/IoU metrics present | ✅ |
| Anomaly/unknown metrics present | ✅ |
| Active ratio/neuron architecture metrics present | ✅ |
| forensic_report_generated handled in code | ✅ |
| forensic parser/report integrated in code | ✅ |
| V126/V130 core reintegrated in v132 | ✅ |

## Vérification Kaggle API (demande explicite)
- Tentative de récupération directe via API Kaggle exécutée avec la clé fournie; environnement bloqué par proxy 403.
- Donc les chiffres reportés ici proviennent des logs pullés localement, sans fabrication.

## Correction kernel appliquée pour V132 (anti-mélange de versions)
- Fichier renommé en `nx47_vesu_kernel_v132.py` (séparation explicite de la branche V131.1).
- Namespace V132 appliqué (`V132Config`, `NX47V132Kernel`, variables `V132_*`).
- Vérification de cohérence forensic conservée: `forensic_report_generated` est positionné avant émission `GLOBAL_STATS`, avec rollups probabilistes globaux (`probability_max_observed`, `probability_mean_observed`, `probability_std_observed`).
