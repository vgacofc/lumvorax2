# PLAN — Confirmation runtime Kaggle multi-versions après restauration dataset V3

Date: 2026-02-19  
Statut: **PLAN ACTIF — EN ATTENTE D'EXÉCUTION**

---

## 1) Objectif

Valider que la restauration V3 du dataset `ndarray2000/nx47-dependencies` fonctionne réellement sur les kernels historiques et récents déjà travaillés/scorés, sans régression.

---

## 2) Matrice de validation

| Version kernel | Cible | Priorité | Critère principal |
|---|---|---:|---|
| `nx47_vesu_kernel_v2.py` | Compatibilité héritée | Haute | bootstrap offline + sortie artefacts |
| `nx47_vesu_kernel_v2.1.py` | Régression post-indentation | Haute | run complet sans crash |
| `nx47_vesu_kernel_v2.2.py` | imagecodecs optionnel | Haute | pas de fail OFFLINE_DEPENDENCY_MISSING |
| `nx47_vesu_kernel_v2.3.py` | discovery dataset robuste | Haute | détection fragments sur layout compétition |
| `nx47_vesu_kernel_v2.4.py` | fallback LZW/Pillow | Critique | pas de `ValueError: requires imagecodecs` |

---

## 3) Procédure standard par version

1. Configurer dataset input + output Kaggle.
2. Exécuter le kernel cible sans modifier le code.
3. Collecter:
   - logs console,
   - `submission.parquet`,
   - `submission.zip`,
   - metadata roadmap/execution.
4. Marquer statut PASS/FAIL.
5. Archiver les preuves dans un rapport dédié d'exécution.

---

## 4) Gates GO / NO-GO

### GO si tous les points sont vrais
- Le kernel termine sans exception bloquante.
- Les artefacts de soumission sont produits.
- Aucune erreur LZW non gérée.
- Aucun blocage sur dépendance restaurée (`imagecodecs` / `tifffile-2026.1.28`).

### NO-GO si un seul point échoue
- `OFFLINE_DEPENDENCY_MISSING` sur package supposé présent.
- `ValueError: <COMPRESSION.LZW: 5> requires the 'imagecodecs' package` non récupéré.
- pas de `submission.parquet` ou `submission.zip`.

---

## 5) Résultat attendu

Si la matrice est verte:
- créer un nouveau rapport final de clôture: `VALIDATION_RUNTIME_V3_CONFIRMED_<DATE>.md`.

Si au moins un échec:
- créer un correctif versionné (nouveau fichier kernel),
- conserver l'historique append-only,
- rejouer uniquement les versions impactées.
