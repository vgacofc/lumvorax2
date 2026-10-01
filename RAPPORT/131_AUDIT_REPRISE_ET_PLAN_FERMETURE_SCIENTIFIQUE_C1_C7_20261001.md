# RAPPORT 131 — AUDIT DE REPRISE ET PLAN DE FERMETURE SCIENTIFIQUE C1-C7 — 20261001

## 0. Périmètre et révision de vérité

Expertises activées :
- audit forensique Git/GitHub ;
- analyse statique C ;
- métrologie et reproductibilité expérimentale ;
- CFD / mécanique des fluides numérique ;
- analyse de convergence numérique ;
- dynamique non linéaire / exposants de Lyapunov ;
- validation numérique et conservation ;
- audit documentaire et gestion des conflits Git ;
- traçabilité code → log → rapport.

Dépôt audité : vgacofc/lumvorax2.
Branche : main.
HEAD distant observé : be201f4dde77e43ccf5b9b06fa11bc34bed4a370.
Le dépôt est donc synchronisé sur le même HEAD que celui annoncé pour la session.

Le rapport 131 n'était pas présent au moment de l'audit. Ce document constitue le nouveau point de suivi de la série RAPPORT/.

Aucun code source scientifique n'est modifié par le présent rapport.

## 1. Processus de reprise

Le contrôle a été effectué directement sur la révision distante actuelle et non sur la seule déclaration de session.

C'est-à-dire : chaque chantier ouvert est confronté au code et aux artefacts effectivement présents dans main. Une tâche n'est déclarée fermée que si le mécanisme, la preuve et la traçabilité sont cohérents.

Les rapports antérieurs 125 à 130 restent la base documentaire. Le rapport 130 avait déjà établi que plusieurs artefacts 127–129 étaient livrés, mais que la convergence stricte et la robustesse Lyapunov restaient ouvertes.

## 2. Chantier 1 — Robustesse de l'exposant de Lyapunov

### Processus

Le fichier src/validation/ns_lyapunov.c implémente effectivement une procédure de type Benettin : deux solveurs sont avancés, une perturbation est introduite, la différence de vorticité est mesurée, puis la perturbation est renormalisée périodiquement. La valeur finale est calculée par accumulation des logarithmes de croissance.

Les paramètres actuellement codés sont :
- grille 32×32 ;
- Re=100 ;
- dt=0,001 ;
- warmup=3000 pas ;
- n_renorm=100 ;
- 50 renormalisations ;
- epsilon=1e-4.

### Problème

Le mécanisme est réel, mais une seule configuration ne permet pas de démontrer que lambda=-1,426073 est une valeur robuste.

Point important supplémentaire : le commentaire d'en-tête mentionne epsilon=1e-6, alors que le code utilise epsilon=1e-4. Cette incohérence documentaire doit être corrigée dans le futur rapport ou code de validation, mais elle ne doit pas être masquée.

La norme utilisée pour la renormalisation est une norme de différence de vorticité, tandis que la renormalisation agit sur les champs u, v et p. C'est une définition opérationnelle spécifique au projet ; elle doit être explicitement assumée et testée.

### Solution et suggestions

Construire une matrice de sensibilité au minimum sur :
- epsilon = 1e-3, 1e-4, 1e-5 ;
- n_renorm = 50, 100, 200 ;
- warmup = 2000, 3000, 5000 ;
- résolution = 32×32 puis 64×64 si le coût est acceptable.

Pour chaque configuration :
1. enregistrer lambda ;
2. enregistrer le temps physique mesuré ;
3. vérifier absence de divergence ;
4. publier min, max, moyenne et dispersion ;
5. ne conclure à une valeur robuste que si les variations restent bornées selon un critère défini à l'avance.

Statut : OUVERT — priorité scientifique P0.

## 3. Chantier 2 — T04 énergie cinétique

### Processus

Le test actuel exécute 3000 pas sur 32×32, conserve l'énergie au pas 100 et compare cette valeur à l'énergie finale au pas 3000.

### Problème

Le nom et la documentation annoncent une décroissance monotone, mais le code ne mesure pas la monotonie.

La condition réelle est seulement :
- EK@100 > 0 ;
- EK@3000 > 0.

Donc un signal qui augmente, diminue, oscille ou possède plusieurs remontées peut passer ce test.

### Solution et suggestions

Remplacer conceptuellement ce test par une série de N snapshots consécutifs, par exemple 100 ou davantage.

Pour chaque snapshot :
- calculer EK ;
- vérifier EK(i+1) <= EK(i) + tolérance numérique ;
- compter le nombre de violations ;
- publier la plus grande remontée ;
- distinguer une monotonie stricte d'une monotonie à tolérance.

