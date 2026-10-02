# RAPPORT 150 — AUDIT DE REPRISE POST-149, ÉTAT RÉEL ET ORDRE DE CONTINUATION

**Date :** 2026-10-02  
**Repository :** `vgacofc/lumvorax2`  
**Branche auditée :** `main`  
**HEAD distant :** `0404a51955499c18ae62eafbce38a48022ca20f2`  
**HEAD précédent :** `31e0f7bb70b208f2a1ec91f7d5d4be205aef2e08`  
**Rapport précédent :** `RAPPORT/149_CORRECTIONS_FL005_BL013_BL015_SERIALISATION_CANONIQUE_20261002.md`  
**Principe de cette session :** audit et documentation uniquement ; aucun code scientifique modifié par ce rapport.  
**CERTIFIED_100=false**  
**unique_human_proven=false**

---

## 1. Expertises activées

- Audit forensique Git/GitHub : HEAD, chronologie des commits, provenance des artefacts et cohérence rapport/code.
- Analyse statique C99 : pointeurs, durée de vie, mutex, sérialisation binaire et contrats d'API.
- Concurrence POSIX/pthreads : race use-after-close, cycle de vie du FILE*, validation ThreadSanitizer.
- Cryptographie appliquée : SHA-256, double-SHA256, vecteurs de référence et sensibilité des champs.
- Protocoles blockchain : définition du message de header, PoW, nonce, bits et déterminisme.
- Sérialisation canonique : endianness, padding ABI, représentation mémoire et interopérabilité.
- Ingénierie de tests : tests de régression, tests différentiels, vecteurs indépendants et tests adversariaux.
- CFD / mécanique des fluides numérique : solveur Navier–Stokes 2D, cavité entraînée, stabilité et convergence.
- Analyse numérique : Richardson, ordre apparent, erreurs spatiales/temporelles et critères de convergence.
- Dynamique non linéaire : estimation d'exposant de Lyapunov, dépendance aux paramètres et robustesse.
- Reproductibilité expérimentale : paramètres, logs, indépendance des vecteurs et répétabilité.
- Audit des stubs : distinction entre temps de calcul réellement mesuré et calcul scientifiquement représentatif.
- Audit documentaire : persistance des tâches, numérotation RAPPORT, statut OPEN/CLOSED et absence de sur-certification.

---

# 2. Processus de reprise

## 2.1 Comment l'état a été vérifié

Le dépôt distant `vgacofc/lumvorax2` a été interrogé directement sur `main`.

La chronologie récente confirme :

1. `31e0f7b` — rapport 148 ;
2. `0404a51` — session 149, corrections FL-005 / BL-013 / BL-015 ;
3. aucun commit postérieur au `0404a51` n'a été observé dans la liste récente consultée.

Le commit `0404a51955499c18ae62eafbce38a48022ca20f2` est donc le point de vérité distant utilisé pour le présent rapport.

### C'est-à-dire

Le bilan fourni pour la session 149 n'est pas seulement repris comme déclaration : les fichiers concernés ont été relus sur `main`.

---

# 3. Ce qui a réellement été produit depuis les anciens audits

## 3.1 Phase documentaire 125 → 131

Les rapports 125–131 ont progressivement transformé les affirmations historiques en état vérifiable.

Les points importants établis dans cette phase sont :

- C1 : les latences NX utilisent maintenant `CLOCK_MONOTONIC` ;
- C4 : les problèmes 6–30 restent explicitement des stubs mesurés ;
- le véritable solveur Navier–Stokes 2D a été ajouté ;
- une validation Ghia existe ;
- Richardson strict n'est pas démontré ;
- la monotonie énergétique T04 n'est pas démontrée ;
- la robustesse Lyapunov n'est pas démontrée ;
- l'intégration du solveur NS dans NX-42 reste distincte de la validation scientifique du solveur.

Le rapport 131 maintient explicitement ces chantiers ouverts.

## 3.2 Phase 132 → 140

La phase suivante a principalement traité la réconciliation documentaire, les conflits Git et une série d'anomalies techniques supplémentaires.

Le principe important conservé dans ces rapports est :

**un artefact exécuté n'est pas automatiquement une preuve scientifique de la propriété qu'il prétend mesurer.**

