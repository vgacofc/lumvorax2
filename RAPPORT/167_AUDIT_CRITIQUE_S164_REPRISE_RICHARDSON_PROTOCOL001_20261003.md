# Rapport 167 — Audit critique S164 : reprise Richardson-PROTOCOL-001 / état réel de 003b→003c

**Date :** 2026-10-03
**Dépôt :** vgacofc/lumvorax2
**Branche :** main
**HEAD audité :** 2561bfa7e087a06455b71907d92ef1e406f49485
**Commit court :** 2561bfa
**Session auditée :** S164
**Rapport précédent :** 166
**CERTIFIED_100=false**
**unique_human_proven=false**
**Mode DEBUG actif**

---

## 1. Expertises activées

- Audit forensique Git/GitHub et vérification de continuité des commits.
- CFD / mécanique des fluides numérique.
- Navier–Stokes incompressible 2D.
- Méthodes aux différences finies et grille MAC/staggered.
- Conditions aux limites périodiques et Couette plan.
- Analyse de projection de Chorin et ordre d'application des conditions limites.
- Analyse de convergence stationnaire.
- Méthode de Richardson et séparation erreur spatiale / temporelle.
- Analyse L1/L2/L∞ et interprétation des métriques numériques.
- Analyse C99, types, contrats d'API et risques de validation.
- Métrologie expérimentale, reproductibilité et critères PASS/FAIL.
- Audit forensic des timestamps et de la traçabilité LUM.
- Gestion de continuité des chantiers antérieurs.

---

## 2. Synchronisation GitHub préalable

Le dépôt distant a été relu avant toute conclusion.

Le HEAD réel de `main` est :

**2561bfa7e087a06455b71907d92ef1e406f49485**

Le commit est :

**Add UNICITE-003 PC2+PC3 fix + UNIF-004 migration v3 (S164)**

Son parent direct est :

**5fdf8d248c83cf707570a4b105da463e389a7d13**

Le commit S164 ajoute notamment :

- `RAPPORT/166_UNICITE003_PC2_PC3_UNIF4_V3_MIGRATION_20261003.md`
- le log JSON S164 ;
- les corrections LUM_ID de la session 164.

Aucune modification Richardson n'apparaît dans le commit S164.

La recherche des commits récents confirme donc que le dernier changement Richardson substantiel reste le travail S159, suivi de l'audit 160. Les commits S162, S163 et S164 sont consacrés à UNICITE/LUM_ID et n'ont pas fermé Richardson-003c.

**Conséquence :** la priorité annoncée par S164, Richardson-PROTOCOL-001, doit être traitée comme une reprise d'un chantier toujours techniquement ouvert, et non comme une fonctionnalité déjà démontrée.

Aucun code source n'est modifié par le présent rapport.

---

# 3. État de vérité immédiat

## Processus

Le chantier Richardson avait suivi cette chaîne :

1. **S154 / Richardson-PROTOCOL-001** : trois lois de pas temporel ont été comparées :
   - dt constant ;
   - dt proportionnel à dx ;
   - dt proportionnel à dx².
2. **S155 / Richardson-PROTOCOL-002** : suppression du temps final fixe et recherche d'une stationnarité adaptative.
3. **S157 / Richardson-PROTOCOL-003** : tentative de validation sur solution exacte Couette `u=y`.
4. **S159 / Richardson-PROTOCOL-003b** : remplacement des bords latéraux par une tentative de périodicité Couette et correction de la coordonnée MAC.
5. **S160** : audit critique démontrant que 003b reste contaminé par le `set_lid_bc()` interne du solveur et qu'une preuve de périodicité discrète reste nécessaire.
6. **S162–S164** : travail UNICITE-001/002/003, sans fermeture de Richardson-003c.

## Problème

Le dépôt contient donc aujourd'hui une chaîne de validations Richardson qui a permis de diagnostiquer plusieurs causes, mais aucune preuve finale d'ordre spatial propre du solveur Navier–Stokes n'est encore établie.

Il serait incorrect de considérer :

- les trois lois de dt comme preuve de séparation spatiale/temporelle ;
- la correction `y_node` comme preuve de convergence ;
- le résultat 003b comme solution Couette exacte validée ;
- ou un futur résultat numérique obtenu avant fermeture du problème de conditions limites comme une preuve Richardson.

## Solution / état recommandé

Conserver Richardson-PROTOCOL-001 en **OPEN** jusqu'à ce qu'un protocole expérimental démontre simultanément :

