# COURS ULTRA PÉDAGOGIQUE — NX46 VX
Date: 2026-02-24
Audience: pilotage compétition + compréhension technique complète

---

## 0) RÈGLE OPÉRATIONNELLE NON NÉGOCIABLE (AJOUT DEMANDÉ)

## Politique d’exécution/soumission
**Tant que le résultat de pré-exécution NX46 VX n’est pas égal ou supérieur au TIFF de référence concurrent, aucune soumission Kaggle n’est autorisée.**

### Gate formel
- Entrée 1: score/proxy de pré-exécution NX46 VX
- Entrée 2: score/proxy du TIFF concurrent de référence
- Condition GO soumission: `preexec_nx46_vx >= ref_concurrent_tiff`
- Sinon: statut `NO-GO_SUBMISSION_LOCKED`

### Pourquoi
- Empêcher la consommation inutile du budget (9 soumissions restantes).
- Forcer un cycle d’amélioration interne mesuré avant tout upload.
- Réduire le risque stratégique J-3.

---

## 1) Ce que contient exactement `nx46-vx-unified-kaggle.ipynb`

Le notebook `RAPPORT-VESUVIUS/NX46-VX/nx46-vx-unified-kaggle.ipynb` est une **fusion séquentielle** de 4 notebooks sources:
1. V61.5
2. V144.2
3. V7.7
4. V7.6

Il contient 10 cellules:
- 4 cellules markdown de séparation/provenance,
- 4 gros blocs code (un par version),
- 1 cellule code de manifeste d’intégration (`function_manifest`, `collision_manifest`).

### Ce qu’il “produit” concrètement
Selon le bloc exécuté/activé:
- découverte données train/test,
- calibration de seuils,
- inférence volumique par tranches,
- génération de masques binaires,
- écriture TIFF multipage compressé,
- packaging `submission.zip`,
- logs forensic et métriques de contrôle.

**Important:** le notebook fusionné est une superposition d’héritages. Il contient des fonctions qui se recouvrent par nom; en Python, la **dernière définition active** prévaut quand mêmes noms.

---

## 2) Schéma exact de fonctionnement (processus par processus)

## PROCESSUS P0 — Bootstrap dépendances
### Objectif
Garantir que les librairies critiques existent avant calcul (ex: `tifffile`, `imagecodecs`).

### Étapes
1. Vérifier environnement Kaggle/offline wheels.
2. Installer offline si possible.
3. Fallback installation standard si autorisé.
4. Écrire un manifest de dépendances.

### Couches internes
- Couche système: pip + chemins datasets dépendances.
- Couche robustesse: try/fallback.
- Couche traçabilité: logs + manifest JSON.

---

## PROCESSUS P1 — Contrat de données (discovery)
### Objectif
Trouver exactement les chemins train/test valides.

### Étapes
1. Explorer candidats de racine.
2. Détecter layout (`test_images`, labels, sample submission).
3. Vérifier présence fragments exploitables.
4. Échouer vite si layout invalide.

### Couches
- Couche structure dossier.
- Couche inventaire items.
- Couche validation (anti “run vide”).

---

## PROCESSUS P2 — Lecture volumique TIFF / normalisation
### Objectif
Convertir données brutes en tenseurs exploitables.

### Étapes
1. Lire stack TIFF par fragment.
2. Normaliser intensité.
3. Appliquer lissage / filtrage z.
4. Préparer tableaux pour extraction de signal.

### Couches
- I/O TIFF sécurisé,
- prétraitement volumique,
- harmonisation dtype/range.

---

## PROCESSUS P3 — Extraction signal (features)
### Objectif
Transformer voxel/pixel en indices discriminants.

### Étapes
1. Features locales (gradient, contraste, laplacien, etc.).
2. Features contexte (fenêtres 2.5D / z-neighbors).
3. Sélection adaptative des features utiles.

### Couches
- signal brut,
- signal dérivé,
- sélection/pondération.

---

## PROCESSUS P4 — “Neurone principal” (apprentissage/décision)
### Objectif
Produire une carte de probabilité encre/non-encre.

### Étapes
1. Entraînement supervisé/pseudo-supervisé (selon version).
2. Ajustement régularisation et ratio cibles.
3. Prédiction probabiliste (`predict_proba`/équivalent).

### Couches
- couche entrée features,
- couche calcul (poids/activation),
- couche sortie probabilité.

---

## PROCESSUS P5 — Calibration multi-seuil
### Objectif
Transformer probabilité en masque binaire stable score-oriented.

### Étapes
1. Calcul seuil adaptatif global + slice-wise.
2. Fusion low/mid/high (selon version).
3. Contrainte densité cible.
4. Nettoyage morphologique.

### Couches
- couche seuil,
- couche fusion,
- couche régulation densité.

---

## PROCESSUS P6 — Inférence par tranches (slice-first)
### Objectif
Limiter mémoire et stabiliser z.

### Étapes
1. Traiter tranche z par tranche z.
2. Accumuler score local.
3. Lisser continuité inter-tranches.
4. Émettre masque 3D final.

### Couches
- boucle slice,
- anti-clignotement z,
- assemblage volume.

---

## PROCESSUS P7 — Packaging & validation soumission
### Objectif
Sortir un ZIP Kaggle conforme.