Exemple : mesurer réellement la durée d'exécution d'une boucle LCG prouve que la boucle a été exécutée et chronométrée ; cela ne prouve pas qu'elle résout la conjecture mathématique associée au nom du problème.

## 3.3 Phase 141 → 148

Les sessions 141–148 ont renforcé :

- la portabilité C ;
- les problèmes d'allocation alignée et de formatage entier ;
- le bridge et les chemins de logging ;
- le build portable isolé ;
- les vecteurs SHA-256 ;
- les audits de concurrence ;
- l'analyse FL-005 ;
- l'analyse BL-013 ;
- l'analyse BL-015 ;
- la traçabilité des rapports.

Le rapport 148 avait laissé FL-005, BL-013 et BL-015 ouverts.

---

# 4. Session 149 — ce qui est effectivement corrigé

## 4.1 FL-005 — durée de vie du FILE*

### Processus

Le logger possède un `FILE*` global protégé par `fl001_log_file_mutex`.

L'ancienne correction faisait :

1. verrouiller ;
2. copier le pointeur FILE* ;
3. déverrouiller ;
4. écrire ensuite.

### Problème

Le déverrouillage intervenait avant l'écriture.

Un autre thread pouvait donc appeler `forensic_logger_destroy()`, fermer le FILE* avec `fclose()`, puis le premier thread pouvait continuer à utiliser le pointeur déjà copié.

C'est un use-after-close : le pointeur existe encore comme valeur mémoire, mais l'objet FILE auquel il faisait référence n'est plus vivant.

### Correction observée sur main

La fonction `forensic_log_individual_lum()` :

1. prend `fl001_log_file_mutex` ;
2. vérifie `forensic_log_file` ;
3. écrit directement dans `forensic_log_file` ;
4. appelle `fflush()` ;
5. seulement ensuite libère le mutex.

### Conséquence

`forensic_logger_destroy()` utilise le même mutex avant son `fclose()`. Il ne peut donc pas fermer le FILE* au milieu de l'écriture protégée.

### Statut

**FL-005 : mécanisme corrigé et clôturé pour le scénario identifié.**

### Limite

La validation dynamique sous ThreadSanitizer reste ouverte.

C'est-à-dire : l'analyse du code montre que le chemin précis identifié est corrigé, mais une exécution concurrente instrumentée reste utile pour rechercher d'autres races qui ne seraient pas visibles par simple lecture.

---

# 5. BL-013 — nonce et bits enfin inclus dans le hash

## Processus

L'ancien `block_header_hash()` utilisait directement la représentation mémoire de la structure et ne hashait que 80 octets.

Or les offsets observés plaçaient :

- `version` à 0 ;
- `prev_hash` à 4 ;
- `merkle_root` à 36 ;
- `timestamp` à 68 ;
- `bits` à 76 ;
- `nonce` à 80.

La fenêtre de 80 octets ne contenait donc pas le nonce et ne représentait pas correctement le contrat voulu.

## Problème

Si le nonce ne participe pas au digest, changer le nonce ne change pas le hash.

C'est incompatible avec un mécanisme de recherche de PoW dans lequel le mineur fait varier le nonce pour chercher un digest satisfaisant une condition.

## Solution observée

Une fonction unique :

`block_header_serialize_canonical()`

construit explicitement 88 octets :

- 4 octets version LE ;
- 32 octets prev_hash ;
- 32 octets merkle_root ;
- 8 octets timestamp LE ;
- 4 octets bits LE ;
- 8 octets nonce LE.

Le hash devient ensuite :

double-SHA256(sérialisation canonique de 88 octets).

## Vérification indépendante

Le vecteur annoncé dans le rapport 149 a été recalculé indépendamment avec SHA-256 et donne :

`27b8b9304208721bed3ba89297dc593d1d9c6840883bc62933042a9b7e921db0`

pour le header de test décrit dans le rapport.

### Statut

**BL-013 : corrigé sur le chemin `block_header_hash()`.**

---

# 6. BL-015 — divergence genesis / block_header_hash

## Processus

Deux chemins calculaient précédemment le hash d'un header :