1. que le problème mathématique discret est bien celui de la référence ;
2. que la solution de référence satisfait réellement les conditions limites ;
3. que les conditions limites sont appliquées au bon moment dans chaque pas ;
4. que la stationnarité est atteinte ;
5. que l'erreur spatiale et l'erreur temporelle sont séparées ;
6. que les métriques diminuent avec le raffinement ;
7. qu'un ordre observé stable est calculé sur au moins deux raffinements successifs.

---

# 4. Richardson-PROTOCOL-001 — résultat historique et portée réelle

## 4.1 Processus

Le protocole S154 a comparé :

- A : dt = constant ;
- B : dt ∝ dx ;
- C : dt ∝ dx².

Les grilles prévues étaient 32×32, 64×64 et 128×128.

L'objectif était de déterminer si une stagnation de L2 provenait :

- d'une erreur temporelle dominante ;
- d'une erreur spatiale ;
- ou d'une autre limitation du problème de référence.

## 4.2 Problème

Les résultats S154 étaient presque identiques entre les trois protocoles.

À t=5 s :

- 32×32 : L2 ≈ 0,019211 ;
- 64×64 : L2 ≈ 0,01856 ;
- 128×128 : L2 ≈ 0,01872.

Changer dt d'un facteur allant jusqu'à 16 n'a donc pas supprimé la stagnation.

Mais cette observation ne suffit pas à conclure immédiatement que « dt n'est pas la cause ».

Pourquoi ?

Parce qu'une expérience de sensibilité à dt n'isole correctement l'erreur temporelle que si :

- le même problème mathématique est résolu ;
- la solution numérique est suffisamment convergée dans le temps ;
- les conditions limites sont cohérentes ;
- la référence est suffisamment précise ;
- le critère de stationnarité ne masque pas une différence entre les protocoles.

## 4.3 Solution

La bonne lecture historique est :

**S154 a montré que réduire dt dans les trois lois testées n'a pas changé significativement la métrique L2 à t=5 s.**

Cela a motivé S155.

La formulation plus forte :

**« l'erreur temporelle est définitivement exclue »**

reste non démontrée.

---

# 5. Richardson-PROTOCOL-002 — stationnarité adaptative

## Processus

S155 a remplacé l'arrêt arbitraire à 5 s par une convergence dynamique basée sur :

- variation de u_max ;
- variation d'énergie cinétique ;
- résidu de Poisson ;
- fenêtre glissante ;
- limite de sécurité de 500 000 pas.

Les résultats du protocole A donnaient environ :

- 32×32 : t ≈ 21,85 s, L2 = 0,011173 ;
- 64×64 : t ≈ 21,80 s, L2 = 0,011122 ;
- 128×128 : t ≈ 21,75 s, L2 = 0,011229.

## Problème

Ces valeurs sont utiles pour montrer que le problème de l'arrêt à 5 s a été traité.

Elles ne constituent cependant pas encore une preuve d'ordre spatial.

Les ordres obtenus étaient :

- 32→64 : ≈ 0,066 ;
- 64→128 : ≈ −0,139.

La tendance n'est pas compatible avec une démonstration d'un ordre spatial propre.

Autre point : le critère de stationnarité est un critère numérique interne, pas une preuve mathématique de proximité avec une solution exacte.

## Solution

Pour Richardson-003c, conserver la stationnarité adaptative comme **condition préalable**, mais ne pas l'utiliser seule comme preuve de convergence spatiale.

Il faut publier pour chaque grille :

- nombre de pas ;
- temps physique final ;
- u_max ;
- énergie ;
- résidu Poisson ;
- variation finale sur fenêtre ;
- L1 ;
- L2 ;
- L∞ ;
- min/max de u ;
- max de |v| ;
- statut convergé ou limite atteinte.

---

# 6. Richardson-PROTOCOL-003b — état du problème Couette

## 6.1 Processus

Le fichier `src/validation/ns_richardson_couette_periodic.c` définit comme référence :

**u_exact(x,y) = y**

avec :

- paroi basse u=0 ;
- paroi haute u=1 ;
- périodicité en x ;
- v=0 ;
- pression homogène.

Mathématiquement, cette référence est pertinente pour un Couette plan périodique si la discrétisation et les conditions limites correspondent effectivement à ce problème.

Le fichier mesure également :

- L1 ;
- L2 ;
- L∞ ;
- u_min ;
- u_max ;
- v_max.

C'est une amélioration importante par rapport à la version précédente, car elle permet de distinguer une erreur de référence d'un véritable overshoot/undershoot.

## 6.2 Problème critique — ordre réel d'application des CL

Le solveur `ns_solver_step()` appelle encore en interne les conditions limites Lid-Driven.

