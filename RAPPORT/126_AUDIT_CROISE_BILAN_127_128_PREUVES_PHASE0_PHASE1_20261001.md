# RAPPORT 126 — AUDIT CROISÉ DU BILAN 127–128 ET VÉRIFICATION DES PREUVES PHASE 0 / PHASE 1

**Dépôt :** vgacofc/lumvorax2
**Branche auditée :** main
**Date d'audit :** 2026-10-01
**Objet :** vérifier rigoureusement les affirmations du bilan utilisateur concernant les rapports 127 et 128, les corrections C1/C2, le solveur Navier–Stokes 2D, l'isolation ARTCB et les prochaines étapes 129–130.
**Règle :** aucune modification du code, aucune correction fonctionnelle. Seul le présent rapport est ajouté à RAPPORT/.

## 1. EXPERTISES ACTIVÉES
- Audit forensique Git/GitHub et réconciliation de révision.
- Contrôle de provenance et chronologie des preuves.
- Analyse statique C et vérification de présence des artefacts.
- Métrologie de performance et distinction mesure réelle / valeur déclarée.
- Mécanique des fluides numérique et validation CFD.
- Analyse numérique des solveurs de Navier–Stokes.
- Contrôle de validation par benchmark externe, notamment Ghia.
- Audit de chaîne documentaire et numérotation.
- Audit d'isolation de projet et de règles Git.

## 2. PROCESSUS D'AUDIT
Le bilan fourni affirme que les rapports 127 et 128 ont déjà été exécutés. Je ne transforme pas cette affirmation en preuve. Je recherche les fichiers, les chemins, les résultats et les artefacts dans la révision distante actuelle.

Une validation de type PASS exige quatre niveaux :
1. le fichier source ou le programme existe ;
2. le test ou l'exécution correspondante existe ;
3. le résultat est conservé dans un log vérifiable ;
4. le rapport relie explicitement résultat, paramètres et code.

C'est-à-dire : un nombre écrit dans un rapport n'est pas équivalent à une mesure retrouvée dans Git.

## 3. ÉTAT DISTANT ACTUEL
La branche main contient les artefacts historiques nombreux et le RAPPORT 125 créé lors de l'audit précédent.

Le dernier état précédemment observé était la révision 2ea3a87e23ef49cc7285010bc6fc527faf119564. Le présent audit se concentre sur la présence effective des nouveaux artefacts annoncés.

Les branches actuellement visibles sont :
- audit/rapport-forensique-c55-validation
- bob
- cursor/fusion-dt-plasma-module-333b
- feature/mdbai-setup
- main

Aucune branche visible ne porte explicitement les artefacts 127/128 annoncés.

## 4. RAPPORT 127 ANNONCÉ — VÉRIFICATION
### Processus
Le bilan annonce le fichier LVX&ARTCB/src/tests/nx42_30_problems_execution_v2.c et indique une conversion de constantes de latence vers clock_gettime(CLOCK_MONOTONIC).

### Vérification
La recherche GitHub dans le dépôt actuel ne retrouve pas nx42_30_problems_execution_v2.c.

La recherche ne retrouve pas non plus les chemins annoncés :
- LVX&ARTCB/src
- LVX&ARTCB/logs
- LVX&ARTCB/RAPPORT
- RAPPORT/127_
- RAPPORT/128_
- RAPPORT/129_
- RAPPORT/130_.

### Problème
Le bilan affirme donc une correction qui n'est pas démontrable dans l'arborescence distante actuellement accessible.

Ce constat est différent de « la correction n'a jamais existé ». Il signifie : « la correction n'est pas présente dans le dépôt/révision que nous auditons actuellement sous les chemins annoncés ».

### Solution
Avant de déclarer C1 corrigée dans ce dépôt, il faut retrouver :
1. le fichier v2 ;
2. le diff ou commit contenant la transformation ;
3. le binaire ou exécutable utilisé ;
4. le log d'exécution ;
5. les paramètres machine ;
6. le rapport 127.

**Statut rapport 127 : NON VÉRIFIÉ dans le dépôt actuel.**

## 5. CHRONOLOGIE — ANOMALIE IMPORTANTE
Le bilan cite un log nommé 127_nx42_v2_execution_20261002.log et un rapport daté 20261002.

La date courante de l'audit est 2026-10-01.

### Problème
Une preuve datée du 2 octobre 2026 ne peut pas être considérée comme une exécution déjà observée au 1er octobre 2026 sans explication supplémentaire de la source ou de l'horloge.