- `block_header_hash()` ;
- `lumvorax_genesis_compute_hash()`.

Ils n'utilisaient pas la même représentation.

## Problème

Deux fonctions qui prétendent calculer le hash du même header doivent utiliser exactement la même séquence d'octets.

Sinon :

- le hash dépend du chemin de calcul ;
- l'interopérabilité devient fragile ;
- deux composants peuvent calculer deux identifiants différents pour le même objet logique.

## Solution observée

`genesis.c` délègue maintenant à `block_header_serialize_canonical()`.

Le calcul genesis utilise donc la même sérialisation 88 octets que `block_header_hash()`.

### Statut

**BL-015 : corrigé sur le chemin audité.**

---

# 7. Tests blockchain — ce qui est réellement démontré

Le test distant contient notamment :

- T01 : SHA-256("abc") contre vecteur FIPS ;
- T02 : SHA-256("") contre vecteur FIPS ;
- T03 : double-SHA256("abc") contre vecteur indépendant ;
- T04 : hash du header canonique contre vecteur indépendant ;
- T05 : déterminisme ;
- T06 : changement de prev_hash ;
- T06b : changement de nonce ;
- T06c : changement de bits ;
- T07 : vecteur SHA-256 NIST supplémentaire.

Les tests T06b et T06c sont particulièrement importants pour BL-013.

Ils vérifient explicitement que :

- nonce différent → digest différent ;
- bits différent → digest différent.

Le rapport 149 annonce 11/11 PASS, ce qui correspond au compteur détaillé T01, T02, T03, T03b, T04, T04b, T05, T06, T06b, T06c et T07.

### Limite

Le test prouve la propriété de sérialisation et du hash sur les vecteurs testés.

Il ne prouve pas à lui seul :

- la conformité complète d'un protocole blockchain externe ;
- la sécurité cryptographique globale du système ;
- la correction de tous les chemins de mining ;
- la validité de toutes les fonctions de validation de bloc.

---

# 8. Nouveau contrôle important : le statut CI du commit 149

Le statut GitHub consulté pour `0404a51` contient un état `failure` associé au contexte **Vercel**.

Cela ne doit pas être interprété comme une preuve que le build C de la session 149 échoue.

Le rapport 149 contient ses propres résultats locaux :

- `make clean && make` : 0 erreur / 0 warning ;
- `make blockchain_test` : build réussi ;
- test blockchain : 11/11 PASS.

### Problème

Il existe une différence entre :

- une preuve de build/test C réalisée dans l'environnement de développement ;
- un statut externe GitHub/Vercel.

### Ordre

Le prochain audit doit identifier précisément ce que couvre le check Vercel et, séparément, fournir une preuve CI C reproductible.

C'est le chantier **BUILD-PROOF-001**.

---

# 9. Navier–Stokes — état réel actuel

## 9.1 Solveur

Le véritable fichier `src/solvers/ns_solver_2d.c` est présent sur `main`.

Il ne faut donc plus le confondre avec l'ancien stub NX-11.

## 9.2 Étude de convergence

Le fichier `src/validation/ns_convergence_study.c` exécute actuellement trois grilles :

- 32×32 ;
- 64×64 ;
- 128×128.

Il utilise :

- Re = 100 ;
- dt = 0,001 ;
- 20 000 pas ;
- comparaison au profil Ghia ;
- calcul d'une erreur L2 ;
- divergence maximale ;
- résidu Poisson ;
- énergie cinétique.

## Problème critique restant

Le code calcule bien un ordre apparent :

`p = log(e_coarse/e_fine) / log(2)`

mais le verdict T01/T02 ne dépend pas réellement de la valeur de cet ordre.

Le code définit T01 à partir d'une variation de L2 inférieure à 10 % et T02 à partir du résidu Poisson inférieur à 2e-5.

Ainsi, le texte du programme parle de Richardson et d'ordre théorique 1, mais le critère PASS ne démontre pas directement :

`p >= 0.8`

### Conséquence

Le calcul de l'ordre est informatif, mais il n'est pas encore un critère de preuve de l'ordre de convergence.

### Statut

**Richardson strict : OPEN.**

---

# 10. T04 — énergie : le nom du test reste trop fort

