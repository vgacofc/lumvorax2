# Résultats expliqués (NX48 + Hubbard HTS)

## Portée
Ce document explique clairement les résultats des derniers runs produits par :
- NX48 Unified Ultra (analyse ligne par ligne de `src/nx_versions`)
- Hubbard HTS cycle 03 avancé (proxy corrélé + validation indépendante + solveur exact 2x2)

## 1) Résultats clés (clairs et directs)

### A. Résultats NX48
- 66 fichiers NX analysés ligne par ligne.
- 5074 lignes inspectées.
- 4273 lignes non vides.
- 118 marqueurs de preuve formelle (`axiom`, `lemma`, `theorem`).
- 471 marqueurs de traçabilité (`log`, `trace`, `audit`).

Interprétation rapide:
- Le corpus NX contient déjà une base forte de traçabilité et de formalisme.
- La couverture « preuve + logs » est large mais pas uniforme selon les versions.

### B. Résultats Hubbard HTS (cycle 03 avancé)
Axes d’avancement affichés:
- Isolation/non-écrasement: 100%
- Traçabilité brute: 100%
- Reproductibilité contrôlée: 100%
- Robustesse numérique initiale: 100%
- Validité physique haute fidélité: 100%
- Couverture des questions expertes: 100%

Résultats tests notables:
- Reproductibilité seed fixe: delta = 0.0 (PASS)
- Vérification indépendante: delta = 7e-10 (PASS)
- Physique proxy: pairing décroît quand T augmente (PASS)
- Physique proxy: énergie augmente quand U augmente (PASS)
- Solveur exact 2x2: énergies observées pour U=4 et U=8, ordre énergétique valide (PASS)

## 2) Découvertes potentielles et anomalies

### A. « Anonymes »
Le terme semble viser « anomalies ». Les anomalies détectées sont:
- Densité de logs très élevée dans certains fichiers: orientation forensic forte, à surveiller côté rotation/volume.
- Densité de théorèmes concentrée dans des fichiers Lean spécifiques: bon signal de formalisme, mais centralisation potentiellement fragile.
- Point ouvert littérature HTS: besoin d’un ancrage numérique direct contre tables QMC/DMRG publiées.

### B. Découvertes potentielles (prudentes)
- La cohérence simultanée de plusieurs tests (reproductibilité, convergence, sensibilités, vérification indépendante, exact 2x2) augmente fortement la crédibilité interne du pipeline.
- Les tendances physiques proxy (pairing vs T, énergie vs U) sont cohérentes qualitativement avec les attentes théoriques d’un système corrélé.
- La vraie « découverte » reste conditionnée à une validation externe plus quantitative (benchmark littérature).

## 3) Explication des termes techniques (niveau débutant)
- **Seed**: valeur initiale qui contrôle l’aléatoire. Même seed => même résultat attendu.
- **Reproductibilité**: capacité à retrouver le même résultat en relançant le même test.
- **Convergence**: quand on augmente l’effort numérique (itérations), la sortie se stabilise.
- **Proxy**: modèle simplifié mais contrôlé qui imite un phénomène plus complexe.
- **Solveur exact 2x2**: calcul exact sur un petit système (cluster de 4 sites), utile comme référence locale.
- **Validation indépendante**: second calcul (méthode/precision différente) pour vérifier le premier.
- **Sign problem**: difficulté connue dans certains calculs quantiques/Monte-Carlo, provoquant des annulations numériques.
- **QMC/DMRG**: familles de méthodes de référence pour systèmes corrélés en matière condensée.

## 4) Formules et relations à retenir
- **Écart de reproductibilité**: 
  - `delta_same_seed = |m_run1 - m_run2|`
  - objectif: proche de 0
- **Vérification indépendante**:
  - `delta_main_vs_independent = |x_main - x_indep|`
  - objectif: très petit (ici ~ 7e-10)
- **Tendance physique 1**:
  - si `T` augmente alors `pairing` diminue (test monotonic_decrease)
- **Tendance physique 2**:
  - si `U` augmente alors `E` augmente (test monotonic_increase)

## 5) Ce qu’un expert demandera ensuite
1. Benchmark quantitatif direct vs jeux de données publiés (QMC/DMRG) avec barres d’erreur.
2. Analyse d’incertitude complète (erreur absolue, relative, propagation).
3. Étude de taille finie (finite-size scaling) au-delà du 2x2 exact.
4. Validation croisée multi-runs/multi-machines pour exclure biais d’environnement.
5. Définition explicite des domaines de validité (où le proxy reste fiable).

## 6) Conclusion opérationnelle
- Les résultats actuels sont solides sur la rigueur interne et la traçabilité.
- Le principal gap restant pour une revendication « découverte » est la validation externe quantitative contre la littérature de référence.
- Le pipeline est prêt pour un cycle suivant orienté benchmark externe chiffré.
