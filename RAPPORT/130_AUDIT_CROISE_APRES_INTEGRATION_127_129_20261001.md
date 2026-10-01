# RAPPORT 130 — AUDIT CROISÉ APRÈS INTÉGRATION DES ARTEFACTS 127–129

**Dépôt :** vgacofc/lumvorax2
**Branche :** main
**Révision auditée :** e029391815815d9507d13a18bb685ff83329e13c
**Date de l'audit :** 2026-10-01
**Objet :** vérification forensique des corrections C1/C2, du solveur NS 2D, des tests de convergence, du calcul de Lyapunov et de l'isolation documentaire.
**Modification du code :** AUCUNE.

## 1. EXPERTISES ACTIVÉES
- Audit forensique Git/GitHub et provenance des commits.
- Réconciliation d'historique et comparaison de révisions.
- Analyse statique C.
- Métrologie et instrumentation temporelle.
- Mécanique des fluides numérique et CFD.
- Analyse numérique de la convergence.
- Validation de solveurs Navier–Stokes.
- Dynamique non linéaire et calcul d'exposants de Lyapunov.
- Audit de reproductibilité expérimentale.
- Contrôle de cohérence code / log / rapport.
- Audit documentaire et isolation inter-dépôts.

## 2. PROCESSUS
Le rapport 126 précédent avait déclaré les artefacts 127–129 non vérifiés parce qu'ils n'étaient pas encore présents sur main. Le nouvel état distant a été vérifié directement.

Le commit e029391 existe bien et son historique contient d3bfd7a. La comparaison avec ae2999b montre l'ajout des rapports, logs, solveur NS 2D, étude de convergence, calcul Lyapunov et correction C1/C2.

**Conclusion de provenance : les artefacts annoncés sont maintenant présents dans le dépôt distant.**

## 3. C1 — MESURE DES LATENCES
### Processus
src/tests/nx42_30_problems_execution_v2.c utilise clock_gettime(CLOCK_MONOTONIC) avant et après le calcul. Les latences sont donc obtenues par mesure d'exécution et non par les constantes historiques 2500/1800 ns.

Le log 127 contient des valeurs variables : 371000/166000 ns, 276000/143000 ns, 280000/141000 ns, etc.

### Problème
Cette correction concerne la métrologie, pas la validité mathématique des problèmes.
Les problèmes 1–5 utilisent encore nx11_physics_stub(), explicitement décrit comme une simulation dissipative LCG minimale.
Les problèmes 6–30 utilisent seulement un calcul LCG de 100 itérations et sont correctement marqués STUB_MEASURED.

### Solution
Conserver C1 comme correction de mesure. Pour les microbenchmarks très courts, répéter le calcul suffisamment de fois avant de mesurer le temps moyen permettrait de réduire l'effet de la résolution du timer et du scheduler.

**Verdict C1 : CORRIGÉE ET VÉRIFIABLE.**

## 4. C4 — PROBLÈMES 6–30
### Processus
Les problèmes 6–30 possèdent maintenant une opération CPU réelle et mesurée.

### Problème
Cette opération est un LCG générique. Elle ne constitue pas l'implémentation mathématique de Riemann, Goldbach, Collatz, etc.

### Solution
Maintenir le statut STUB_MEASURED jusqu'à ce qu'un calcul spécifique, documenté, testable et reproductible soit associé à chaque problème.

**Verdict C4 : AMÉLIORÉE POUR LA MÉTROLOGIE, NON RÉSOLUE SCIENTIFIQUEMENT.**

## 5. C2 — CONVENTION LYAPUNOV
### Processus
NX35_LOG_P9_CORRECTED.csc classe 0.0254219 comme WEAKLY_CHAOTIC. Le code NS utilise la convention générale lambda > 0 = chaos et lambda < 0 = stabilité.

### Problème
Le seuil 0.01 utilisé pour distinguer MARGINAL_CHAOS et WEAKLY_CHAOTIC est une convention interne au projet, pas une loi générale de la dynamique.
De plus, le code reconnaît que la formule historique ayant produit metric_lyapunov=0.0254219 dans NX-35 n'est pas reconstituée.

### Solution
Conserver le label corrigé comme correction documentaire, mais ne pas présenter 0.0254219 comme une mesure Benettin validée tant que la chaîne de calcul NX-35 n'est pas retrouvée.

**Verdict C2 : LABEL CORRIGÉ ; ORIGINE MATHÉMATIQUE DE LA VALEUR HISTORIQUE NON ÉTABLIE.**

## 6. C3 — SOLVEUR NAVIER–STOKES 2D
### Processus
src/solvers/ns_solver_2d.c existe maintenant sur main. Le code contient une grille décalée, une étape de vitesse intermédiaire, advection/diffusion, Poisson de pression par Gauss-Seidel SOR, correction de vitesse et conditions Lid-Driven Cavity.

### Problème
Le benchmark Ghia donne PASS selon le seuil choisi, mais le rapport 128 reconnaît lui-même : Euler explicite d'ordre 1, grille 64×64, 5000 pas et état encore légèrement transitoire.

Le point y=0.7344 présente même un changement de signe entre la référence (+0.00332) et la simulation (-0.013519).

### Solution
Pour une validation plus forte : étude du pas temporel, étude spatiale avec dt adapté à dx, critère quantitatif d'état stationnaire, conservation et comparaison des deux profils complets.