Le code calcule :

- énergie à 100 pas ;
- énergie finale à 3000 pas.

Mais le verdict est simplement :

`EK_final > 0 && EK_at_100 > 0`

Ce critère ne démontre pas une décroissance monotone.

De plus, dans une cavité entraînée, le couvercle fournit de l'énergie au système.

### C'est-à-dire

Dire « l'énergie décroît monotonement » suppose une grandeur précisément définie et une période où une décroissance est physiquement attendue.

Le test actuel ne vérifie ni :

`E(t+1) <= E(t)`

pour tous les pas,

ni une enveloppe monotone,

ni une dissipation nette correctement définie pour le problème forcé.

### Statut

**T04 monotonie : OPEN.**

---

# 11. Lyapunov — état réel actuel

Le fichier `src/validation/ns_lyapunov.c` est réel et exécutable.

Il utilise actuellement :

- warmup = 3000 pas ;
- renormalisation tous les 100 pas ;
- 50 renormalisations ;
- epsilon = 1e-4.

Le commentaire historique mentionne encore une perturbation initiale à 1e-6.

## Problème

Le protocole documenté et le protocole exécuté ne sont donc pas parfaitement alignés.

En outre, la robustesse d'un seul exposant Lyapunov n'est pas encore démontrée.

### Il faut tester

- epsilon ;
- warmup ;
- intervalle de renormalisation ;
- nombre de renormalisations ;
- résolution ;
- au minimum plusieurs répétitions ;
- idéalement une étude de sensibilité autour de Re=100.

### Point important

Le code distingue correctement maintenant :

- lambda <= 0 : STABLE ;
- 0 < lambda <= 0.01 : MARGINAL_CHAOS ;
- lambda > 0.01 : WEAKLY_CHAOTIC.

La valeur historique 0.0254219 est donc classée WEAKLY_CHAOTIC selon cette convention.

Mais cela ne suffit pas à prouver la robustesse scientifique de cette valeur.

### Statut

**Lyapunov : OPEN — robustesse et protocole reproductible à fermer.**

---

# 12. NX-42 — ce qui reste réellement un stub

Le fichier `src/tests/nx42_30_problems_execution_v2.c` confirme :

- C1 : timing avec `CLOCK_MONOTONIC` ;
- problèmes 1–5 : appel à `nx11_physics_stub()` ;
- problèmes 6–30 : boucle LCG minimale ;
- statut explicite `STUB_MEASURED`.

Le code précise lui-même que les problèmes 6–30 sont des mesures de stub.

### Conclusion

C1 est une correction de métrologie.

Elle ne transforme pas les problèmes 1–30 en solveurs scientifiques.

C4 reste donc OPEN.

---

# 13. V138 — anomalie documentaire encore présente

Le fichier courant `nx47_vesu_kernel_v138.py` contient toujours des marqueurs de conflit Git.

Le contrôle direct du contenu courant détecte :

- `<<<<<<<` ;
- `=======` ;
- `>>>>>>>`.

### Problème

Un fichier Python contenant des marqueurs de conflit non résolus n'est pas une version documentaire ou logicielle proprement réconciliée.

### Statut

**V138 : OPEN.**

La correction doit être faite par réconciliation du contenu, pas par suppression aveugle des marqueurs.

---

# 14. BUILD-PORT-002

Le Makefile possède bien une cible portable séparée.

La cible utilise :

- `build/obj/portable/` ;
- `-march=x86-64` sous Linux ;
- des objets distincts des objets natifs.

### Ce qui est démontré

La séparation des objets de compilation est présente.

### Ce qui n'est pas encore démontré

Il manque une exécution indépendante sur une machine réellement dépourvue des extensions ISA utilisées par la build native.

### Statut

**BUILD-PORT-002 : OPEN.**

---

# 15. BUILD-THREAD-001

Le correctif FL-005 est lisible et cohérent avec le problème identifié.

Mais la preuve recommandée reste :

- build avec ThreadSanitizer ;
- exécution concurrente ;
- destruction concurrente du logger ;
- plusieurs threads d'écriture ;
- vérification d'absence de race reportée.

### Statut

**BUILD-THREAD-001 : OPEN.**

---

