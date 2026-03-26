# CAHIER_DES_CHARGES_ROADMAP_V4_NEXT_CYCLE17_AZ

## Objectif
Intégrer de A à Z les suggestions restantes (scientifiques + techniques) avec logs/tests/gates simultanés, sans régression et avec traçabilité complète.

## A. Architecture et données
1. Normaliser systématiquement les observables (`energy_per_site`, `pairing_normalized`).
2. Ajouter schéma de métadonnées obligatoire: lattice size, géométrie, bords, U/t/μ/T, seed, solveur, dt.
3. Versionner le format CSV (champ `schema_version`).
4. Ajouter identifiant unique de run et chaînage parent→enfant.

## B. Build, exécution, reproductibilité
1. Figer versions (Python/Rust/deps) et stocker `environment_versions.log`.
2. Générer checksum final et manifeste des artefacts à chaque run.
3. Ajouter mode replay d’un run à seed fixe.

## C. Contrôles numériques (obligatoires)
1. Test stabilité vs pas de temps (`dt/2`, `dt`, `2dt`).
2. Test conservation/invariants selon module.
3. Test sensibilité conditions initiales.
4. Test accumulation erreur flottante long-run.

## D. Observables physiques minimales par domaine
1. Hubbard: corrélations longue distance, susceptibilité, DOS, Green function.
2. QCD: Wilson loops, potentiel quark-antiquark, test aire law.
3. QFT hors équilibre: spectre en k, charge/énergie conservée.
4. Chimie quantique: convergence vers références HF/FCI pour cas test.
5. Nucléaire dense: observables d’équation d’état et stabilité.

## E. Analyse statistique
1. Barres d’erreur et IC95 pour chaque claim.
2. Test d’hypothèse explicite par résultat (H0/H1).
3. Data collapse et extraction d’exposants critiques quand applicable.

## F. Gating CI/CD (bloquant)
1. `CSV_INTEGRITY_GATE` (déjà présent) => bloquant.
2. `MODULE_COVERAGE_GATE` (déjà présent) => bloquant.
3. `METADATA_COMPLETENESS_GATE` => nouveau bloquant.
4. `PHYSICS_OBSERVABLES_GATE` => nouveau bloquant.
5. `UNCERTAINTY_REPORTING_GATE` => nouveau bloquant.

## G. Logs et artefacts à générer simultanément
1. Rapport pédagogique run (non-expert + glossaire + limites).
2. Rapport scientifique (méthode, hypothèses, preuves, limites).
3. Registre problème→solution→statut.
4. Drift monitor (infra vs observables).
5. Manifest/checksums/provenance.

## H. Schéma de tests obligatoire
- Smoke: exécution complète + artefacts.
- Numérique: stabilité dt, invariants, convergence.
- Physique: observables domaine.
- Régression: comparaison run N vs N-1.
- Performance: CPU/RAM/time budget.

## I. Gouvernance des claims
1. Tag claim: `certain`, `probable`, `hypothèse`.
2. Interdire claims physiques forts si gates D/E/F non validés.
3. Ajouter section « ce qui n’est pas démontré » obligatoire dans chaque rapport.

## J. Roadmap de déploiement
- Sprint 1: métadonnées + gates bloquants + normalisation.
- Sprint 2: observables Hubbard/QCD/QFT/chemistry minimaux.
- Sprint 3: statistiques (IC95, tests hypothèse, scaling).
- Sprint 4: industrialisation CI + dashboards + audit final.

## K. Critères d’acceptation finaux
1. 0 erreur CSV, 0 champ critique manquant.
2. Reproductibilité inter-run des observables de base.
3. Présence des observables physiques minimales par domaine.
4. Rapports auto-générés (pédagogique + scientifique).
5. Checksums/provenance complets et vérifiés.
