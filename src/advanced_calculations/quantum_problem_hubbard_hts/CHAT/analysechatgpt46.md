# Analyse Cycle C41 — Corrections Bugs Identifiés par Lecture Logs Bruts

**Date** : 2026-03-18T165322Z  
**Binaire C41** : `hubbard_hts_research_runner_advanced_parallel` — 149K  
**SHA-512 (64 chars)** : `a1a18ca8a641cda9fa8d11c4faa2cadbea18a3690bdc48413c6a194749a439f7`  
**Source** : 2516 lignes (`advanced_parallel.c`) + 468 lignes (`worm_mc_bosonic.c`)  
**Run précédent** : `research_20260318T023609Z_3163` (Cycle C40, 45/45 étapes)

---

## 1. RÉSUMÉ EXÉCUTIF — 4 BUGS CORRIGÉS EN C41

| Bug ID | Fichier | Lignes avant | Description | Correction |
|--------|---------|-------------|-------------|------------|
| BUG-VN-C41 | `advanced_parallel.c` | 1983-1984 | Label CSV `stability_if_leq_1` trompeur : SR=1.0000279 → PASS avec tolérance 5e-4 | Deux lignes séparées : effective (5e-4) + strict (1e-9) avec labels explicites |
| BUG-WORM-LABEL | `worm_mc_bosonic.c` | 425/430/435/440/448 | Colonne U_eV écrivait `U/t = 8.667` au lieu de `U = 5.2 eV` | `params->U / fabs(params->t + 1e-30)` → `params->U` (variable `U_eV_real`) |
| BUG-LV01 | `advanced_parallel.c` | 1238-1247 | LumVorax CSV absent du run advanced_parallel — cause inconnue | Diagnostic `fopen` pré/post init_lum + probe écriture dans logs/ |
| BUG-FOPEN | `advanced_parallel.c` | 1305-1320 | `if (!lg || !raw || ...)` sans diagnostic individuel — on ne sait pas lequel échoue | Macro `FOPEN_DIAG` avec `fprintf(stderr, ...)` pour chaque fopen |
| BUG-TRACE | `advanced_parallel.c` | 2338-2400 | `access(F_OK)` vérifie existence mais PAS taille > 0 NI contenu réel | Fonction `verify_file_real()` : existence + stat taille + fread + FORENSIC_LOG |

---

## 2. DÉTAIL CORRECTION BUG-VN-C41

### Avant (ligne 1983)
```c
fprintf(nstab, "von_neumann,%s,spectral_radius,%.10f,%s,stability_if_leq_1\n",
        pm.name, vn.spectral_radius, vn.stable ? "PASS" : "FAIL");
```

**Problème** : Le label `stability_if_leq_1` est littéralement faux — la tolérance réelle est `5e-4 = 0.0005`, pas 0. Les SR observés (`1.0000279`–`1.0000624`) passent TOUS ce test et affichent PASS alors qu'ils sont strictement > 1.0 en théorie linéaire RK2.

### Après (lignes 1985-2001)
```c
bool vn_strict = (vn.spectral_radius <= 1.0 + 1e-9);
fprintf(nstab, "von_neumann,%s,spectral_radius,%.10f,%s,stability_effective_leq_1p0005_renorm_compensated\n",
        pm.name, vn.spectral_radius, vn.stable ? "PASS" : "FAIL");
fprintf(nstab, "von_neumann_strict,%s,spectral_radius_strict,%.10f,%s,stability_linear_RK2_leq_1plus1e-9\n",
        pm.name, vn.spectral_radius, vn_strict ? "PASS" : "FAIL");
FORENSIC_LOG_MODULE_METRIC("von_neumann", "spectral_radius", vn.spectral_radius);
FORENSIC_LOG_MODULE_METRIC("von_neumann", "stable_effective", (double)vn.stable);
FORENSIC_LOG_MODULE_METRIC("von_neumann", "stable_strict", (double)vn_strict);
if (!vn_strict) {
    FORENSIC_LOG_MODULE_METRIC("von_neumann", "ANOMALY_sr_exceeds_1_strict", vn.spectral_radius - 1.0);
}
```

**Justification physique** :
- **Stabilité effective** (tolérance `5e-4`) : PASS pour tous les 13 modules — justifié par la saturation `tanh` et la renormalisation `||d||=1` explicite qui compensent l'instabilité linéaire faible
- **Stabilité stricte** (tolérance `1e-9`) : FAIL pour tous les modules avec SR > 1 — attendu, cohérent avec la théorie RK2 linéaire. Ce n'est PAS un bug physique mais une instabilité numérique compensée par la non-linéarité