C'est-à-dire : un timestamp futur peut provenir d'une horloge système mal réglée, d'un artefact préparé à l'avance ou d'une autre machine ; il ne peut pas être présenté comme preuve d'une exécution locale déjà vérifiée sans provenance.

### Solution
Pour accepter cette preuve, il faut conserver :
- timestamp brut ;
- timezone ;
- hostname ou identifiant machine ;
- commit SHA du code exécuté ;
- commande exacte ;
- sortie complète ;
- hash du log.

**Statut chronologique : NON VALIDÉ.**

## 6. RAPPORT 128 ANNONCÉ — SOLVEUR NAVIER–STOKES
### Processus
Le bilan décrit un solveur 2D par méthode de projection de Chorin, grille décalée, Poisson Gauss-Seidel SOR et cavité entraînée, avec comparaison à Ghia 1982.

### Vérification
La recherche GitHub ne retrouve pas `ns_solver_2d.c` dans le dépôt actuel.
Elle ne retrouve pas non plus un rapport 128 correspondant ni un log `128_ns_lid_driven_ghia_20261002.log`.

Les recherches `ns_solver_2d`, `ghia`, `Re=100` et `5000 pas` ne fournissent pas de preuve directe de ce nouveau solveur dans l'état audité.

### Problème
Les valeurs annoncées :
- 64×64 ;
- Re=100 ;
- 5 000 pas ;
- 3,9 s ;
- L∞ = 0,0168 ;
- L∞ = 0,0152 ;
- tolérance 0,05 ;
- PASS ;
ne sont donc pas actuellement reproductibles à partir des fichiers annoncés dans ce dépôt.

### Solution
Pour transformer cette affirmation en validation scientifique, il faut retrouver :
1. le source du solveur ;
2. les constantes et conditions aux limites ;
3. le code de calcul des profils ;
4. les données Ghia exactes utilisées ;
5. le programme de comparaison ;
6. le log brut ;
7. le commit exact ;
8. les erreurs L∞ calculées à partir des données brutes.

**Statut rapport 128 : NON VÉRIFIÉ.**

## 7. ATTENTION SCIENTIFIQUE SUR « PASS GHIA 1982 »
### Processus
Une validation contre une référence scientifique ne consiste pas uniquement à obtenir une erreur inférieure à une tolérance choisie. Il faut aussi démontrer que les variables comparées, les positions d'échantillonnage, les conventions de signe, le maillage, les conditions aux limites et la définition de l'erreur sont compatibles.

### Problème
Les seuls nombres fournis dans le bilan ne permettent pas de contrôler ces éléments.

Une erreur L∞ de 0,0168 peut être parfaitement calculée tout en étant scientifiquement mal définie si, par exemple, les points comparés ne correspondent pas aux points de référence.

### Solution
Le futur rapport de validation doit publier le tableau complet des points, les valeurs numériques calculées, les valeurs de référence, l'erreur absolue par point et la formule exacte de L∞.

**Verdict scientifique : PASS non attribuable tant que les données primaires ne sont pas retrouvées.**

## 8. C2 — LYAPUNOV : ÉTAPE 129
Le rapport 125 avait établi que la valeur `metric_lyapunov = 0.0254219` est présente dans le log P9, mais que la prétendue contradiction avec `STABLE` n'était pas encore démontrée.

Le bilan annonce maintenant une correction de cette convention.

### Processus
Un exposant de Lyapunov décrit un taux moyen de séparation exponentielle des trajectoires. Mais l'interprétation du signe dépend de la définition exacte de la quantité calculée et du système considéré.

### Problème
Changer simplement le texte STABLE/UNSTABLE sans recalculer ou sans établir la convention mathématique ne constitue pas une correction scientifique.

### Solution
Le rapport 129 doit établir :
1. la formule exacte utilisée ;
2. la trajectoire ou série temporelle d'entrée ;
3. la méthode d'estimation ;
4. la fenêtre temporelle ;
5. les unités ;
6. la tolérance numérique ;
7. la règle de décision stabilité ;
8. le résultat brut ;
9. la cohérence entre résultat et label.

**Statut C2 : chantier toujours ouvert tant que ces preuves ne sont pas retrouvées.**

## 9. PHASE 2 — RAPPORT 130 ANNONCÉ
Le bilan prévoit de remplacer `nx11_physics_stub` par le solveur NS et de calculer un Lyapunov sur le champ de vorticité.

### Processus
La vorticité est le rotationnel du champ de vitesse. En 2D, elle permet de caractériser la rotation locale du fluide. Un exposant de Lyapunov calculé sur une série dérivée de vorticité est cependant une construction scientifique spécifique ; il faut définir exactement l'état dynamique dont on mesure la séparation.