Le test périodique réapplique ensuite ses propres conditions limites après le pas.

C'est-à-dire :

**le solveur ne résout pas directement un pas complet du problème Couette périodique.**

Il résout une étape dans laquelle des conditions Lid-Driven sont appliquées, puis le test remplace les valeurs de frontière après coup.

Cette opération n'est pas équivalente à un solveur dont chaque opérateur discret utilise les conditions Couette dès le départ.

## 6.3 Conséquence

Même si les champs finaux présentent des bords périodiques, les calculs intermédiaires ont pu utiliser les mauvais bords.

Cela peut contaminer :

- le terme advectif ;
- le terme diffusif ;
- la divergence ;
- l'équation de Poisson ;
- la correction de vitesse.

## 6.4 Solution

Richardson-003c doit déplacer la condition limite dans le flux normal du solveur.

Deux architectures sont possibles :

### Option A — callback / politique de conditions limites

Le solveur reçoit une politique de CL et l'applique aux étapes internes appropriées.

Avantage : le solveur reste générique.

### Option B — étape spécialisée Couette

Créer un chemin de validation strictement contrôlé pour le problème Couette.

Avantage : périmètre plus petit.

Dans les deux cas, la preuve doit montrer que les CL Couette sont appliquées avant chaque opérateur qui dépend des valeurs de frontière.

---

# 7. Deuxième problème P0 — périodicité discrète de u à démontrer

## Processus

La version 003b utilise notamment une relation de type :

- bord Ouest = copie de la dernière colonne intérieure ;
- bord Est = copie de la première colonne intérieure.

Cette stratégie cherche à représenter les couches fantômes d'une grille MAC.

## Problème

Le point délicat n'est pas seulement « avoir deux valeurs périodiques ».

Il faut démontrer que la relation utilisée correspond exactement à la localisation physique de la composante u dans le solveur.

Pour une grille MAC, u et v ne sont pas localisés aux mêmes coordonnées que les centres de cellules.

Une copie décalée d'une colonne peut donc représenter :

- une vraie couche fantôme ;
- ou une translation spatiale non voulue.

Le rapport 160 avait correctement maintenu ce point comme OPEN.

## Solution

Avant toute nouvelle campagne, effectuer un test de cohérence discret indépendant :

1. construire le champ analytique u=y, v=0 ;
2. appliquer les CL périodiques ;
3. calculer explicitement les divergences ;
4. calculer le Laplacien discret ;
5. calculer le terme advectif ;
6. vérifier que chaque terme correspond à la solution analytique attendue ;
7. vérifier la périodicité sur les indices exacts utilisés par le solveur.

Cette étape doit produire des résidus mesurables.

**Sans cette preuve, Richardson-003c ne doit pas être déclaré PASS.**

---

# 8. Coordonnée MAC — état corrigé mais causalité historique à ne pas surinterpréter

## Processus

Le protocole 003b utilise :

**y = (j - 0,5) × dy**

pour les valeurs de u intérieures.

Cette convention est cohérente avec la localisation staggered décrite dans le fichier.

## Problème

Le changement de coordonnée est une correction importante, mais il ne démontre pas à lui seul que toute anomalie précédente de L∞ venait de cette seule erreur.

Pour établir une causalité expérimentale, il faut comparer le même état numérique avec :

- ancienne formule ;
- nouvelle formule.

Toutes les autres variables doivent rester identiques.

## Solution

Conserver la correction et ajouter un test de régression métrique :

- même champ u ;
- même grille ;
- même pas ;
- ancienne évaluation ;
- nouvelle évaluation.

Le rapport doit distinguer :

**« convention géométrique corrigée »**

de :

**« cause unique de l'anomalie historique prouvée »**.

---

# 9. L1 ≈ 0,5 et u_min < 0 — interprétation

## Processus

La métrique L1 mesure l'écart moyen absolu entre la solution numérique et u=y.

Lorsque L1 reste proche de 0,5 et que u_min devient négatif, la solution numérique n'est manifestement pas proche du profil Couette attendu.

## Problème

Le mécanisme exact n'est pas encore réduit à une seule cause.

Deux causes sont directement pertinentes :

1. contamination par les conditions Lid-Driven internes ;
2. convention de périodicité discrète à démontrer.

Il faut donc éviter de transformer « cause fortement plausible » en « cause unique démontrée ».

## Solution

Faire une campagne d'isolement :

### Test 1
Champ analytique imposé sans évolution temporelle.

### Test 2
Un seul pas avec CL Couette correctes.

### Test 3
Plusieurs pas avec CL Couette appliquées au bon endroit.

