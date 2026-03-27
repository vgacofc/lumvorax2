# Plan expert — Intégrer TIFF concurrent dans apprentissage via LUMV1 + validation dépendances 360

Date: 2026-02-20

## 1) Analyse de ta proposition (verdict)
Ta proposition est **techniquement pertinente** à condition d’être implémentée dans un cadre strict anti-fuite et anti-régression:
1. apprentissage normal interne,
2. phase d’alignement contrôlé vs TIFF concurrent (teacher/reference),
3. retour au pipeline test normal pour produire la soumission finale.

Ce schéma peut améliorer calibration densité/seuils, mais doit éviter de sur-ajuster sur un seul TIFF de référence.

---

## 2) Architecture recommandée (sans smoke/hardcoding)

## Phase A — Ingestion référence concurrente
- Charger TIFF concurrent (3D) depuis source tracée.
- Convertir immédiatement en LUMV1 (`convert_competitor_tiff_to_lum.py`).
- Émettre logs complets:
  - taille/shape avant-après,
  - hash payload,
  - statistiques globales + par tranche z.

## Phase B — Apprentissage standard
- Exécuter train habituel (aucune dépendance concurrente dans cette étape).
- Sauvegarder checkpoints + forensic baseline.

## Phase C — Ajustement assisté référence
- Charger volume concurrent (format `.lum`),
- comparer nos prédictions à la référence:
  - IoU 3D,
  - F-beta proxy,
  - densité globale/par tranche,
  - distance distributions (histogram KL / Wasserstein simple).
- Ajuster paramètres autorisés:
  - seuils multi-niveaux,
  - lissage z,
  - poids fusion.
- Interdire toute modification non traçable hors liste blanche des paramètres.

## Phase D — Production finale standard
- Désactiver la référence concurrente.
- Relancer test normal et générer TIFF final.
- Fournir preuve forensic: run final sans teacher input actif.

---

## 3) Ce qui a été ajouté immédiatement dans le repo

1. **Nouvelles versions runtime**: V2.5, V7.7, V61.5, V144.3 (forensic360 renforcé).
2. **Outil conversion TIFF->LUMV1**:
   - `RAPPORT-VESUVIUS/validation_lumvorax/convert_competitor_tiff_to_lum.py`
3. **Inventaire modules/fonctions C/C++ LUMVORAX**:
   - `RAPPORT-VESUVIUS/validation_lumvorax/build_lumvorax_module_inventory.py`
   - `RAPPORT-VESUVIUS/validation_lumvorax/lumvorax_module_inventory_20260220.json`
   - `RAPPORT-VESUVIUS/validation_lumvorax/RAPPORT_INVENTAIRE_MODULES_FONCTIONS_LUMVORAX_20260220.md`
4. **Notebook de validation 360 dépendances**:
   - `RAPPORT-VESUVIUS/validation_lumvorax/notebook_validation_lumvorax_dependances_360_20260220.ipynb`
5. **Tests unitaires 360 supplémentaires**:
   - `RAPPORT-VESUVIUS/tests/test_lumvorax_dependency_stack_360.py`

---

## 4) Modules/fonctions et rôles (synthèse claire)
Le détail exhaustif est dans le rapport inventaire auto-généré. Rôle des familles:
1. `src/lum/*`: cœur objets LUM (sécurité, allocation, intégrité).
2. `src/file_formats/*`: format `.lum` universel, header/sections/mappings, gestion fichiers.
3. `src/vorax/*`: opérations VORAX et traitement 3D volumique.
4. `src/logger/*` + `src/debug/*`: forensic/logging/memory tracking/alerting.

---

## 5) Notebook demandé: périmètre et objectifs
Le notebook produit valide:
1. inventaire modules/fonctions,
2. smoke compile C des modules clés,
3. conversion TIFF->LUMV1 + logs par tranche,
4. visibilité des tests existants Python.

À exécuter dans Kaggle/Replit avec les dépendances installées.

---

## 6) Questions d’expert à trancher (importantes)
1. La référence concurrente est-elle utilisée en production, ou uniquement en calibration offline?
2. Quelles métriques pilotent l’auto-ajustement (ordre de priorité)?
3. Quels paramètres sont modifiables automatiquement (liste blanche exacte)?
4. Quelle limite pour empêcher sur-apprentissage à la référence concurrente?
5. Quelle politique sécurité/licence sur l’usage de cette référence?
6. Quel protocole rollback si score baisse malgré ajustement assisté?
7. Quel seuil OOM force un mode compact (patch/batch downscale)?
8. Quel manifest minimal obligatoire dans chaque run pour attester authenticité?

---

## 7) Problèmes/limites rencontrés (notification)
1. Validation Kaggle réelle (score/LB) impossible localement sans exécution distante.
2. Activation native C/C++ complète dépend présence `.so` runtime.
3. Inventaire fonctions est fiable pour prototypes `.h`, mais la sémantique fine de chaque fonction nécessite revue métier ligne par ligne des implémentations complètes.

---

## 8) Plan exécution immédiate recommandé
1. Exécuter notebook 360 validation dépendances dans environnement cible.
2. Générer un `.lum` référence concurrente versionné avec logs tranche par tranche.
3. Intégrer phase C (ajustement assisté) dans V2.5 et V144.3 derrière flag explicite.
4. Lancer campagne A/B:
   - A = baseline sans référence,
   - B = ajustement assisté actif,
   - comparer score réel + robustesse mémoire.
5. Conserver uniquement configuration gagnante sans régression forensic/conformité.

