# RAPPORT COMPARATIF RÉEL — V126 vs V130 vs V131
## Périmètre et preuves utilisées
- Kernels analysés localement : `nx47_vesu_kernel_v126.py`, `nx47_vesu_kernel_v130.py`, `nx47_vesu_kernel_v131.py`.
- Logs runtime analysés localement : `nx47-vesu-kernel-new-v130.log` (142 lignes), `nx47-vesu-kernel-new-v131.log` (1673 lignes).

## Résultats réels extraits du log V130
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
| `mutation_events` | 0 |
| `pruning_events` | 1 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.123 |
| `FILE_DONE.calc_per_sec` | 8920019.62472221 |
| `probability_audit.max` | 0.1458509862422943 |
| `exec_complete` | True |
| `merkle_last` | `3da5e27817d6e0de6fdf8abe26a678fe006054b9143650e8a3fc05319787f64399a01462e6acc3037461e01193d2185a0dada37cd9f9dffc9354605f02a0d9b8` |

## Résultats réels extraits du log V131
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
| `mutation_events` | 0 |
| `pruning_events` | 1 |
| `val_f1_mean_supervised` | 0.0 |
| `val_iou_mean_supervised` | 0.0 |
| `best_threshold_mean_supervised` | 0.35 |
| `FILE_DONE.elapsed_s` | 3.261 |
| `FILE_DONE.calc_per_sec` | 8541976.038740847 |
| `probability_audit.max` | 0.1458509862422943 |
| `exec_complete` | True |
| `merkle_last` | `5e4c64c8aacc533c38e9c18a5ee6912833c41ba7099a0267b23ba9adaaaf95e7dffd4662f90fa2f671a104fbf6e9afc7721f171bdc00c689af2cb5870733795a` |

## Avant / Après (V130 -> V131 runtime)
| KPI | V130 | V131 | Delta (V131-V130) |
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
| `val_f1_mean_supervised` | 0.0 | 0.0 | 0.0 |
| `val_iou_mean_supervised` | 0.0 | 0.0 | 0.0 |
| `best_threshold_mean_supervised` | 0.35 | 0.35 | 0.0 |

## Comparaison code kernel (V126 -> V130 -> V131)
| Capacité | V126 | V130 | V131 |
|---|---:|---:|---:|
| `supervised_train` | ✅ | ✅ | ✅ |
| `threshold_scan` | ✅ | ✅ | ✅ |
| `golden_nonce_topk` | ✅ | ✅ | ✅ |
| `hysteresis_topology_3d` | ✅ | ✅ | ✅ |
| `train_unet_25d_supervised` | ❌ | ✅ | ✅ |
| `forensic_parser_v130` | ❌ | ❌ | ✅ |
| `forensic_report_v131` | ❌ | ❌ | ✅ |

## Conclusion factuelle
- Les logs V130 et V131 sont présents et parsés localement.
- `files_processed`: V130=1 vs V131=1.
- `pixels_anchor_detected`: V130=0 vs V131=0.
- `golden_nonce_detected`: V130=11 vs V131=11.
- `val_f1_mean_supervised`: V130=0.0 vs V131=0.0.