**Verdict C3 : SOLVEUR NS RÉEL PRÉSENT ; BENCHMARK GHIA PASS SELON LE CRITÈRE DU RAPPORT ; CONVERGENCE ASYMPTOTIQUE COMPLÈTE NON DÉMONTRÉE.**

## 7. ÉTUDE DE CONVERGENCE — T01 À T05
### Processus
Trois grilles 32×32, 64×64 et 128×128 sont exécutées pendant 20000 pas avec dt=0.001.
Les L2 obtenues sont 0.011174, 0.011123 et 0.011230.

### Problème
Le nom Richardson est trop fort par rapport à ce que démontre le programme.
L'ordre calculé est 0.007 pour 32→64 et -0.014 pour 64→128. Cela ne démontre pas un ordre spatial de convergence égal à 1.

T01 vérifie une variation inférieure à 10 %. C'est un test de saturation de la métrique, pas une preuve mathématique de convergence.
T04 est encore plus important : le commentaire parle d'une décroissance d'énergie, mais le code vérifie seulement que l'énergie à t=3000 et celle à t=100 sont positives. Il ne teste pas une décroissance monotone.

### Solution
Renommer cette étape en étude de raffinement et de stabilité, ou modifier le protocole pour réellement démontrer l'ordre de convergence.
Pour un ordre spatial, réduire dt avec le raffinement et calculer l'ordre sur une erreur dont la référence est indépendante.
Pour T04, tester réellement la propriété annoncée si elle est physiquement justifiée.

**Verdict : T01/T02/T03/T05 PASS selon leurs critères locaux ; T04 PASS ne signifie pas décroissance monotone ; la convergence Richardson stricte reste à démontrer.**

## 8. LYAPUNOV NS 2D
### Processus
Le programme utilise une orbite de référence, une orbite perturbée, la vorticité comme observable, une renormalisation périodique et une accumulation logarithmique. Le log donne lambda=-1.426073.

### Problème
Le résultat est une mesure numérique réelle, mais un exposant de Lyapunov dépend de l'espace d'état, de la norme, de la perturbation, de l'intervalle de renormalisation, du warmup et de la fenêtre d'observation.

La construction actuelle utilise une norme de différence de vorticité pour la renormalisation tout en perturbant les champs u/v/p. Il faut donc documenter cette définition comme une construction opérationnelle propre au projet.

### Solution
Tester la sensibilité à epsilon, à l'intervalle de renormalisation, à la durée de warmup, à la fenêtre d'observation et au maillage. Publier une plage de résultats plutôt qu'une seule valeur.

**Verdict : CALCUL RÉEL PRÉSENT ET TRAÇABLE ; ROBUSTESSE DU λ ENCORE À ÉTABLIR.**

## 9. C5 — ISOLATION ARTCB
### Processus
La preuve de la ligne LVX&ARTCB/ à la ligne 82 appartient au dépôt ARTCB distinct.

### Problème
Cette règle ne peut pas être vérifiée depuis le seul dépôt vgacofc/lumvorax2.

### Solution
Auditer directement le dépôt ARTCB : .gitignore, git check-ignore, git status, chemins suivis, historique et recherche de fichiers LumVorax.

**Verdict C5 : CONFIRMÉE CÔTÉ ARTCB SELON LA PREUVE FOURNIE ; NON RECONSTITUABLE DEPUIS LE SEUL DÉPÔT LUMVORAX.**

## 10. COHÉRENCE DOCUMENTAIRE
Les rapports 127–129 sont maintenant présents sur main.
Certaines références de type SHA HEAD ARTCB dans ces rapports appartiennent à l'autre dépôt. Elles doivent rester identifiées comme références externes et ne doivent pas être confondues avec des commits de vgacofc/lumvorax2.

## 11. MATRICE FINALE
| Domaine | État |
|---|---|
| Artefacts 127–129 | CONFIRMÉS |
| C1 métrologie | CORRIGÉE |
| C4 problèmes 6–30 | STUBS TOUJOURS PRÉSENTS |
| C2 labeling historique | CORRIGÉ |
| Formule historique NX-35 | NON RECONSTRUITE |
| Solveur NS 2D | PRÉSENT |
| Ghia | PASS selon seuil choisi |
| Convergence asymptotique | NON DÉMONTRÉE |
| Richardson strict | NON DÉMONTRÉ |
| Lyapunov NS | CALCUL RÉEL PRÉSENT |
| Robustesse Lyapunov | À ÉTUDIER |
| C5 isolation ARTCB | CONFIRMÉE côté ARTCB |
| Modification du code pendant cet audit | AUCUNE |

## 12. CONCLUSION
Le problème principal du rapport 126 est résolu : les artefacts 127–129 sont maintenant réellement présents sur main et leur provenance est vérifiable.

Le statut 100 % peut être retenu pour la livraison documentaire des artefacts annoncés, mais pas pour la fermeture scientifique de tous les chantiers.

Les travaux persistants sont :
1. remplacer progressivement les stubs par des calculs spécifiques ;
2. renforcer la démonstration de convergence NS ;
3. transformer l'étude dite Richardson en véritable étude d'ordre ;
4. caractériser la robustesse du Lyapunov ;
5. retrouver, si possible, la formule historique NX-35 ;
6. auditer directement le dépôt ARTCB pour C5 ;
7. conserver la résolution du conflit documentaire V138 dans les tâches ouvertes.

**Aucun code source n'a été modifié pendant cet audit.**