### Étapes
1. Binarisation finale (`0/1` ou `0/255` selon mode).
2. Écriture TIFF multipage compressé.
3. Création CSV/ZIP.
4. Validation de contenu.

### Couches
- couche format,
- couche conformité,
- couche export.

---

## PROCESSUS P8 — Forensic / observabilité
### Objectif
Prouver ce qui a été fait et comparer les runs.

### Étapes
1. Logs événementiels.
2. Hash/checksum sorties.
3. Statistiques densité/slices.
4. Rapport décisionnel GO/NO-GO.

### Couches
- télémétrie,
- intégrité,
- décision.

---

## 3) Phase par phase (vision run compétition)

## Phase A — Pré-exécution fermée (obligatoire)
- Exécuter sans soumission.
- Produire score/proxy interne.
- Comparer au TIFF concurrent.
- Appliquer verrou soumission si inférieur.

## Phase B — Itération technique
- Corriger features/seuils/fusion.
- Réduire variance run à run.
- Stabiliser densité utile.

## Phase C — Qualification
- 2 reruns cohérents minimum.
- Validation format + forensic ok.
- Déverrouillage soumission uniquement si condition atteinte.

## Phase D — Soumission contrôlée
- Seulement après GO.
- Utiliser quota journalier avec discipline stricte.

---

## 4) “Neurone principal” vs CNN/autres modèles (explication humaine)

## Neurone principal (dans ce contexte)
Ici, “neurone principal” représente le **moteur de décision final** qui agrège les signaux (features) et décide encre/non-encre.
- Avantage: rapide, contrôlable, calibrable finement.
- Risque: moins puissant qu’un CNN profond si features pauvres.

## CNN (convolutionnel)
Un CNN apprend lui-même des motifs spatiaux via convolutions.
- Avantage: capte textures complexes du papyrus.
- Risque: plus coûteux en RAM/temps, plus sensible au setup.

## Position NX46 VX unifié
Le notebook fusionné mélange:
- logique “classique calibrée” (seuils/fusion/forensic),
- blocs de type apprentissage supervisé (certaines branches V144.x),
- stratégies slice-wise pragmatiques (V7.x).

En langage simple: **on combine cerveau analytique + réflexes robustes + contrôle qualité strict**.

---

## 5) Dictionnaire ultra-pédagogique (termes techniques)

- **TIFF multipage 3D**: fichier image contenant plusieurs couches (slices z).
- **Slice**: une coupe 2D du volume 3D.
- **Feature**: indicateur numérique extrait de l’image (contraste, gradient...).
- **Calibration**: réglage des seuils/poids pour mieux coller au signal réel.
- **Quantile/percentile**: seuil basé sur la distribution des valeurs.
- **Morphologie**: opérations de nettoyage de masque (ouvrir/fermer/composantes).
- **Pseudo-labels**: labels estimés automatiquement quand labels réels limités.
- **Regularization**: contrainte pour éviter sur-ajustement.
- **Forensic runtime**: journal “preuve” technique d’exécution.
- **Hash/checksum**: empreinte garantissant intégrité d’un fichier.
- **GO/NO-GO**: décision autoriser/interdire la suite du pipeline.

---

## 6) Traduction “langue humaine” du notebook (A→Z)

1. Le notebook démarre, vérifie/installe ce qu’il lui faut pour lire/écrire TIFF.
2. Il cherche les données Kaggle au bon endroit.
3. Il lit les volumes, les rend comparables (normalisation).
4. Il extrait des signaux qui ressemblent à de l’encre.
5. Il fait tourner un moteur d’apprentissage/décision.
6. Il transforme la probabilité en masque binaire via seuils intelligents.
7. Il lisse entre tranches pour éviter du bruit “clignotant”.
8. Il fabrique la soumission (TIFF + ZIP) conforme.
9. Il écrit des preuves (logs, stats, empreintes) pour audit.
10. Dans NX46 VX: on impose le verrou “pas de soumission tant que pré-exécution < concurrent”.

---

## 7) Comparaison synthétique des 4 bases intégrées

- **V61.5**: forte composante exécution GPU stricte + forensic orienté run.
- **V144.2**: bloc le plus riche en fonctions d’analyse/apprentissage/calibration.
- **V7.7**: robuste côté pipeline Kaggle-ready, packaging, validation zip.
- **V7.6**: proche V7.7, stabilité opérationnelle + logique binaire/soumission.

Conclusion pratique:
- V144.2 apporte profondeur analytique,
- V7.x apporte robustesse production,
- V61.5 apporte discipline runtime/forensic.

---

## 8) Check-list exécution interne (avant toute soumission)

1. Dépendances OK (manifest présent).
2. Discovery test/train validée.
3. Pré-exécution complète sans exception.
4. Proxy/score pré-exécution calculé.
5. Comparaison au TIFF concurrent effectuée.
6. Si inférieur: lock soumission maintenu.
7. Si égal/supérieur: rerun cohérence + GO final.

---

## 9) Résumé ultra-court

- Le notebook unifié contient la totalité des briques majeures des 4 versions.
- Il sait charger, apprendre/calibrer, inférer slice-wise, produire TIFF/ZIP, et tracer.
- Nouvelle règle imposée: **pas de soumission tant que pré-exécution n’atteint pas le niveau TIFF concurrent**.
- Ce document sert de cours opérationnel complet pour comprendre chaque couche du système.
