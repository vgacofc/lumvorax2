# RAPPORT 133 — AUDIT DE DIVERGENCE LOCAL D7 / DISTANT F7 ET ÉTAT RÉEL DES CONFLITS — 20261001

## 0. Expertises activées

- Audit forensique Git/GitHub et analyse de provenance des commits
- Réconciliation local/distant et analyse de divergence de branches
- Analyse statique C
- Validation numérique CFD / Navier–Stokes
- Analyse de convergence et séparation erreurs spatiales/temporelles
- Dynamique non linéaire et méthodologie Lyapunov
- Reproductibilité expérimentale
- Audit documentaire, conflits Git et intégrité de la série RAPPORT/
- Traçabilité code → artefact → log → rapport

## 1. Objet du rapport

Le présent rapport vérifie l'état réellement observable dans le dépôt distant `vgacofc/lumvorax2` après la session locale annonçant :

- HEAD local `d7f000f` ;
- résolution de quatre conflits Git ;
- création du rapport 132 ;
- zéro marqueur `<<<<<<<` dans l'arbre local ;
- push encore bloqué.

Le contrôle est volontairement effectué contre le dépôt distant réellement accessible. Il ne transforme pas une déclaration locale en fait distant.

## 2. Processus

Le processus consiste à :

1. interroger l'historique récent de `main` ;
2. vérifier le dernier commit distant ;
3. inspecter l'arborescence distante ;
4. vérifier les fichiers scientifiques concernés ;
5. rechercher les marqueurs de conflits Git ;
6. comparer les constatations actuelles avec le point de référence du rapport 131 ;
7. conserver séparément les faits distants et les affirmations relatives à l'état local.

C'est-à-dire : un fichier résolu localement n'est considéré comme résolu dans GitHub qu'après présence du commit correspondant dans l'historique distant.

## 3. Résultat de synchronisation distante

Le dernier commit actuellement observable sur `main` est :

`f7f8b18c54b582b1d4aea5e31f7c130681b2065c`

Message : `RAPPORT 131 - audit reprise et plan de fermeture scientifique`.

Le commit précédent est :

`be201f4dde77e43ccf5b9b06fa11bc34bed4a370`

Le commit local annoncé `d7f000f` n'est pas présent dans l'historique distant actuellement accessible.

Conclusion : la résolution locale annoncée n'est pas encore propagée dans le dépôt distant.

## 4. Processus — conflits Git

Une recherche distante des marqueurs de conflit a été effectuée sur `main`.

Le marqueur `<<<<<<<` est encore trouvé dans sept fichiers, dont notamment :

- `compterendu.md`
- `RAPPORT_IAMO3/NX/REPONSE_FORMELLE_REVIEWER_2_NX39.1.md`
- `RAPPORT_IAMO3/NX/REPONSE_FORMELLE_REVIEWER_2_NX39 (copy).md`
- `RAPPORT/125_RAPPORT_RECONCILIATION_FORENSIQUE_ETAT_REEL_C1_C5_20261001.md`
- `RAPPORT/131_AUDIT_REPRISE_ET_PLAN_FERMETURE_SCIENTIFIQUE_C1_C7_20261001.md`
- `RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé.md`
- `RAPPORT-VESUVIUS/fxgfgfchgf`

Le marqueur `=======` est également présent dans de nombreux fichiers ; le marqueur `>>>>>>>` est retrouvé dans huit fichiers, dont les mêmes artefacts principaux.

## 5. Problème critique — le rapport 131 lui-même contient encore un conflit distant

Le rapport 131 est bien présent sur le commit `f7f8b18`, mais la recherche distante montre encore un marqueur `<<<<<<<` dans ce fichier.

Cela crée une distinction essentielle :

- le rapport 131 documente une anomalie ;
- mais sa propre copie distante n'est pas encore proprement fusionnée ;
- la résolution locale annoncée n'est donc pas reflétée sur `main`.

Il ne faut pas déclarer la série documentaire globalement propre tant que le commit de résolution n'est pas poussé.

## 6. Problème — V138 reste en conflit sur le distant

Le fichier :

`RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé.md`

existe bien sur `main`.

Cependant, son contenu distant contient encore explicitement des sections séparées par des marqueurs de conflit Git.

Le conflit oppose notamment une section issue de `codex/analyze-nx-47-learning-process-and-compare-models-45z5qg` à la version concurrente.

Conclusion distante : V138 reste NON RÉSOLU.

La résolution locale annoncée dans la session courante devra donc être vérifiée après propagation de `d7f000f`.

## 7. Processus — C1

Le fichier `src/tests/nx42_30_problems_execution_v2.c` est présent.

La mesure temporelle utilise effectivement `clock_gettime(CLOCK_MONOTONIC)`.

Le mécanisme de mesure est donc réel.

## 8. Problème — C4

La même inspection confirme que :

- les problèmes 6–30 restent des calculs LCG minimaux ;
- leur statut imprimé reste `STUB_MEASURED` ;
- le problème 5 est encore explicitement décrit comme un stub Navier–Stokes ;
- mesurer correctement une opération ne transforme pas cette opération en résolution du problème mathématique annoncé.

Conclusion : C4 reste OUVERT sur le distant.

## 9. Processus — convergence

Le fichier `src/validation/ns_convergence_study.c` est présent.

Il exécute trois résolutions 32×32, 64×64 et 128×128 avec `dt=0.001` et calcule des ordres apparents.

Le code conserve cependant une logique de test où T01 est une saturation de L2 à moins de 10 % et où T02 vérifie un résidu de Poisson.