# 16. BUILD-PROOF-001

Le build principal est décrit comme réussi localement.

Cependant, une preuve reproductible indépendante manque encore.

### Objectif

Créer une validation CI qui exécute au minimum :

- build C complet ;
- build blockchain ;
- tests blockchain ;
- build portable ;
- tests pertinents ;
- publication des logs.

### Statut

**BUILD-PROOF-001 : OPEN.**

---

# 17. Registre persistant après session 149

Les anomalies suivantes restent à conserver :

### Concurrence / logging

- FL-001 ;
- BUILD-THREAD-001 ;
- LL-002 ;
- LL-004 ;
- LL-005.

### Blockchain

- BL-003 → BL-012 à réexaminer selon leur statut exact ;
- BUILD-PROOF-001 ;
- BUILD-PORT-002.

### Scientifique

- NS-001 → NS-012 ;
- Richardson strict ;
- T04 monotonie ;
- robustesse Lyapunov ;
- C3 intégration NS dans NX-42 ;
- C4 problèmes 6–30.

### Autres registres

- NQubit ;
- ART-CT003 ;
- IBM-001 / IBM-002 ;
- TT-001 / TT-002.

Aucune de ces tâches ne doit être considérée comme abandonnée simplement parce que FL-005, BL-013 et BL-015 ont été clôturées.

---

# 18. ORDRE DE CONTINUATION RECOMMANDÉ

## Ordre 1 — BUILD-THREAD-001

### Processus

Transformer la correction FL-005 en preuve dynamique.

### Action

Construire une campagne TSan concurrente ciblée sur :

- `forensic_log_individual_lum()` ;
- `forensic_logger_destroy()` ;
- `forensic_log()` ;
- `unified_forensic_log()`.

### Critère de fermeture

- compilation TSan réussie ;
- scénario concurrent répété ;
- aucune race signalée sur le chemin testé ;
- log de test conservé.

---

## Ordre 2 — BUILD-PROOF-001

### Processus

Transformer les résultats locaux en preuve reproductible.

### Action

Mettre en place une CI C indépendante du poste de développement.

### Critère de fermeture

La CI doit publier :

- build complet ;
- blockchain_test ;
- résultats ;
- warnings ;
- erreurs ;
- environnement.

---

## Ordre 3 — BUILD-PORT-002

### Processus

Vérifier que le binaire portable fonctionne réellement sur une cible sans les extensions ISA natives.

### Action

Tester au minimum :

- compilation native ;
- compilation portable ;
- exécution portable ;
- comparaison des résultats déterministes.

### Critère de fermeture

Aucune dépendance accidentelle aux objets natifs et aucune instruction ISA interdite sur la cible de test.

---

## Ordre 4 — Blockchain : audit BL-003 → BL-012

### Processus

Les corrections BL-013/BL-015 ne doivent pas masquer les anomalies blockchain historiques.

### Action

Reprendre chaque ID BL-003 à BL-012 individuellement.

Pour chaque ID :

1. retrouver le problème ;
2. retrouver le code concerné ;
3. vérifier son statut actuel ;
4. produire une preuve ;
5. clôturer uniquement si la preuve existe.

---

## Ordre 5 — Richardson strict

### Processus

Faire du calcul d'ordre une condition réelle du test.

### Action

Ne pas seulement afficher `p`.

Le verdict doit vérifier explicitement les conditions scientifiques définies :

- trois grilles ;
- même temps physique ;
- erreur définie sans ambiguïté ;
- ordre apparent ;
- seuil explicite ;
- comportement cohérent lors du raffinement ;
- séparation erreur spatiale / erreur temporelle.

### Critère de fermeture

Un résultat qui démontre réellement le comportement annoncé, ou un rapport qui conclut honnêtement que le solveur n'atteint pas encore le critère.

---

## Ordre 6 — T04

### Processus

Redéfinir précisément la grandeur énergétique.

### Action

Déterminer si le test porte sur :

- énergie totale ;
- dissipation ;
- variation d'énergie après arrêt du forçage ;
- régime transitoire ;
- régime stationnaire.

Puis construire un test adapté.

### Critère de fermeture

Le nom « monotonie » doit correspondre exactement au calcul exécuté.

