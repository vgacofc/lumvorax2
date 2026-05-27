# AUDIT CIBLÉ — Dépendances LUMVORAX, pré-exécutions, densités, et cas `Version 18 OOM`

Date: 2026-02-20  
Contexte: réponse corrective suite à insatisfaction sur le rapport précédent.

---

## 1) Correction du point clé demandé (V2.4 + OOM)

Tu as confirmé que le fichier réellement utilisé est:
- `RAPPORT-VESUVIUS/src_vesuvius/nx47_vesu_kernel_v2/nx47_vesu_kernel_v2.4.py`

Constat aligné avec les artefacts locaux:
- la pré-exécution V2.4 / V140 termine (`Execution Complete`) autour de ~19960s,
- mais la soumission Kaggle référencée ensuite est `nx47_vesu_kernel_v2 - Version 18` avec `Notebook Out of Memory`.

Conclusion d'expert:
- Oui, il est cohérent d'avoir **pré-exécution OK** puis **soumission OOM** si les ressources runtime changent (RAM dispo, overhead notebook, duplication buffers, pipeline submission plus lourd).

---

## 2) Est-ce que les pré-exécutions utilisent réellement les dépendances LUMVORAX ?

## 2.1 Réponse courte
- **V2.4**: OUI (fort).
- **V7.6**: OUI (fort pour dépendances wheels, pas de bridge natif `.so` explicite).
- **V144.2**: OUI (fort pour wheels offline; pas de preuve d'appel direct `liblumvorax_replit.so`).
- **V61.4**: OUI partiel (utilise `imagecodecs`/`tifffile`, mais via `/kaggle/input/nx47-dependencies` au lieu du chemin dataset V3 principal).

## 2.2 Preuves techniques par version

### V2.4 (`nx47_vesu_kernel_v2.4.py`)
- Bootstrap offline fail-fast avec installation `numpy/pandas/tifffile/pyarrow/imagecodecs`.
- Répertoires recherchés: dataset V3 principal + fallbacks.
- Bridge natif LUM/VORAX via `ctypes` avec recherche `liblumvorax_replit.so`.
- Logs runtime montrent installation wheel `imagecodecs` depuis `ndarray2000/nx47-dependencies`.

### V7.6
- Fonction `install_offline` + `bootstrap_dependencies_fail_fast()` installent `numpy/imagecodecs/tifffile` depuis dataset + fallbacks.
- Logs runtime montrent installation `imagecodecs` et `tifffile` depuis `ndarray2000/nx47-dependencies`.

### V144.2
- Bootstrap offline explicite `numpy/imagecodecs/tifffile`.
- Recherche prioritaire dans `/kaggle/input/datasets/ndarray2000/nx47-dependencies`.
- Logs runtime montrent installation effective `imagecodecs` depuis ce dataset.

### V61.4
- Installation offline au démarrage, mais chemin codé: `--find-links=/kaggle/input/nx47-dependencies`.
- Logs montrent `imagecodecs` installé depuis ce fallback; `tifffile` déjà présent runtime.
- Donc: dépendances utilisées, mais **pas alignement strict** sur le chemin dataset V3 principal.

---

## 3) Processus + schéma détaillé d'usage des dépendances (phases)

## 3.1 Schéma générique observé
1. **BOOTSTRAP_DEPENDENCIES**
   - Installer offline les wheels critiques (`numpy`, `tifffile`, `imagecodecs`, etc.).
2. **RELOAD_MODULES TIFF**
   - Rechargement de `tifffile` après installation `imagecodecs` pour activer LZW.
3. **I/O TIFF sécurisé**
   - Lecture TIFF LZW via `tifffile`; fallback Pillow si nécessaire.
4. **PROCESSING**
   - Préprocessing/features/segmentation.
5. **WRITE SUBMISSION**
   - Écriture TIFF (souvent LZW), validation format binaire `0/1` ou `0/255` selon mode.
6. **FORENSIC/AUDIT**
   - Logs roadmap, stats, intégrité, preuves de run.

## 3.2 Où LUMVORAX bénéficie concrètement par version
- **V2.4**:
  - bootstrap strict,
  - roundtrip `.lum` (encode/decode/checksum),
  - bridge natif optionnel `.so`,
  - événements `LUM_ROUNDTRIP_OK` par fragment.
- **V7.6**:
  - bénéfice principal côté compatibilité TIFF LZW (imagecodecs+tifffile) et robustesse packaging submission.
- **V144.2**:
  - bénéfice principal côté robustesse TIFF offline + forensic avancé.
- **V61.4**:
  - bénéfice principal minimal: installation imagecodecs/tifffile pour I/O TIFF.

---

## 4) Densité (%) des images de soumission générées

Calculs réalisés en lisant les TIFF depuis `submission.zip` (ou structure équivalente):
- **V61.4**: `12.256454467773438 %`
- **V7.6**: `2.341796875 %`
- **V144.2**: `6.0 %`

Cas V2.4:
- `submission.parquet` contient uniquement colonnes `id,target` (pas de colonne densité explicite).
- Approximation disponible via `target` moyen:
  - **V2.4 target moyen ≈ `10.30666335635102 %`**.
- Note méthodologique: c'est une approximation proxy de densité/score agrégé, pas une mesure voxel binaire directe de tous les TIFF du dossier `submission/`.

---

## 5) "L'agent précédent a-t-il bien exécuté le prompt V3 dataset dépendances ?"

Réponse factuelle: **partiellement, pas validé à 100%**.

Pourquoi:
- Le prompt cible `PROMPT_AGENT_DATASET_DEPENDENCIES_V3.md` exige GO/NO-GO avec présence complète artefacts + validation runtime.
- Le fichier d'état `validation_results_dataset_v3_pending_20260219.json` indique explicitement:
  - `status: pending_runtime_confirmation`,
  - validation inventaire déclarée OK,
  - mais confirmation runtime encore en attente.

Conclusion:
- Travail avancé, mais **pas clôturé en GO définitif**.

---

## 6) Code "sans placeholder/smoking/hardcoding/stub" ?

Réponse d'expert nuancée:
- **V2.4**: je ne vois pas de fallback synthétique actif; le code force données TIFF réelles + fail-fast + roundtrip `.lum`.
- **V144.2**: présence d'un module `run_simulation_100()` basé RNG (usage de simulation), et policy hardcoded metric configurable `warn|error`.
- **V7.6/V61.4**: orientés exécution réelle; pas de stub explicite détecté dans le flux principal.

Conclusion stricte:
- Je ne peux **pas** certifier "zéro risque absolu" de toute forme de smoke/hardcoding sur l'ensemble de tous chemins de code sans campagne de tests exhaustifs multi-branches.
- Je peux certifier que les chemins principaux inspectés sont majoritairement réels et orientés data TIFF réelle.

---

## 7) "Le système dataset LUM VORAX est-il 100% opérationnel pour toutes nouvelles versions ?"

Réponse: **NON prouvé à 100%** à ce stade.

Motifs bloquants:
1. Rapport de validation dataset indique `pending_runtime_confirmation`.
2. Validation locale LUM/VORAX dit explicitement que le moteur C 3D natif n'est pas confirmé à 100% tant que disponibilité/compilation runtime n'est pas totalement garantie.
3. Les versions n'emploient pas toutes le bridge natif `.so` de la même manière.

Donc:
- opérationnel **partiel/fort** sur plusieurs runs,
- mais pas démonstration "universelle 100%" sans matrice de validation supplémentaire.

---

## 8) "Le format universel LUM local traite-t-il réellement tous types TIFF/3D/imagecodecs/autres ?"

Réponse: **partiellement validé**.

Validé:
- Roundtrip `.lum` unitaire OK,
- lecture/écriture TIFF 2D/3D et LZW couverte dans les pipelines inspectés,
- fonctionnement local smoke validé.

Non encore prouvé exhaustivement:
- "tous types de fichiers" au sens universel absolu,
- tous cas limites de compression exotiques,
- toutes profondeurs/bits/endian/metadata atypiques,
- toutes combinaisons runtime Kaggle (RAM/GPU/versions libs).

---

## 9) Questions d'expert additionnelles (importantes, à poser)

1. Quelle est la **version canonique de prod** (V2.4, V7.6, V144.2, V61.4) selon objectif score/robustesse ?
2. Veut-on imposer un **contrat unique bootstrap dépendances** (mêmes paths, mêmes wheels, même policy fail-fast) pour 100% des versions ?
3. Veut-on imposer `hardcoded_metric_policy=error` partout (et non `warn`) ?
4. Veut-on interdire en prod toute fonction de simulation RNG (même si non appelée) via linter/policy CI ?
5. Veut-on un **OOM guard standardisé**:
   - limite RAM par étape,
   - flush/GC obligatoire,
   - réduction automatique batch/patch,
   - checkpoint/reprise ?
6. Veut-on une validation automatique de densité mask par version (seuil min/max attendus) avant envoi Kaggle ?
7. Veut-on une preuve obligatoire d'usage bridge natif (`liblumvorax_replit.so`) dans les logs de chaque run prod ?
8. Faut-il bannir définitivement les versions qui n'utilisent que fallback `/kaggle/input/nx47-dependencies` ?
9. Faut-il une matrice de non-régression multi-formats TIFF (LZW, deflate, planar configs) avec artefacts de preuve ?
10. Souhaite-t-on une qualification "GO PROD" formelle signée seulement après exécution complète du plan runtime V3 ?

---

## 10) Décision opérationnelle proposée

- **Décision actuelle**: `GO partiel` (pas GO absolu).
- Conditions minimales pour `GO complet`:
  1. fermer le `pending_runtime_confirmation` du dataset V3,
  2. unifier bootstrap dépendances sur toutes versions,
  3. activer guard anti-OOM standard,
  4. prouver usage natif `.so` sur run de référence,
  5. exécuter matrice tests TIFF/3D et publier résultats signés.


---

## Addendum 2026-02-20 — réponse à la demande "ancien Python vs nouveau LUMVORAX"

Verdict consolidé:
- Les systèmes forensic 360° ne sont pas tous au même niveau d’intégration native.
- L’usage des nouvelles dépendances dataset est confirmé sur V2.4/V7.6/V144.2 (et fallback sur V61.4).
- Le moteur forensic est principalement Python instrumenté selon la version; V2.4 est le plus avancé vers LUMVORAX hybride.

Actions décidées:
1. Standardiser un runtime forensic unique cross-versions.
2. Forcer un manifest dépendances + hash.
3. Ajouter preuve explicite d’activation bridge natif `.so`.
4. Déployer plan de convergence versions V61.5/V144.3/V7.7/V7.6.
