# ROADMAP STRATÉGIQUE — NX46 VX (Sprint compétition J-3)
Date: 2026-02-24
Version cible: **NX46 VX**
Périmètre: Vesuvius challenge (pipeline train/calibration/inference/submission)

---

## 1) Mandat consolidé (utilisateur + historique RAPPORT-VESUVIUS)

Cette roadmap fusionne:
- les demandes stratégiques explicites utilisateur,
- les plans et rapports historiques de `RAPPORT-VESUVIUS`,
- le retour d’expérience des cycles de validation `validation_lumvorax`.

### Contraintes absolues
1. **Ne pas intégrer dans NX46 VX** la partie:
   - `Bridge natif C/.so` (instrumentation conservée, activation runtime exclue du plan opérationnel J-3).
2. Respecter la fenêtre compétition:
   - **9 soumissions restantes**, maximum **3 soumissions/jour**.
3. Priorité: score utile + stabilité + reproductibilité, et non complexification non testée.

---

## 2) Objectifs stratégiques NX46 VX

1. **TIFF concurrent comme source d’apprentissage indirecte**
   - calibrage automatique dynamique,
   - adaptation des plages de détection,
   - robustesse accrue aux ancres papyrus.

2. **Fusion des 4 versions existantes**
   - une seule branche opérationnelle NX46 VX,
   - réintégration des meilleures fonctions de chaque version,
   - conservation du neurone/performance de référence issu de V61.

3. **Traitement par tranches (slice-first)**
   - pipeline z-slice séquentiel,
   - anti-fuite détectable en run/submission,
   - maîtrise mémoire en environnement Kaggle.

4. **Système professeur (9 modèles concurrents)**
   - distillation multi-enseignants,
   - fine-tuning train complet,
   - ultra-fine-tuning test contrôlé (sans fuite de protocole),
   - supervision additionnelle par TIFF concurrent.

5. **Apprentissage complet A→Z**
   - pipeline 100% activé,
   - production TIFF réel final,
   - validation complète pré-soumission.

6. **Analyse anatomique d’apprentissage**
   - traçabilité de transfert enseignant→neurone principal,
   - identification des signaux discriminants ancres,
   - forensic HFBL 360 (bit/nanoseconde orienté décision).

---

## 3) Ce qui est explicitement hors-scope (pour éviter retard)

- Activation réelle du bridge natif C/.so en runtime compétition.
- Refonte système massive sans preuve de gain mesuré.
- Campagnes “large brute-force” non compatibles budget 9 soumissions.

---

## 4) Architecture cible NX46 VX (fusion unifiée)

## 4.1 Blocs pipeline
1. `data_contract_guard` (layout train/test, fail-fast explicite)
2. `dynamic_calibration_engine` (quantiles, blend, smoothing, densité cible)
3. `slice_inference_engine` (traitement par tranche + stabilisation z)
4. `teacher_distillation_engine` (9 professeurs + pondération adaptative)
5. `forensic360_observability` (delta masque, métriques, hash TIFF/ZIP)
6. `submission_controller` (gating GO/NO-GO, budget de soumissions)

## 4.2 Règle d’or
Aucune release NX46 VX n’est candidate si l’une des preuves manque:
- run complet sans exception,
- soumission valide,
- delta masque explicable,
- score comparatif enregistré,
- décision GO/NO-GO tracée.

---

## 5) Nouveau plan opérationnel (J-3)

## PHASE 0 — Alignement et baseline (Jour 1, créneau 1)
- Geler baseline “champion courant”.
- Créer `run_registry` unique (run→hash→score→paramètres).
- Définir protocole A/B strict (même dataset/même conditions).

Livrables:
- baseline hash TIFF + ZIP,
- seuils initiaux,
- fiche variance attendue.

## PHASE 1 — Fusion technique des 4 versions (Jour 1, créneau 2)
- Intégrer modules utiles sans régression masquée.
- Réinjecter neurone/performance V61 de référence.
- Ajouter compatibilité totale slice-first.

Livrables:
- build NX46 VX alpha,
- rapport de mapping fonctions source→cible,
- test de non-régression format.

## PHASE 2 — Distillation “Professeurs x9” (Jour 1, créneau 3 + Jour 2 créneau 1)
- Construire pondération enseignants (confiance par zone/slice).
- Injecter TIFF concurrent comme signal auxiliaire de calibration.
- Produire version NX46 VX beta (distillée).

Livrables:
- matrice poids enseignants,
- indicateurs de transfert vers neurone principal,
- comparaison baseline vs beta.

## PHASE 3 — Calibration dynamique ciblée (Jour 2, créneaux 2-3)
- Ajuster plages détection selon calibrage observé.
- Contraintes morphologiques anti-bruit + stabilité inter-slices.
- Balayage court (petit nombre de combinaisons à fort rendement).

Livrables:
- top-3 configurations candidate,
- registre gains/pertes par config,
- statut robustesse mémoire/runtime.

## PHASE 4 — Soumissions pilotées budget 9 (Jour 3)
- Max 3 soumissions/jour, priorisées par score proxy + risque.
- Stratégie: 1 conservatrice / 1 agressive / 1 équilibrée.
- Arrêt immédiat si chute significative ou anomalie de validation.

Livrables:
- tableau décisionnel des 9 soumissions,
- champion final NX46 VX RC,
- dossier de reproductibilité complet.

---

## 6) Cadence de soumissions recommandée (9 restantes)

- **Jour 1**: 2 soumissions (baseline confirm + alpha fusion)
- **Jour 2**: 3 soumissions (beta distillation + calibration)
- **Jour 3**: 3 soumissions (finalistes)
- **Réserve**: 1 soumission “joker” (uniquement si signal net)

---

## 7) GO/NO-GO NX46 VX (gating strict)

## GO si tout est vrai
1. runtime complet sans exception,
2. `submission_content_validation = ok`,
3. gain score >= baseline + marge définie,
4. variance contrôlée (>=2 reruns cohérents),
5. forensic cohérent: hash, delta masque, densité utile,
6. budget soumissions respecté.

## NO-GO si au moins un échec
- absence score exploitable,
- gain non reproductible,
- delta masque non explicable,
- dérive densité extrême,
- instabilité mémoire/temps.

---

## 8) Suggestions additionnelles (points souvent oubliés)

1. **Budget keeper automatique**: blocage soft au-delà de 3 soumissions/jour.
2. **Kill-switch anti-régression**: rollback auto sur champion précédent.
3. **Score proxy interne**: corrélation continue pour éviter soumissions “à l’aveugle”.
4. **Journal exécutable unique**: un seul JSONL par run, signé hash commit.
5. **Check-list pré-upload**: format, densité, connectivité, variance, mémoire.
6. **Observabilité pédagogique**: mini-rapport “ce que les professeurs ont appris” à chaque release.

---

## 9) Nommage/versioning

- Dossier officiel: `RAPPORT-VESUVIUS/NX46-VX/`
- Builds:
  - `NX46-VX-alpha` (fusion)
  - `NX46-VX-beta` (distillation)
  - `NX46-VX-rc` (candidat final)

