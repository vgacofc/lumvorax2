# NQubit_v5 — Correctifs d’intégrité forensic et clarification audit final

Cette version V5 ajoute une chaîne d’intégrité **canonique et vérifiable** sans modifier les anciens artefacts V4.

## Objectif V5
- Définir quel artefact fait foi pour l’audit final.
- Ajouter un mécanisme de signature au-delà du SHA256.
- Fournir un protocole vérifiable de génération/validation de manifest.
- Fournir un prompt Replit V5 + rapport post-run automatique.

## Décision d’audit (V5)
- Artefact qui fait foi : **`manifest_forensic_v5.json` signé**.
- `sha256_replit_v4.txt` reste utile comme trace opérationnelle, mais n’est plus l’autorité finale.

## Outils V5
- `tools/build_manifest_v5.py` : génère le manifest canonique.
- `tools/verify_manifest_v5.py` : vérifie tous les hash du manifest.
- `tools/sign_manifest_v5.sh` : signe le manifest (`openssl`) et peut vérifier la signature.
- `tools/generate_v5_post_run_report.py` : lit les résultats et produit `RAPPORT_REPLIT_EXECUTION_V5_FINAL.md`.
- `tools/stress_harness_v5.py` : stress CPU/RAM + télémétrie (objectif configurable, ex: 99%).

## Prompt Replit
- `PROMPT_REPLIT_AGENT_NQUBIT_V5.md` (nouveau prompt exécutable par l’agent Replit pour run complet V5).

## Tests V5
- `make test_unit`
- `make test_integration`
- `make test_stress`
- `make test_all`
- `make report_v5`

## Protocole recommandé
1. Geler les artefacts de run.
2. Générer le manifest V5.
3. Signer le manifest avec une clé privée d’audit.
4. Vérifier hash + signature en CI avant publication.