### Problème
Brancher un solveur NS dans NX ne suffit pas à rendre valide automatiquement un Lyapunov.

Il faut distinguer :
- validation du solveur fluide ;
- extraction de la vorticité ;
- définition de l'espace d'état ;
- calcul de l'exposant ;
- interprétation de stabilité.

### Solution
Ordre recommandé :
1. valider indépendamment le solveur NS ;
2. vérifier conservation et convergence ;
3. extraire la vorticité ;
4. définir l'observable ou l'état utilisé pour Lyapunov ;
5. effectuer le calcul sur données réellement produites ;
6. comparer à une référence ou à une propriété analytique lorsque disponible ;
7. seulement ensuite intégrer NX-42.

**Statut Phase 2 : conception cohérente, implémentation non vérifiée dans le dépôt actuel.**

## 10. ISOLATION ARTCB — RÉCONCILIATION
Le `.gitignore` actuellement présent dans main a été relu directement.

Il contient des règles Python, résultats, benchmarks, binaires, logs et caches, mais aucune ligne `LVX&ARTCB/`.

### Problème
L'affirmation « `.gitignore` ligne 82 = LVX&ARTCB/ » n'est donc pas vraie pour le `.gitignore` actuellement présent sur main.

Il est possible que cette règle existe dans un autre dépôt, une autre branche ou une copie locale, mais elle n'est pas démontrée dans la révision distante auditée.

### Solution
Fournir ou retrouver le dépôt/branche qui contient cette règle et effectuer ensuite une vérification Git complète des chemins réellement suivis.

**Statut isolation dans main : NON CONFIRMÉE par cette règle.**

## 11. TÂCHES ANTÉRIEURES QUI RESTENT OUVERTES
Conformément à la règle de persistance des travaux, les anciens chantiers ne sont pas considérés comme abandonnés.

### C1
Les latences historiques codées en dur restent un problème démontré dans le code historique. La correction v2 annoncée n'est pas retrouvée.

### C2
La convention Lyapunov reste ouverte.

### C3
L'absence d'un solveur NS dans `sch_nx_v11.c` reste établie pour ce fichier ; le nouveau solveur annoncé n'est pas retrouvé.

### C4
Les validations décoratives des problèmes 6–30 restent un problème du fichier historique. La correction correspondante n'est pas retrouvée.

### C5
L'isolation par `LVX&ARTCB/` n'est pas confirmée dans le `.gitignore` actuel.

### V138
Le conflit Git documentaire précédemment détecté doit rester dans la liste P0 tant qu'une version propre n'est pas démontrée.

## 12. MATRICE DE VALIDATION DU BILAN FOURNI
| Affirmation | État dans GitHub actuel |
|---|---|
| Rapport 127 Phase 0 C1 | NON VÉRIFIÉ |
| nx42_30_problems_execution_v2.c | NON RETROUVÉ |
| log 127 du 2026-10-02 | NON RETROUVÉ + date future à l'instant de l'audit |
| Rapport 128 NS 2D | NON VÉRIFIÉ |
| ns_solver_2d.c | NON RETROUVÉ |
| log Ghia 2026-10-02 | NON RETROUVÉ |
| PASS Ghia L∞ 0,0168 / 0,0152 | NON VÉRIFIÉ |
| Rapport 129 C2 | NON RETROUVÉ |
| Rapport 130 Phase 2 | NON RETROUVÉ |
| `.gitignore` LVX&ARTCB/ | NON PRÉSENT DANS MAIN |

## 13. CONCLUSION
Le bilan utilisateur décrit une évolution techniquement plausible et structurée, mais les artefacts nécessaires pour la considérer comme accomplie ne sont pas présents sous les chemins annoncés dans le dépôt `vgacofc/lumvorax2` actuellement audité.

Le point le plus important est la séparation entre « résultat annoncé » et « résultat vérifiable ». Aucun PASS scientifique ne doit être enregistré uniquement à partir d'un texte de bilan.

À ce stade, la base de travail vérifiable reste :
- C1 historique confirmée ;
- C2 partiellement confirmée ;
- C3 historique confirmée ;
- C4 historique confirmée ;
- C5 non confirmée dans main ;
- corrections 127/128 annoncées mais non retrouvées ;
- Phase 2 annoncée mais non vérifiable.

**Aucune modification de code n'a été effectuée.**
**Le présent document est le RAPPORT 126 de la série RAPPORT/.**