---

## Ordre 7 — Lyapunov

### Processus

Passer d'une valeur unique à une étude de robustesse.

### Action minimale

Répéter l'estimation avec plusieurs :

- epsilon ;
- warmup ;
- intervalles de renormalisation ;
- nombres de renormalisations.

Documenter toutes les valeurs et leurs dispersions.

### Critère de fermeture

Une conclusion sur lambda doit être liée à une procédure reproductible et à une analyse de sensibilité.

---

## Ordre 8 — C3 : intégration NS dans NX-42

### Processus

Séparer trois niveaux :

1. solveur NS réel ;
2. validation scientifique du solveur ;
3. intégration du solveur dans NX-42.

### Action

Ne pas déclarer C3 fermée tant que les trois niveaux ne sont pas démontrés séparément.

---

## Ordre 9 — C4 : problèmes 6–30

### Processus

Remplacer progressivement les stubs par des implémentations spécifiques, ou maintenir explicitement leur statut STUB.

### Règle

Une mesure de temps réelle ne doit jamais être utilisée comme preuve de résolution mathématique.

---

## Ordre 10 — V138

### Processus

Réconcilier le fichier Python.

### Action

Identifier les deux versions concurrentes, déterminer les différences fonctionnelles, choisir la version cohérente et supprimer les marqueurs de conflit uniquement après réconciliation.

### Critère de fermeture

Aucun marqueur de conflit dans le fichier et exécution/reproductibilité du pipeline concerné.

---

# 19. Priorité opérationnelle consolidée

L'ordre de travail suivant est retenu pour la prochaine session documentaire :

**150 → BUILD-THREAD-001 → BUILD-PROOF-001 → BUILD-PORT-002 → BL-003→BL-012 → Richardson → T04 → Lyapunov → C3 → C4 → V138 → reste du registre historique.**

Cet ordre n'implique pas que les tâches scientifiques secondaires soient supprimées : elles restent persistantes dans le registre.

---

# 20. Point de vérité final

À la date du présent audit :

### Confirmé

- HEAD distant = `0404a51955499c18ae62eafbce38a48022ca20f2`.
- Rapport 149 présent.
- FL-005 corrigé sur le chemin identifié.
- BL-013 corrigé sur `block_header_hash()`.
- BL-015 corrigé par utilisation de la sérialisation canonique commune.
- sérialisation canonique = 88 octets LE.
- vecteur canonique indépendant vérifié.
- T06b et T06c présents pour vérifier nonce et bits.
- solveur NS 2D réel présent.
- C1 utilise réellement `CLOCK_MONOTONIC`.
- problèmes 6–30 restent explicitement STUB_MEASURED.
- V138 contient encore des marqueurs de conflit.

### Non démontré

- absence de toutes les races concurrentes ;
- CI C indépendante ;
- portabilité réelle sur cible sans ISA native ;
- ordre Richardson conforme à un critère scientifique strict ;
- monotonie énergétique T04 ;
- robustesse Lyapunov ;
- intégration scientifique complète NS → NX-42 ;
- résolution scientifique des problèmes NX-42 6–30.

---

# 21. Conclusion

La session 149 a effectivement fermé trois anomalies P1 importantes :

- **FL-005** : durée de vie FILE* ;
- **BL-013** : nonce/bits absents du digest ;
- **BL-015** : divergence de sérialisation genesis/header.

La correction blockchain est particulièrement structurante : la sérialisation canonique devient le point commun entre les chemins qui calculent le hash du header.

Mais la clôture de ces trois anomalies ne constitue pas une certification globale du dépôt.

Le dépôt doit maintenant passer de la phase de correction locale à une phase de **preuve reproductible** :

1. preuve dynamique de concurrence ;
2. preuve CI ;
3. preuve de portabilité ;
4. fermeture méthodique du registre blockchain ;
5. preuve de convergence ;
6. définition correcte du test énergétique ;
7. robustesse Lyapunov ;
8. intégration NS/NX ;
9. remplacement ou maintien explicite des stubs ;
10. réconciliation V138.

**CERTIFIED_100 reste false.**

Aucune modification du code scientifique n'est requise ou effectuée par le présent rapport.
