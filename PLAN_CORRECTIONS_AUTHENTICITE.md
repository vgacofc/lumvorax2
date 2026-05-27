# Plan complet de corrections d’authenticité (Lumvorax / NX)

> **Objectif** : éliminer les faux positifs d’authenticité (placeholders, mocks, fallback “dummy”), rendre les résultats reproductibles avec données réelles, et fournir une stratégie de tests unitaires “360°” + intégration, sans modifier les anciens fichiers `.md`.

---

## 0) Périmètre et méthode

- **Périmètre** : kernels NX‑47 (ARC/RNA/FINA/NFL/IAMO3/VESU), sous-système cryptographique/forensic (SHA‑256 “mock”), SIMD/optimisations, logging, exécution tests authentiques.
- **Méthode** : remplacement des chemins de fallback par des chemins explicites (erreur/fail‑fast), implémentations réelles pour les parties “mock”, et garde‑fous de configuration.
- **Règle** : *si un environnement ne fournit pas les données réelles, l’exécution doit échouer proprement (pas de faux résultats).*

---

## 1) Corrections par module (avec domaines d’expertise requis)

### 1.1 Kernels AIMO3/NX‑47 (ARC, RNA, FINA, NFL, IAMO3)
**Domaines** : MLOps/Kaggle, ingénierie données, robustesse d’exécution, audit/forensic.

**Avant** (extraits) :
- Déclarations “aucun stub/placeholder” mais fallback “Using mock data” si le dataset manque.【F:nx47-arc-kernel.py†L1-L17】【F:nx47-arc-kernel.py†L1539-L1556】
- Fallback identique dans les modules `modules/*` (RNA/FINA/NFL/IAMO3).【F:modules/rna/nx47-rna.py†L1541-L1556】【F:modules/fina/nx47-fina.py†L1541-L1556】【F:modules/nfl/nx-47-nfl.py†L1541-L1556】【F:modules/iamo3/nx-47-iamo3.py†L1541-L1556】

**Après (plan)** :
1) **Fail‑fast** si `test.csv` n’existe pas : lever une erreur bloquante et journaliser un statut “AUTHENTICITY_BLOCKED”.
2) **Config stricte** : ajouter un flag d’environnement `NX_REQUIRE_REAL_DATA=1` (par défaut activé) interdisant tout fallback.
3) **Audit** : log structuré de la provenance des données (chemin, checksum SHA‑256), enregistré dans un artefact unique.

**Tests unitaires 360° (exemples)** :
- `test_requires_real_data_when_flag_set()` : vérifie que l’absence du dataset provoque l’échec.
- `test_dataset_checksum_logged()` : vérifie le hash d’entrée.
- `test_no_mock_data_path()` : vérifie qu’aucun chemin “mock” n’est pris.

---

### 1.2 VESU Kernel (nx47_vesu_kernel.py / nx47_vesu_kernel_v2.py)
**Domaines** : vision/IA, pipeline Kaggle, qualité des données, MLOps.

**Avant** :
- Création d’un dataset local “dummy” si Kaggle absent (images factices).【F:nx47_vesu_kernel.py†L132-L141】
- Génération de 100 entrées `dummy_*` si aucun fragment n’est trouvé.【F:nx47_vesu_kernel_v2.py†L50-L59】

**Après (plan)** :
1) **Retirer la génération de données factices** : remplacer par `raise FileNotFoundError` + log “NO_REAL_DATA”.
2) **Validation stricte** : vérifier structure et intégrité (ex. nombre de tuiles, checksums).
3) **Séparer mode demo** : un mode explicitement “demo” qui **ne peut pas** être confondu avec un mode réel (noms de fichiers, outputs séparés).

**Tests unitaires 360°** :
- `test_vesu_fails_without_real_tiles()`.
- `test_vesu_rejects_empty_fragment_list()`.
- `test_demo_mode_marked_non_authentic()`.

---

### 1.3 Forensic/Hashing (NX‑25, NX‑11 strict/canonical)
**Domaines** : cryptographie appliquée, intégrité forensique, systèmes temps réel.

**Avant** :
- Usage de `sha256_mock` dans NX‑25 et NX‑11 (hash non authentique).【F:src/nx_versions/nx25_heritage_engine.cpp†L15-L22】【F:src/sch/nx/sch_nx_v11_strict.c†L30-L43】【F:src/sch/nx/sch_nx_v11_canonical_final.c†L29-L36】
- Index `INITIAL_STUB_SHA256` dans NX‑11 strict.【F:src/sch/nx/sch_nx_v11_strict.c†L111-L118】

