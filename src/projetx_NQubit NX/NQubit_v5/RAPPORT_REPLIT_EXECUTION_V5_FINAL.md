# RAPPORT REPLIT EXECUTION V5 FINAL

## 1) Résumé exécution
- Manifest: `results/manifest_forensic_v5.json`
- Artifacts déclarés: 13
- Missing: 0
- Mismatches: 0
- Verify exit code: 0

## 2) Source d’autorité d’audit
- source_of_truth: `manifest_forensic_v5.json (signed)`
- Décision: audit final basé sur manifest signé V5 (et non sha256 list seule).

## 3) Statut ADC
- ADC physique réel validé: NON (preuves matérielles/certification manquantes).
- Pipeline forensic logiciel validé: OUI (si verify=0 mismatch et tests V5 pass).

## 4) KPI V4 de référence (lecture post-run)
- scenarios: 360
- wins: 360
- win_rate: 1.000000000000
- avg_nx_score: 0.361543481940
- avg_classical_score: 0.189412669968
- avg_margin: 0.172130811972
- avg_energy_delta: 13.916848103278

## 5) Détail brut verify
```
artifact_count=13
missing=0
mismatches=0
```

## 6) GO/NO-GO
- GO technique V5: OUI (intégrité manifest cohérente).