Attention scientifique : dans une cavité entraînée, l'énergie du système complet peut recevoir de l'énergie par le couvercle mobile. Il ne faut donc pas imposer sans justification une décroissance monotone globale de l'énergie sur toute la phase transitoire. Le test doit être reformulé selon la quantité physique réellement attendue et la fenêtre temporelle choisie.

Statut : OUVERT — priorité P1.

## 4. Chantier 3 — Richardson / convergence spatiale stricte

### Processus

Le programme actuel utilise trois grilles 32×32, 64×64 et 128×128 et calcule une quantité L2 par comparaison au profil Ghia.

Il calcule également des ordres apparents :
- p32→64 = log(E32/E64)/log(2) ;
- p64→128 = log(E64/E128)/log(2).

### Problème

Le programme appelle cette étude Richardson, mais les valeurs historiques observées sont proches de zéro pour les ordres calculés : environ 0,007 et -0,014.

Le test T01 mesure seulement une variation relative de L2 inférieure à 10 %. Cela démontre une saturation ou une stabilité de la métrique choisie, pas un ordre spatial de convergence.

Le test T02 porte en réalité sur le résidu du solveur de Poisson et non sur l'ordre de convergence spatial.

La situation est donc :
- résultat expérimental réel ;
- métriques utiles ;
- intitulé « preuve Richardson » trop fort.

### Solution et suggestions

Pour une vraie étude de convergence :
1. définir une solution de référence indépendante, par exemple une grille beaucoup plus fine et suffisamment stationnaire ;
2. contrôler le temps physique final identique ;
3. coupler le pas temporel à la résolution, par exemple dt proportionnel à dx ou selon une contrainte CFL fixée ;
4. mesurer l'erreur de chaque grille par rapport à la même référence ;
5. vérifier que l'erreur diminue de manière régulière ;
6. calculer l'ordre sur plusieurs raffinements ;
7. distinguer erreur spatiale, erreur temporelle et erreur de comparaison à la référence.

Le choix 256×256 comme référence est une proposition de protocole, pas une preuve en soi : la grille de référence doit elle-même être vérifiée comme suffisamment résolue.

Statut : OUVERT — priorité scientifique P0.

## 5. Chantier 4 — C2 : formule historique NX-35

### Processus

La valeur historique metric_lyapunov=0.0254219 est présente dans NX35_LOG_P9.csc et le label a été corrigé vers WEAKLY_CHAOTIC selon la convention documentée.

### Problème

La valeur historique n'est pas reliée à une formule reconstruite, à un état dynamique source et à une procédure de calcul reproductible.

Il faut également éviter de présenter le seuil 0,01 comme une loi générale : il s'agit d'une convention de classification du projet.

### Solution et suggestions

Rechercher la chaîne complète :
entrée → transformation → formule → accumulation → valeur 0.0254219 → label.

Si la chaîne historique ne peut pas être reconstruite, conserver explicitement le statut « valeur historique non reproduite » et ne pas la présenter comme une mesure scientifiquement reproduite.

Statut : OUVERT — priorité P2 documentaire.

## 6. Chantier 5 — C4 : problèmes 6–30

### Processus

Le fichier src/tests/nx42_30_problems_execution_v2.c mesure maintenant réellement le temps avec CLOCK_MONOTONIC.

### Problème

La mesure de latence est réelle, mais elle mesure un stub générique pour les problèmes 6–30. Le code utilise notamment une boucle LCG déterministe.

C'est-à-dire : mesurer correctement le temps d'exécution d'un calcul fictif ne transforme pas ce calcul en résolution du problème annoncé.

Les problèmes 1–5 utilisent également nx11_physics_stub ; le problème 5 est explicitement nommé « Navier-Stokes Dissipation (stub) ».

### Solution et suggestions

Conserver le label STUB_MEASURED tant qu'aucun calcul spécifique n'existe.

Pour fermer C4, chaque problème doit avoir :
- une définition précise du problème ;
- un algorithme réellement associé ;
- des entrées explicites ;
- une sortie vérifiable ;
- un test de correction ;
- une mesure de performance séparée de la validation scientifique.

Statut : OUVERT — priorité P0 code/scientifique.

## 7. Chantier 6 — C3 : intégration NS dans le pipeline NX-42

### Processus

Le solveur ns_solver_2d.c est désormais un solveur Navier–Stokes 2D réel basé sur projection de Chorin, grille décalée, Euler explicite, différences centrées et Poisson SOR.

### Problème

La présence du solveur ne démontre pas son intégration dans sch_nx_v11.c ni dans le pipeline NX-42.

Il faut conserver la séparation suivante :
- validation du solveur ;
- intégration dans NX ;
- validation de l'intégration.

Une intégration réussie ne constitue pas automatiquement une preuve scientifique du solveur.