**Après (plan)** :
1) **Remplacer `sha256_mock`** par une implémentation SHA‑256 réelle (lib ou implémentation locale auditée).
2) **Tag d’intégrité** : forcer un champ `HASH_IMPL=SHA256_REAL` dans logs.
3) **Migration de compatibilité** : fournir un convertisseur pour les anciens logs (marqués “legacy”).

**Tests unitaires 360°** :
- `test_sha256_known_vectors()` (vecteurs officiels).
- `test_log_hash_chain_integrity()`.
- `test_legacy_log_flagging()`.

---

### 1.4 SIMD Optimizer
**Domaines** : C bas niveau, SIMD, performance/HPC.

**Avant** :
- `matrix_multiply_lum_optimized` retourne `NULL` comme placeholder.【F:src/optimization/simd_optimizer.c†L432-L456】

**Après (plan)** :
1) **Implémenter un chemin réel** (scalar fallback correct + SIMD optionnel).
2) **Validation dimensionnelle stricte** (assertions, erreurs formelles).
3) **Benchmarks reproductibles** (fixer graine, tailles).

**Tests unitaires 360°** :
- `test_simd_matrix_multiply_matches_scalar()`.
- `test_matrix_multiply_handles_invalid_dims()`.

---

### 1.5 Logger / Analyse logs
**Domaines** : observabilité, analyse forensique, C/IO.

**Avant** :
- `lum_log_analyze` est un placeholder (valeurs fixes).【F:src/logger/lum_logger.c†L423-L433】

**Après (plan)** :
1) **Parse réel** des logs et statistiques complètes (opérations, erreurs, distribution).
2) **Traçabilité** : versionner le format de log.

**Tests unitaires 360°** :
- `test_log_parser_counts_operations()`.
- `test_log_parser_handles_corrupt_lines()`.

---

### 1.6 Core LUM (stubs commentés)
**Domaines** : core systems, mémoire, logging C.

**Avant** :
- Notes de placeholders pour `memory_tracker_cleanup` et `lum_log`.【F:src/lum/lum_core.c†L1105-L1124】

**Après (plan)** :
1) **Définir API formelle** pour `memory_tracker_cleanup`.
2) **Unifier `lum_log`** avec le logger du système pour garantir cohérence des métriques.

**Tests unitaires 360°** :
- `test_memory_tracker_cleanup_frees_all()`.
- `test_lum_log_integration_with_logger()`.

---

### 1.7 RSA Structure Analyzer
**Domaines** : crypto/maths, analyse statistique.

**Avant** :
- Observables simulées (“mocked”, “Placeholder”).【F:src/crypto/rsa_structure_analyzer.c†L13-L28】

**Après (plan)** :
1) **Remplacer les métriques simulées** par une mesure réelle (résidus, coprimalité sur fenêtre paramétrée).
2) **Validation scientifique** : comparer sur jeux de tests connus.

**Tests unitaires 360°** :
- `test_rsa_observable_matches_reference_distribution()`.

---

## 2) Plan d’intégration “de A à Z”

1) **Gouvernance** : définir le standard d’authenticité (`AUTHENTICITY_SPEC.md` nouveau, séparé).
2) **Phase 1 – Fail‑fast & audit des entrées** : suppression des fallbacks mock/dummy.
3) **Phase 2 – Crypto/Forensic réel** : remplacer les fonctions hash simulées.
4) **Phase 3 – Optimisations réelles** : SIMD + logging complet.
5) **Phase 4 – Tests unitaires 360°** : couverture par domaine + tests d’intégration.
6) **Phase 5 – Validation CI** : exécution automatique dans pipeline.

---

## 3) Plan de tests 360° (extrait structuré)

- **Données/IO** : vérif data réelle, checksums, format, taille.
- **Crypto/Forensic** : vecteurs SHA‑256, chaîne de hash, cohérence logs.
- **Performance** : SIMD vs scalar, marges d’erreur.
- **Robustesse** : erreurs input, corruption, valeurs extrêmes.
- **Non‑régression** : golden outputs contrôlés.

---

## 4) Résultat attendu

- Toute exécution doit être **authentique ou échouer explicitement**.
- Les logs doivent attester **la provenance et l’intégrité**.
- Les résultats ne doivent pas être confondus avec des démos ou mocks.