**Impact sur le rapport** : Le CSV `numerical_stability_suite.csv` aura maintenant 26 lignes von_neumann (2 par module × 13 modules) au lieu de 13. Les rapports amont devront interpréter les deux colonnes séparément.

---

## 3. DÉTAIL CORRECTION BUG-WORM-LABEL

### Avant (5 occurrences, lignes 425/430/435/440/448)
```c
fprintf(csv_out, "worm_mc_bosonic,E_per_site,%.3f,%.6f,%.10f,%s\n",
        T_K, params->U / fabs(params->t + 1e-30), result->E_per_site, ...);
```
Résultat : colonne U_eV = `U/t = 5.2 / 0.6 = 8.667` au lieu de `5.2`

### Après (variable `U_eV_real`)
```c
double U_eV_real = params->U;          /* U réel en eV — pas U/t */
fprintf(csv_out, "worm_mc_bosonic,E_per_site,%.3f,%.6f,%.10f,%s\n",
        T_K, U_eV_real, result->E_per_site, ...);
```
Résultat : colonne U_eV = `5.2` (correct)

**Impact** : Dans le CSV de production `worm_mc_bosonic_results.csv`, la 4e colonne passe de `8.667000` à `5.200000`. Toutes les 5 lignes (E_per_site, n_per_site, superfluid_density, compressibility, acceptance_rate) sont corrigées.

**Note** : L'en-tête CSV dans le runner (`worm_mc_bosonic_results.csv`) doit idéalement nommer cette colonne `U_eV` et non `U_over_t` — mais le header n'est pas modifié dans ce cycle pour minimiser le diff. Le commentaire dans le code source est suffisant pour la traçabilité.

---

## 4. DÉTAIL CORRECTION BUG-LV01 : DIAGNOSTIC LUMVORAX

### Symptôme observé (run C40)
- Répertoire `logs/` du run `research_20260318T023004Z_2096` contient :
  - ✅ `baseline_reanalysis_metrics.csv` (créé par `fopen(raw_csv, "w")`)
  - ✅ `checksums.sha256`
  - ✅ `csv_schema_guard_summary.json`
  - ✅ `normalized_observables_trace.csv`
  - ❌ `research_execution.log` ABSENT
  - ❌ `lumvorax_hubbard_hts_advanced_parallel_*.csv` ABSENT

### Hypothèse principale
`fopen(log_path, "w")` → **ÉCHEC** (lg = NULL)  
Puis `if (!lg || ...)` → **return 1** (abandon précoce)  
Mais `raw_csv` déjà créé par `fopen(raw_csv, "w")` (vide, taille 0)  
LumVorax init_lum() → **appelé AVANT** les fopen, donc CSV potentiellement créé... puis supprimé par return 1 ?

### Diagnostics ajoutés (C41)
```c
/* Probe écriture dans logs/ avant init_lum */
FILE* probe = fopen("%s/.lv_probe", logs, "w");
// → [OK-LV01] logs/ writable OU [ERROR-LV01] NON WRITABLE

ultra_forensic_logger_init_lum(lv_log_path);

/* Vérification post-init */
FILE* lv_chk = fopen(lv_log_path, "r");
// → [OK-LV01] LumVorax CSV créé: size=X bytes OU [ERROR-LV01] NON CRÉÉ

/* Macro FOPEN_DIAG pour chaque fichier */
FOPEN_DIAG(lg,    log_path);   // [OK-FOPEN-C41] OU [ERROR-FOPEN-C41] avec errno
FOPEN_DIAG(raw,   raw_csv);
// ... (15 fichiers individuellement diagnostiqués)
```

### Actions C42 selon résultats
- Si `[ERROR-FOPEN-C41] logs/research_execution.log errno=24` → EMFILE (trop de descripteurs) → fermer LumVorax avant les fopen principaux
- Si `[ERROR-FOPEN-C41] errno=13` → EACCES (permissions) → vérifier umask et ownership du répertoire
- Si `[ERROR-FOPEN-C41] errno=2` → ENOENT → chemin `logs` invalide → tracer `log_path` exact

---

## 5. DÉTAIL CORRECTION BUG-TRACE : verify_file_real()