C'est-à-dire : ces deux tests ne constituent pas, à eux seuls, une démonstration stricte d'un ordre spatial asymptotique.

Conclusion : le chantier Richardson reste OUVERT conformément au rapport 131.

## 10. Problème — T04

Le code de T04 mesure seulement :

- énergie au pas 100 ;
- énergie au pas 3000 ;
- positivité des deux valeurs.

Il ne construit pas une série temporelle et ne compte pas les remontées d'énergie.

Conclusion : le nom « décroissance monotone » ne correspond toujours pas à une vérification mathématique de monotonie.

En outre, pour une cavité entraînée, le système reçoit de l'énergie via le couvercle mobile. Une condition de décroissance globale doit donc être définie physiquement avant d'en faire un critère de validation.

## 11. Processus — Lyapunov

Le fichier `src/validation/ns_lyapunov.c` est présent et implémente effectivement une procédure de type Benettin avec renormalisation périodique.

Paramètres observés :

- 32×32 ;
- Re=100 ;
- dt=0.001 ;
- warmup=3000 ;
- n_renorm=100 ;
- 50 renormalisations ;
- epsilon utilisé dans le code : 1e-4.

## 12. Problème — incohérence documentaire Lyapunov

Le commentaire du fichier annonce une perturbation initiale epsilon=1e-6 alors que l'exécution utilise epsilon=1e-4.

Cette différence reste un défaut de reproductibilité documentaire.

La valeur de lambda ne doit donc pas être considérée comme robuste tant qu'une étude de sensibilité n'a pas été effectuée sur epsilon, le temps de renormalisation, le warmup et la résolution.

## 13. C3 — intégration NX

Le dépôt contient plusieurs variantes de `sch_nx_v11.c`, notamment dans le dataset VESUVIUS.

L'existence de ces fichiers ne suffit pas à démontrer que le pipeline NX-42 exécute effectivement `ns_solver_2d.c`.

La fermeture de C3 exige une preuve de chaîne :

entrée NX → point d'appel → `ns_solver_2d` réellement exécuté → sortie → log → validation.

Conclusion : C3 reste OUVERT tant que cette chaîne n'est pas démontrée.

## 14. État scientifique consolidé

| Chantier | État distant actuel |
|---|---|
| C1 — mesure latence | Corrigé et présent |
| C2 — label NX35 | Corrigé documentaire ; formule historique ouverte |
| Solveur NS 2D | Présent |
| Validation Ghia | Présente dans les artefacts de validation |
| Richardson strict | Ouvert |
| T04 monotonie | Ouvert |
| Robustesse Lyapunov | Ouverte |
| C3 intégration NX-42 | Non démontrée |
| C4 problèmes 6–30 | Ouvert, STUB_MEASURED |
| V138 | Conflit distant toujours présent |
| Autres conflits Git | Présents |
| Rapport 132 local annoncé | Non vérifiable dans `main` sous ce numéro de série |

## 15. Distinction locale / distante

### État local annoncé dans la session

Le bilan fourni annonce :

- résolution des quatre conflits ciblés ;
- zéro marqueur de conflit dans l'arbre local ;
- commit `d7f000f` ;
- rapport 132 ;
- push bloqué par 403.

### État distant effectivement vérifié

Le dépôt distant reste sur `f7f8b18`.

Les marqueurs de conflit sont toujours présents.

Le commit `d7f000f` n'est pas observable dans l'historique distant.

Le rapport 132 annoncé localement n'est donc pas considéré comme livré sur `main`.

## 16. Solution et suggestions

### Solution immédiate

Pousser le commit local `d7f000f` sur `main` après résolution de l'authentification GitHub.

### Contrôle immédiatement après push

Effectuer dans cet ordre :

1. vérifier le nouveau HEAD distant ;
2. vérifier que `d7f000f` est présent ;
3. rechercher `<<<<<<<`, `=======` et `>>>>>>>` ;
4. vérifier les quatre fichiers spécifiquement résolus ;
5. vérifier la présence du rapport 132 ;
6. comparer le nombre de fichiers modifiés avec le commit local ;
7. seulement ensuite déclarer C7 fermé.

### Important

Aucune correction scientifique ne doit être déduite du seul fait que les conflits Git ont disparu.

La résolution documentaire et la validation scientifique restent deux chaînes distinctes.

## 17. Persistance des chantiers

Les chantiers antérieurs restent actifs :

- P0 — robustesse Lyapunov ;
- P0 — convergence Richardson réellement contrôlée ;
- P0 — intégration NS dans NX-42 ;
- P0 — remplacement des stubs 6–30 ;
- P1 — T04 correctement défini ;
- P1 — inventaire et résolution des conflits documentaires ;
- P2 — reconstruction de la formule historique NX35.

Aucun de ces chantiers n'est considéré abandonné.

## 18. Point de vérité au terme de l'audit

Le dépôt distant vérifié est en retard sur l'état local annoncé.

La session locale semble avoir produit une résolution plus avancée, mais cette résolution n'est pas encore démontrable sur `main` tant que `d7f000f` n'est pas accessible dans l'historique distant.

Le point de vérité distant reste donc :

- artefacts scientifiques 127–131 présents ;
- plusieurs corrections scientifiques réelles ;
- preuves de convergence et robustesse Lyapunov encore ouvertes ;
- C3 et C4 encore ouverts ;
- conflits Git encore présents sur `main`.

Aucun code scientifique n'est modifié par ce rapport.
