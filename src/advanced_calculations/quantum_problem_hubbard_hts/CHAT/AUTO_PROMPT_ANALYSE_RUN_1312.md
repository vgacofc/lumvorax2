# AUTO-PROMPT — MODÈLE DE REQUÊTE D'ANALYSE POUR FUTUR CYCLE
## Généré automatiquement pour le run `research_20260312T123420Z_1312`

---

Ce fichier est le prompt structuré à réutiliser tel quel pour demander l'analyse du prochain cycle. Il codifie exactement ce qu'il faut demander pour obtenir le même niveau d'analyse approfondie.

---

```
Lis toi-même ligne par ligne, fichier par fichier, chaque CSV, JSON, log et code source C des deux runs suivants :

NOUVEAU RUN (à analyser) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/<RUN_ID_NOUVEAU>/

ANCIEN RUN (référence) :
  src/advanced_calculations/quantum_problem_hubbard_hts/results/research_20260312T123420Z_1312/

ANALYSES PRÉCÉDENTES (à intégrer sans les modifier) :
  src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/

CODE SOURCE C (à auditer ligne par ligne) :
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle_advanced_parallel.c
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_research_cycle.c
  src/advanced_calculations/quantum_problem_hubbard_hts/src/hubbard_hts_module.c

Produis un rapport d'analyse totale en français, sauvegardé dans :
  src/advanced_calculations/quantum_problem_hubbard_hts/CHAT/RAPPORT_ANALYSE_TOTALE_<ID_NOUVEAU>_vs_1312_<DATE>.md

Sans modifier aucun fichier existant dans CHAT/.

Le rapport doit couvrir obligatoirement dans cet ordre :

1. INVENTAIRE DES FICHIERS : liste et compte de chaque fichier nouveau/disparu entre les deux runs.

2. TABLEAU COMPARATIF LIGNE PAR LIGNE des observables physiques clés :
   énergie, pairing, sign_ratio, drift, norme ψ, amplitude FFT, pompage dynamique.

3. ANALYSE DE L'INTÉGRATEUR NUMÉRIQUE (code C) :
   - Lire la fonction d'évolution pas à pas (boucle step)
   - Identifier si normalize_state_vector() est appelé DANS la boucle ou non
   - Identifier la méthode déclarée dans metadata (euler_explicit / rk2_without_forced_renorm / rk2_stabilized)
   - Détecter toute incohérence entre label et implémentation réelle
   - Donner les numéros de ligne exacts avant/après modification

4. ANALYSE DE CHAQUE ERREUR :
   Pour chaque test FAIL ou régression :
   - Nom du test et fichier CSV
   - Valeur dans l'ancien run
   - Valeur dans le nouveau run
   - Cause physique exacte
   - Cause de code exacte (fichier, numéro de ligne, extrait avant/après)
   - Solution à appliquer avec la ligne de code exacte à écrire

5. VALIDATION DES CORRECTIONS des cycles précédents :
   Chaque point signalé comme problème dans les analyses CHAT/ antérieures :
   vérifié, RÉSOLU ou NON RÉSOLU avec preuve dans les données.

6. POINTS NON SIGNALÉS PRÉCÉDEMMENT que tu aurais détectés toi-même.

7. TABLEAU BILAN final : score par dimension, progression V4-Next, FAILs restants.

8. FEUILLE DE ROUTE prochaine correction : actions prioritaires avec fichier et ligne de code exacts.
```

---

**Critères de qualité du rapport attendu :**
- Chaque affirmation appuyée sur un fichier ou une ligne de code précise
- Aucune valeur inventée — toutes extraites des CSV/JSON/logs réels
- Toutes les incohérences entre labels et code réel explicitement nommées
- Explications pédagogiques : chaque concept expliqué sans jargon opaque
- Aucune modification des fichiers CHAT/ existants
