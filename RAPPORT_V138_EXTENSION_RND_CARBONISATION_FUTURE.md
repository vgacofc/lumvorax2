# RAPPORT ANNEXE (séparé) — Extension future R&D carbonisation/décarbonisation

## Objet du document

Ce document est volontairement séparé du plan V138 principal. Il sert de base pour une future mise à jour, lorsque décidée, afin d’explorer une simulation physique exploitable pour améliorer la détection d’encre réelle.

---

## 1) Reformulation de l’intuition stratégique

Intuition: simuler la transformation **origine -> carbonisé -> scan** puis apprendre un modèle inverse **scan carbonisé -> état probable d’origine** pour enrichir l’apprentissage de NX47 sur les données réelles.

Bénéfice attendu:
- meilleure robustesse aux variations de texture/carbonisation,
- meilleure séparation papyrus/encre,
- meilleure généralisation hors cas « propres ».

---

## 2) Questions d’autocritique (expert) et réponses

### Q1 — Est-ce scientifiquement justifié ou juste attractif?
- Réponse: justifié si les hypothèses physiques sont explicites, les limites connues, et le gain validé sur données réelles (pas seulement synthétiques).

### Q2 — Le coût HPC est-il soutenable?
- Réponse: oui si multi-niveaux (mésoscopique prioritaire) et si on réserve l’atomique aux sous-problèmes critiques.

### Q3 — Risque principal?
- Réponse: sur-apprentissage au domaine synthétique (domain gap). Il faut calibration et validation stricte sur scans réels.

### Q4 — Comment prouver la valeur?
- Réponse: ablations: baseline réelle vs réelle+synthétique, mêmes splits, mêmes seeds, gains stables.

### Q5 — Qu’est-ce qui ferait arrêter cette piste?
- Réponse: absence de gain réel répété, coût disproportionné, ou contradictions physiques majeures.

---

## 3) Cadre de simulation proposé (progressif)

### Niveau A — Voxel physiquement simplifié (priorité)
- Modèle thermodynamique simplifié (pyrolyse/oxydation agrégées),
- simulation de contraste CT plausible,
- génération de volumes synthétiques supervisés.

### Niveau B — Méso-structure papyrus/encre
- fibres orientées,
- encre/pigments et diffusion,
- déformations/fissures/bris thermiques.

### Niveau C — Sous-modules atomiques ciblés
- réservé à des micro-phénomènes difficiles,
- utilisé comme ancrage scientifique, pas comme pipeline complet standard.

---

## 4) Chaîne directe/inverse exploitable NX47

1. Génération état initial (papyrus + encre de référence).
2. Simulation carbonisation et dégradation.
3. Simulation acquisition scanner (bruit/résolution/artefacts).
4. Entraînement modèle inverse/auxiliaire.
5. Transfert vers pipeline réel NX47.
6. Évaluation stricte sur données compétition réelles.

---

## 5) Critères de réussite (go/no-go)

Go si:
- gain reproductible sur validation réelle,
- pas de régression stabilité,
- coût calcul acceptable,
- traçabilité complète des générateurs synthétiques.

No-go si:
- gains non robustes,
- dérive domaine synthétique,
- coût disproportionné,
- impossibilité d’audit causal.

---

## 6) Livrables futurs si activé

- Spécification scientifique des hypothèses.
- Générateur synthétique versionné (hash + seeds).
- Rapport comparatif baseline vs augmentation synthétique.
- Rapport limites/falsification.

---

## 7) Positionnement

Cette extension est prometteuse mais doit rester:
- falsifiable,
- mesurable,
- traçable,
- secondaire tant que la conformité et la preuve d’exécution réelle V138 ne sont pas verrouillées.

