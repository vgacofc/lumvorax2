# PROMPT REPLIT AGENT — NQubit_v5 (Integrity + Signature + Post-run Analysis)

Tu dois exécuter **strictement** le module `src/projetx_NQubit NX/NQubit_v5` en t’appuyant sur les artefacts runtime V4, puis produire un **rapport V5 post-exécution**.

## Objectifs V5
1. Rejouer la chaîne forensic avec validation d’intégrité canonique.
2. Générer et vérifier `manifest_forensic_v5.json`.
3. Signer le manifest V5 (si clés disponibles) et vérifier la signature.
4. Lancer tests unitaires + intégration + stress V5.
5. Lire les résultats générés en fin d’exécution et produire `RAPPORT_REPLIT_EXECUTION_V5_FINAL.md`.

## Étapes obligatoires
1. `cd src/projetx_NQubit NX/NQubit_v5`
2. Exécuter les tests V5:
   - `make test_unit`
   - `make test_integration`
   - `make test_stress`
3. Régénérer le manifest V5 depuis les artefacts V4:
   - `python tools/build_manifest_v5.py --input-dir '../NQubit_v4/results' --output 'results/manifest_forensic_v5.json'`
4. Vérifier intégrité:
   - `python tools/verify_manifest_v5.py --manifest 'results/manifest_forensic_v5.json'`
5. Signature (si clés présentes dans l’environnement):
   - `bash tools/sign_manifest_v5.sh 'results/manifest_forensic_v5.json' '<private_key.pem>' '<public_key.pem>'`
   - sinon consigner clairement l’indisponibilité des clés.
6. Collecte télémétrie machine:
   - `uname -a > results/replit_env_info_v5.txt`
   - `lscpu >> results/replit_env_info_v5.txt || true`
   - `cat /proc/meminfo >> results/replit_env_info_v5.txt`
7. Génération rapport post-run:
   - `python tools/generate_v5_post_run_report.py --manifest 'results/manifest_forensic_v5.json' --output 'RAPPORT_REPLIT_EXECUTION_V5_FINAL.md'`

## Rapport attendu
Créer `RAPPORT_REPLIT_EXECUTION_V5_FINAL.md` avec:
- Statut global (PASS/FAIL) des tests unitaires, intégration, stress.
- Résultat de vérification du manifest (`missing`, `mismatches`).
- Statut signature (faite / non faite + raison).
- Analyse AVANT/APRÈS V5 sur autorité d’audit et traçabilité.
- Explication claire sur ADC réel: ce qui est validé et non validé.
- Recommandation GO/NO-GO.

## Règles strictes
- Ne pas modifier les versions antérieures.
- Ne pas supprimer d’artefacts existants.
- Tous les nouveaux fichiers doivent rester dans `src/projetx_NQubit NX/NQubit_v5`.
- Si une commande est indisponible, poursuivre et documenter l’écart.