### Test 4
Convergence stationnaire complète.

### Test 5
Raffinement 32→64→128.

Cette progression permet de savoir exactement à quelle étape l'erreur apparaît.

---

# 10. Séparation erreur spatiale / erreur temporelle — protocole cible

## Processus

Pour une erreur numérique globale, on peut conceptuellement écrire :

**erreur totale ≈ erreur spatiale + erreur temporelle + autres erreurs de modèle/référence**

L'objectif n'est pas de supposer que ces termes sont indépendants, mais de construire des expériences où l'un devient négligeable par rapport à l'autre.

## Solution expérimentale recommandée

### Étape A — fixer une solution exacte

Utiliser Couette périodique correctement implémenté ou une MMS.

### Étape B — fixer dt très petit

Choisir une valeur de dt suffisamment petite pour que l'erreur temporelle soit sous la tolérance visée.

### Étape C — raffiner dx

Mesurer :

- E32 ;
- E64 ;
- E128 ;
- éventuellement E256.

Calculer l'ordre :

p = log(E_N / E_2N) / log(2)

uniquement si :

- E_2N < E_N ;
- les deux calculs sont convergés ;
- la référence est identique ;
- le problème mathématique est identique.

### Étape D — vérifier la temporalité

À grille fixe, diminuer dt :

- dt ;
- dt/2 ;
- dt/4 ;
- éventuellement dt/8.

Si l'erreur converge selon la loi temporelle attendue, cela donne une estimation indépendante de la contribution temporelle.

### Étape E — comparer

Le régime recherché est :

**erreur temporelle << erreur spatiale**

pour mesurer l'ordre spatial.

Puis inversement :

**erreur spatiale << erreur temporelle**

pour mesurer l'ordre temporel.

---

# 11. Point important : le protocole A S159 n'est pas complet

Le fichier 003b ne traite pas symétriquement toutes les combinaisons.

Le code actuel exclut la grille 128×128 du protocole A en raison du coût avec dt=1e-4.

## Problème

Sans les trois niveaux de grille pour le même protocole, il n'est pas possible de calculer les deux ordres Richardson successifs :

- 32→64 ;
- 64→128.

Le test 64→128 devient donc non évaluable.

## Solution

Deux possibilités honnêtes :

1. exécuter 128×128 ;
2. modifier la campagne expérimentale pour employer une valeur de dt suffisamment grande mais scientifiquement justifiée.

La deuxième solution doit être validée par une étude de sensibilité temporelle ; elle ne doit pas être choisie uniquement pour réduire le temps de calcul.

**Un résultat absent doit rester NON ÉVALUABLE, pas PASS ni FAIL.**

---

# 12. T01b–T06b — statut à conserver

Les tests annoncés dans 003b sont :

- T01b : ordre 32→64 ≥ 1,5 ;
- T02b : ordre 64→128 ≥ 1,5 ;
- T03b : L2 strictement décroissant ;
- T04b : L∞128 < 0,05 ;
- T05b : u_max128 ≤ 1 + 1e-6 ;
- T06b : u_min128 ≥ −1e-6.

Tant que le problème Couette n'est pas résolu sans contamination interne et que la campagne complète n'est pas exécutée :

**T01b–T06b = NON ÉVALUABLES pour une certification Richardson finale.**

Il ne faut pas convertir un manque de données en FAIL artificiel.

---

# 13. Forensic Richardson — point secondaire mais toujours ouvert

Le fichier 003b produit encore un événement forensic par checkpoint et utilise `time_ns_get_absolute()` pour cet événement.

Le rapport 160 avait déjà établi que cette fonction repose sur CLOCK_REALTIME.

## Problème

Le contrat forensic unifié recherché dans les rapports précédents vise une séparation claire :

- horloge monotone pour ordre/durée ;
- horloge civile pour corrélation ;
- event_seq pour l'ordre exact.

La migration UNICITE-003 de S164 concerne UNIF-002/003/004, mais ne branche pas encore Richardson sur ce contrat partagé.

## Solution

Le futur Richardson forensic doit réutiliser le schéma unifié plutôt que créer un quatrième format spécifique.

Chantier :

**FORENSIC-UNIF-RICHARDSON = OPEN**

---

# 14. Continuité des chantiers antérieurs

Le changement de priorité vers Richardson ne doit pas effacer les chantiers précédents.

## P0

- Richardson-PROTOCOL-003c.
- FORENSIC-UNIF-RICHARDSON.
- Vérification complète de la séparation temporelle/spatiale.

## P1

- T04 renforcé : fenêtre complète, max-min, moyenne, écart-type, pente.
- BUILD-THREAD-001.
- PERF-FORENSIC-001.