### Solution et suggestions

Avant toute conclusion :
1. établir le point d'appel exact ;
2. tracer les entrées/sorties ;
3. vérifier que le solveur exécuté est bien ns_solver_2d ;
4. comparer un run avec et sans intégration ;
5. conserver des logs distincts de validation physique et de validation pipeline.

Statut : OUVERT — priorité P0.

## 8. Chantier 7 — conflit documentaire V138

### Processus

Le fichier « RAPPORT V138 — Cartographie technologique exhaustive, écarts V125→V137 et plan d’intégration verrouillé.md » est toujours présent avec des marqueurs de conflit Git explicites :
<<<<<<<
=======
>>>>>>>

### Problème

Le document n'est pas un artefact documentaire proprement fusionné.

Point important : le conflit n'est pas limité à un ancien fichier abstrait. Il est toujours observable sur la révision main actuellement auditée.

Une recherche de marqueurs <<<<<<< montre également d'autres fichiers du dépôt contenant des conflits. Le chantier V138 doit donc être traité comme une anomalie documentaire plus large, et non comme un unique fichier isolé.

### Solution et suggestions

Pour le fichier V138 :
1. déterminer quelle branche/version représente la vérité documentaire ;
2. fusionner manuellement les deux sections ;
3. supprimer tous les marqueurs de conflit ;
4. vérifier la cohérence des numéros de sections ;
5. rechercher de nouveau les marqueurs dans le fichier ;
6. documenter le commit de résolution.

Pour les autres fichiers, effectuer un inventaire séparé afin de ne pas mélanger leur résolution avec le rapport 131.

Statut : OUVERT — priorité P1 Git/documentation.

## 9. État global après synchronisation

| Chantier | État au HEAD be201f4 | Conclusion |
|---|---|---|
| C1 mesure latence | Corrigé | CLOCK_MONOTONIC présent |
| C2 label NX35 | Corrigé documentaire | formule historique encore ouverte |
| NS 2D | Livré | validation Ghia utile, convergence asymptotique encore ouverte |
| Richardson strict | Ouvert | étude actuelle = saturation/validation, pas preuve d'ordre |
| T04 énergie | Ouvert | pas de test de monotonie |
| Lyapunov robustesse | Ouvert | une configuration seulement |
| C4 problèmes 6–30 | Ouvert | STUB_MEASURED |
| C3 intégration NX | Ouvert | solveur présent, branchement pipeline non démontré |
| V138 conflit | Ouvert | marqueurs encore présents |

## 10. Ordre de fermeture recommandé

### P0 scientifique
1. Robustesse Lyapunov.
2. Étude de convergence réellement contrôlée.
3. Clarification des critères physiques du test énergie.

### P0 architecture
4. Démonstration de l'intégration ns_solver_2d dans NX-42.

### P0 code scientifique
5. Remplacement progressif des stubs 6–30 par des calculs spécifiques.

### P1 documentaire/Git
6. Résolution propre du conflit V138.
7. Inventaire séparé des autres fichiers contenant des marqueurs de conflit.

### P2 historique
8. Reconstruction de la formule NX-35 metric_lyapunov.

## 11. Persistance des tâches

Aucun chantier antérieur n'est abandonné.

Le registre de continuité après le rapport 131 est :
- C1 : fermé pour la métrologie de latence ;
- C2 : partiellement fermé, formule historique ouverte ;
- C3 : solveur réel livré, intégration NX ouverte ;
- C4 : ouvert ;
- convergence : ouvert ;
- Lyapunov : ouvert ;
- T04 : ouvert ;
- V138 : conflit documentaire ouvert ;
- isolation LumVorax/ARTCB : à maintenir séparée et à vérifier directement dans le dépôt ARTCB lorsque nécessaire.

## 12. Conclusion de l'audit

Le dépôt distant est bien revenu sur HEAD be201f4 et les artefacts des rapports 127–130 sont présents.

Le statut « 100 % livré » peut être utilisé pour décrire la livraison des artefacts de la session. Il ne doit pas être utilisé pour déclarer la fermeture scientifique de tous les chantiers.

Le point de vérité du rapport 131 est donc :

- livraison documentaire 127–130 : confirmée ;
- C1 : corrigée ;
- solveur NS 2D : présent ;
- validation Ghia : présente selon le critère défini par le projet ;
- convergence Richardson stricte : non démontrée ;
- robustesse Lyapunov : non démontrée ;
- T04 monotonie énergétique : non démontrée ;
- C4 problèmes 6–30 : non résolu ;
- intégration NS dans NX-42 : non démontrée ;
- conflit V138 : toujours présent.

Aucun code source scientifique n'a été modifié pour produire ce rapport.
