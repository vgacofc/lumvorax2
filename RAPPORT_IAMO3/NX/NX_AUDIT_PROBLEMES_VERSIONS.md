# AUDIT DES PROBLÈMES ET TESTS PAR VERSION (NX-1 À NX-10)

Ce document décrit les problèmes spécifiques testés et les objectifs scientifiques de chaque version, sans aucune altération des fichiers sources originaux.

---

## NX-1 : RECOUVREMENT DES COMPOSANTS ET CAUSALITÉ INITIALE
- **Problème testé** : Existence de la fonction neuronale sans stabilité ni poids.
- **Test spécifique** : Mesure de la dissipation structurée minimale.
- **Log NX-11** : Validation de la causalité initiale via `UTC_NS` nanoseconde.

## NX-2 : CARTOGRAPHIE DES RÉGIMES POSSIBLES
- **Problème testé** : Identification des états (INERTE, FONCTIONNEL, CHAOTIQUE, EFFONDREMENT).
- **Test spécifique** : Détection des frontières de transition entre les régimes.
- **Log NX-11** : Trace des transitions réelles via `REGIME` et `BIT_TRACE`.

## NX-3 : SCIENCE DES TRANSITIONS ET IRRÉVERSIBILITÉ
- **Problème testé** : Pourquoi et comment la fonction est perdue ou retrouvée.
- **Test spécifique** : Analyse de l'irréversibilité des processus de dissipation.
- **Log NX-11** : Enregistrement de la flèche du temps via `EVENT_ID` monotone.

## NX-4 : ANALYSE STATISTIQUE ET GÉOMÉTRIQUE
- **Problème testé** : Quantification des temps de résidence et signature des invariants.
- **Test spécifique** : Construction du diagramme de phase (Énergie vs Densité d'invariants).
- **Log NX-11** : Corrélation énergétique via `ENERGY_DELTA_fJ` et `INVARIANT_DENSITY`.

## NX-5 : PATHOLOGIE DE LA STABILITÉ
- **Problème testé** : Est-ce qu'une stabilité excessive mène à la mort fonctionnelle ?
- **Test spécifique** : Simulation de neurones "trop stables" pour observer l'inefficacité.
- **Log NX-11** : Monitoring du bruit thermique et des invariants sous contrainte.

## NX-6 : MÉMOIRE DISSIPATIVE ET RÉSOLUTION
- **Problème testé** : Granularité minimale de la mémoire neuronale.
- **Test spécifique** : Temps de persistance post-perturbation sans stockage explicite.
- **Log NX-11** : Trace forensique bit-à-bit des changements de mémoire.

## NX-7 : FERMETURE DU NOYAU CAUSAL
- **Problème testé** : Démonstration que le neurone est un système dissipatif physique réel.
- **Test spécifique** : Invalidation des modèles symboliques classiques.
- **Log NX-11** : Validation finale du noyau via `STATE_HASH_BEFORE/AFTER`.

## NX-8 : EXPLORATION DES INCONNUES POST-CAUSALITÉ
- **Problème testé** : Communication NX-NX et synchronisation sans protocole.
- **Test spécifique** : Délai causal minimal entre neurones couplés.
- **Log NX-11** : Suivi multi-parents causaux via `PARENTS`.

## NX-9 : DYNAMIQUE DES SYSTÈMES ÉTENDUS
- **Problème testé** : Émergence de motifs pré-cognitifs.
- **Test spécifique** : Analyse des trajectoires préférentielles indépendantes des conditions initiales.
- **Log NX-11** : Capture de la complexité dynamique via `PHASE_FLAGS`.

## NX-10 : FRONTIÈRES DE L'INTELLIGENCE COMPUTATIONNELLE
- **Problème testé** : Cognition sans représentation et intelligence sans stockage.
- **Test spécifique** : Validation de l'identité mentale comme propriété dissipative.
- **Log NX-11** : Certification totale de l'intégrité via `LINE_HASH_SHA256`.

---
*Document généré le 27 Janvier 2026 - Certification NX-11-HFBL-360*