## P2

- Robustesse quantitative Lyapunov.
- BUILD-PROOF-001.
- BUILD-PORT-002.
- ARCH-MEM-001.

## Continuité longue

- BL-003 → BL-012.
- C3 NS → NX-42.
- C4 NX-42 problèmes 6–30.
- FORENSIC Richardson/Lyapunov/NX-42.

Aucun de ces chantiers ne doit être considéré comme implicitement clos par les corrections UNICITE S162–S164.

---

# 15. Tableau de vérité global après S164

| Élément | État |
|---|---|
| HEAD S164 vérifié | PASS |
| UNICITE-003 S164 | PASS expérimental selon campagne documentée |
| Richardson-PROTOCOL-001 | **OPEN** |
| Richardson-PROTOCOL-002 stationnarité | mécanisme présent, preuve d'ordre spatial non acquise |
| Richardson-PROTOCOL-003 | **FAIL comme validation exacte** |
| Richardson-PROTOCOL-003b | **diagnostic utile, non certifié** |
| Richardson-PROTOCOL-003c | **OPEN P0** |
| Couette exact u=y | mathématiquement pertinent, implémentation solver encore à démontrer |
| CL internes Lid-Driven pendant step | **problème architectural confirmé** |
| Périodicité discrète u | **à démontrer/corriger** |
| Coordonnée MAC y=(j−0,5)dy | correction appliquée |
| Cause unique ancien Linf>1 | non démontrée |
| Stationnarité adaptative | présente |
| Séparation erreur spatiale/temporelle | non démontrée |
| Campagne A complète 32/64/128 | incomplète dans 003b |
| T01b–T06b | non évaluables pour certification finale |
| FORENSIC Richardson unifié | OPEN |
| BUILD-THREAD-001 | OPEN |
| PERF-FORENSIC-001 | OPEN |
| Lyapunov robuste | OPEN |
| CERTIFIED_100 | **false** |
| unique_human_proven | **false** |

---

# 16. Ordre d'exécution recommandé pour la prochaine session

### Étape 1 — fermer la définition discrète du problème Couette

Prouver les coordonnées et les relations de périodicité pour u, v et p.

### Étape 2 — supprimer la contamination Lid-Driven

Utiliser un callback de CL ou un chemin de step spécifiquement contrôlé.

### Étape 3 — test analytique à champ imposé

Vérifier que u=y, v=0, p=constante donne les résidus discrets attendus.

### Étape 4 — test un pas

Vérifier que l'état Couette ne dérive pas artificiellement après un pas.

### Étape 5 — stationnarité

Exécuter la convergence adaptative et publier toutes les métriques.

### Étape 6 — séparation temporelle

Faire varier dt à grille fixe.

### Étape 7 — raffinement spatial

Faire 32→64→128 avec dt suffisamment petit.

### Étape 8 — Richardson

Calculer les ordres seulement après validation des sept étapes précédentes.

### Étape 9 — forensic

Brancher la même convention LUM_ID/timestamps/event_seq que les campagnes UNIF récentes.

### Étape 10 — certification

Ne fermer Richardson-PROTOCOL-001 que si les résultats complets permettent réellement de distinguer :

- erreur spatiale ;
- erreur temporelle ;
- erreur de référence ;
- erreur de conditions limites.

---

# 17. Conclusion critique

La session S164 a correctement terminé son chantier UNICITE-003, mais elle n'a pas modifié la situation scientifique de Richardson.

Le point central reste inchangé :

**le solveur actuel ne permet pas encore de considérer 003b comme une expérience Couette exacte, car les conditions Lid-Driven sont appliquées à l'intérieur de `ns_solver_step()` avant d'être remplacées après coup.**

Un deuxième verrou subsiste :

**la périodicité de la composante u doit être démontrée par rapport à la convention MAC exacte du solveur, et pas seulement par une copie de valeurs de colonnes.**

Enfin, le protocole dt constant / dt∝dx / dt∝dx² reste utile comme expérience de sensibilité, mais il ne devient une preuve de séparation spatiale/temporelle qu'après fermeture des conditions limites, de la référence exacte et de la stationnarité.

Le prochain objectif scientifique doit donc être :

**Richardson-PROTOCOL-003c → problème Couette discrètement cohérent → stationnarité → étude temporelle → raffinement spatial → ordre Richardson.**

**Verdict : Richardson-PROTOCOL-001 = OPEN.**

**CERTIFIED_100=false.**

**unique_human_proven=false.**

**Aucun code source n'a été modifié pendant cet audit.**