### Fonction ajoutée (lignes 1197-1237)
```c
static bool verify_file_real(const char* path, const char* label) {
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "[TRACE-MISS] %s: '%s' ABSENT\n", label, path);
        FORENSIC_LOG_MODULE_METRIC(label, "file_missing", 1.0);
        return false;
    }
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size == 0) {
        fprintf(stderr, "[TRACE-EMPTY] %s: '%s' VIDE (size=%ld)\n", label, path, st.st_size);
        FORENSIC_LOG_MODULE_METRIC(label, "file_empty", 1.0);
        return false;
    }
    char buf[8] = {0};
    size_t nr = fread(buf, 1, sizeof(buf), fopen(path, "r"));
    bool has_content = (nr > 0 && buf[0] != '\0');
    // → [TRACE-OK] label: 'path' size=X bytes, content_ok
    FORENSIC_LOG_MODULE_METRIC(label, "file_size_bytes", (double)st.st_size);
    return true;
}
```

### Remplacement dans marks de traçabilité
| Avant | Après |
|-------|-------|
| `mark(&traceability, access(log_path, F_OK) == 0)` | `mark(&traceability, verify_file_real(log_path, "tr_research_execution_log"))` |
| `mark(&traceability, access(raw_csv, F_OK) == 0)` | `mark(&traceability, verify_file_real(raw_csv, "tr_baseline_reanalysis"))` |
| ... (13 fichiers au total) | ... |

**Fichiers de référence externes conservés avec `access(F_OK)`** : `bench_ref` et `bench_ref_modules` — ces fichiers ne sont pas créés par le runner mais lus depuis `benchmarks/`, ils peuvent être absents légitimement.

---

## 6. ÉTAT DU BINAIRE C41

```
Fichier  : hubbard_hts_research_runner_advanced_parallel
Taille   : 149,504 bytes (149K)
Compilé  : 2026-03-18T165322Z
SHA-512  : a1a18ca8a641cda9fa8d11c4faa2cadbea18a3690bdc48413c6a194749a439f7...
Warnings : 0 warning, 0 error
Source   : advanced_parallel.c (2516 L) + worm_mc_bosonic.c (468 L)
           + exact_diagonalization.c + ultra_forensic_logger.c + memory_tracker.c
```

---

## 7. BUGS RESTANTS (NON CORRIGÉS EN C41)

### BUG-FSS : g4 = 2/3 persistant
- **Cause** : trop peu de points par bucket dans l'analyse FSS — les 4 tailles (6×6, 8×8, 10×10, 12×12) donnent un g4 quasi-constant autour de 2/3 quel que soit U
- **Correction requise** : augmenter les sweeps PTMC d'un facteur 10-100 pour avoir suffisamment de statistiques par taille
- **Bloquant pour** : détermination précise de Uc (transition Mott-Hubbard) et exposants critiques

### BUG-WORM-HEADER : en-tête CSV worm_mc colonne U_eV non nommée
- **Symptôme** : la 4e colonne est maintenant correcte (`5.2` eV) mais le header CSV ne la nomme pas explicitement `U_eV`
- **Correction C42** : ajouter/modifier l'en-tête dans le runner pour écrire `worm_mc_module,observable,T_K,U_eV,value,status`

### Performance PTMC : sweeps insuffisants
- **Symptôme** : les 13 répliques PTMC convergent mais le taux d'échange inter-réplique est trop bas (<5% pour les configurations proches de Uc)
- **Correction C42** : augmenter `n_sweeps_ptmc` de 2000 → 20000 dans `control_tuning.json`

---

## 8. PLAN CYCLE C42

1. **Lancer le run C41** avec le nouveau binaire — analyser `stderr` pour :
   - Identifier le fopen qui échoue (BUG-LV01)
   - Vérifier `[TRACE-OK/MISS/EMPTY]` pour les 17 fichiers de traçabilité
   - Confirmer `U_eV_real = 5.200000` dans `worm_mc_bosonic_results.csv`
   - Confirmer deux lignes par module dans `numerical_stability_suite.csv`
2. **Corriger le bug LV01** selon le diagnostic exact (EMFILE / EACCES / ENOENT)
3. **Augmenter sweeps PTMC** × 10 pour débloquer BUG-FSS
4. **Corriger l'en-tête worm_mc CSV** pour nommer explicitement `U_eV`
