# Rapport 189 — Audit de continuité S181 et reprise FU002 → Richardson / LUM-VORAX

**Date :** 2026-10-04  
**Dépôt :** vgacofc/lumvorax2  
**Branche :** main  
**Dernier commit vérifié :** c5f2e4a11eb40ea3ead76855df8a096f654b99dc  
**Règle d'action :** rapport documentaire uniquement ; aucun code scientifique modifié par cet audit.  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

## Expertises activées

- Audit forensique Git/GitHub et provenance commit → source → exécution → log → rapport
- Analyse statique C99/C11
- Concurrence POSIX/pthreads et ThreadSanitizer
- Métrologie des horloges et qualification unité/résolution/précision
- Forensic bit-level / IEEE-754 / provenance des valeurs
- Architecture LUM/VORAX et traçabilité bit → LUM → transformation → résultat
- CFD / Navier–Stokes 2D / Chorin
- Richardson / MMS / convergence spatiale et temporelle
- Analyse de stationnarité T04
- Dynamique non linéaire / robustesse Lyapunov
- Blockchain / SHA-256 / sérialisation canonique
- CI reproductible / portabilité ISA
- Audit des stubs NX-42
- Audit documentaire et persistance du registre historique

# 1. Synchronisation Git — point de vérité

Le dépôt distant confirme la présence de S181.

Le commit final vérifié est :

**c5f2e4a11eb40ea3ead76855df8a096f654b99dc**

Ce commit corrige le dernier champ documentaire qui restait temporaire : `git_head_after` dans le log v34 et le rapport 188. Le commit S181 référencé devient explicitement :

**27be3820a06e1463b7b480ef337189fdae099531**

La chaîne documentaire rapport → log → commit est donc cohérente sur ce point.

## Processus — c'est-à-dire

Un audit forensique doit permettre de partir du rapport, retrouver le log brut, puis retrouver exactement le commit qui correspond à l'état audité.

## Problème

Un champ `TBD` cassait cette chaîne même si le code et les tests étaient corrects.

## Solution

S181 ferme cette anomalie documentaire. Elle doit rester considérée comme une correction de traçabilité, pas comme une preuve scientifique supplémentaire.

# 2. S181 — ce qui est réellement fermé

Le rapport 188 confirme :

- TSan `log_event()` : 0 data race sur 8 threads et 100 000 événements ;
- TSan `check_continuity()` : 0 data race ;
- monotonie mono-thread et concurrente : 0 violation ;
- résolution effective mesurée sur macOS : environ 1 µs ;
- coût `CLOCK_MONOTONIC` : environ 125 ns ;
- environ 88,9 % des appels consécutifs `CLOCK_MONOTONIC` ont un delta nul ;
- `CERTIFIED_100` reste false.

## Processus

TSan recherche les accès mémoire concurrents incompatibles. La qualification d'horloge mesure séparément le comportement réel de l'instrument temporel.

## Problème

0 data race ne signifie pas automatiquement que la sémantique applicative concurrente est correcte.

De même, une valeur stockée en nanosecondes ne signifie pas que l'horloge distingue réellement deux événements séparés de moins d'une microseconde.

## Solution

Conserver les deux niveaux séparés :

**sécurité mémoire ≠ correction sémantique**,  
**unité nanoseconde ≠ résolution nanoseconde**.

La limite métrologique S181 doit donc être conservée dans toute future certification LUM/VORAX.

# 3. check_continuity() — point à ne pas considérer comme scientifiquement fermé

Le test S181 montre 0 data race, mais le rapport documente explicitement une situation TOCTOU sous usage concurrent.

## Processus

La fonction lit l'état précédent sous mutex, libère le mutex, prend sa décision, puis reprend le mutex pour mettre à jour l'état.

## Problème

Deux producteurs peuvent raisonner à partir du même ancien `last_seq`. Le résultat peut produire un LOSS apparent alors qu'il n'y a pas de data race.

## Solution

La règle d'usage actuelle doit rester :

**check_continuity() mono-flux par session**, ou coordination applicative explicite pour plusieurs producteurs.

Ne pas transformer « TSan PASS » en « machine d'état concurrente universellement prouvée ».

# 4. LUM/VORAX — rien ne doit être considéré comme oublié

Le pipeline LUM/VORAX reste un chantier transversal.

La couverture actuelle doit être distinguée en quatre niveaux :

1. **présence du module** ;
2. **exécution réelle** ;
3. **validation indépendante du résultat** ;
4. **provenance bit-level complète**.

Le niveau 4 n'est pas encore fermé.

## Ce qui reste à démontrer

Pour une donnée numérique entrant dans LumVorax, la chaîne cible est :

**bit d'entrée → BIT_ID → LUM_ID → transformation → valeur de sortie → résultat scientifique**

Pour chaque transformation pertinente, il faut pouvoir retrouver :

- valeur avant ;
- valeur après ;
- identifiant parent ;
- identifiant enfant ou version ;
- opération ;
- module ;
- thread ;
- run_id ;
- timestamp monotone ;
- hash/payload si nécessaire ;
- résultat final auquel l'événement contribue.

C'est la vraie définition opérationnelle de **FORENSIC-UNIF-002 universel**.

## Point critique

Le rapport 186 avait déjà établi que le câblage LUM/VORAX et l'exécution de modules ne suffisent pas à démontrer une provenance bit-à-bit universelle. S181 améliore la sécurité concurrente et la métrologie, mais ne ferme pas cette preuve.

# 5. Richardson-PROTOCOL-003 — reprendre maintenant le chantier scientifique

Le dépôt contient bien les artefacts Richardson-PROTOCOL-003 et les rapports historiques associés.

L'objectif reste :

**solution exacte/manufacturée → séparation erreur spatiale / erreur temporelle → ordre observé réellement démontré.**

## Processus

Une solution manufacturée fournit une solution connue. On compare alors le résultat numérique à cette solution connue, au lieu de dépendre uniquement de Ghia.

Pour le temporel, la grille doit rester identique et le pas temporel varier.

## Problème

Les anciens résultats Ghia ont montré que l'erreur pouvait être contaminée par :

- régime transitoire ;
- erreur temporelle ;
- erreur spatiale ;
- erreur de référence.

Les expériences antérieures sur Couette ont également produit des diagnostics utiles mais n'ont pas encore constitué une fermeture définitive du protocole.

## Solution

Reprendre avec une matrice expérimentale explicitement séparée :

- même problème ;
- même grille ;
- même temps physique ;
- `dt` ;
- `dt/2` ;
- `dt/4` ;
- solution exacte/manufacturée ;
- L2 ;
- L∞ ;
- résidu Poisson ;
- divergence ;
- état stationnaire ;
- ordre observé.

Le but n'est pas de « faire passer Richardson », mais de démontrer quelle erreur domine.

# 6. Ordre temporel du Chorin complet

L'expérience Euler scalaire a montré un ordre temporel proche de 1. Cela ne certifie pas automatiquement le solveur Chorin complet.

## C'est-à-dire

Euler est seulement une étape du pipeline :

advection → diffusion → vitesse intermédiaire → Poisson → correction de pression → conditions aux limites.

Chaque étape peut modifier l'ordre global.

## Prochaine preuve

Faire l'expérience temporelle sur le solveur complet :

**dt → dt/2 → dt/4**

à grille constante, même temps physique final et état stationnaire comparable.

Le résultat attendu doit être une mesure, pas une hypothèse.

# 7. T04 — ne pas perdre le chantier

T04 est actuellement PASS selon son critère défini, mais cela ne constitue pas encore une preuve générale de stationnarité asymptotique.

La prochaine fermeture doit analyser toute la fenêtre finale :

- minimum ;
- maximum ;
- moyenne ;
- écart-type ;
- pente ;
- variation relative maximale ;
- norme de dérivée discrète ;
- divergence ;
- résidu Poisson.

Deux points finaux proches ne suffisent pas à exclure une oscillation entre eux.

# 8. Lyapunov — robustesse quantitative toujours ouverte

Le signe négatif de lambda est intéressant, mais il faut conserver la distinction :

**robustesse du signe** ≠ **robustesse quantitative de lambda**.

Le sweep doit être réellement exécuté sur plusieurs epsilon, puis répété avec variation du warmup, intervalle de renormalisation, nombre de renormalisations et résolution.

Aucune certification globale ne doit être déclarée sur la seule valeur `lambda = -1,426073`.

# 9. BUILD-THREAD-001

S181 ferme la validation TSan de FU002 pour le scénario testé.

Mais le registre historique ne doit pas être effacé : BUILD-THREAD-001 reste une famille de validation plus large.

Il faut conserver la distinction entre :

- TSan de `parallel_processor` déjà fermé ;
- TSan de FU002 S181 fermé pour le scénario testé ;
- couverture concurrente globale du projet, qui reste à démontrer.

# 10. BUILD-PROOF-001

Toujours ouvert.

Il faut une CI C indépendante capable de reproduire :

- compilation ;
- tests ;
- exit codes ;
- logs ;
- artefacts ;
- environnement ;
- résultats.

Un rapport écrit décrivant une exécution n'est pas équivalent à l'exécution reproductible elle-même.

# 11. BUILD-PORT-002

Toujours ouvert.

Le fait qu'une machine actuelle possède une extension ISA donnée ne prouve pas que le chemin portable fonctionne sur une cible dépourvue de cette extension.

La validation doit être exécutée sur une cible réellement appropriée.

# 12. Blockchain — registre historique conservé

Les corrections FL-005, BL-013 et BL-015 restent documentées comme fermées.

Les chantiers historiques **BL-003 → BL-012** doivent cependant rester dans le registre jusqu'à leur propre fermeture indépendante.

Aucun PASS blockchain ne doit être utilisé pour certifier les parties CFD, LUM/VORAX ou NX-42.

# 13. NX-42 — C3/C4 restent actifs

Il faut maintenir la distinction fondamentale :

**temps mesuré correctement ≠ problème scientifique réellement résolu.**

C4 reste ouvert pour les problèmes 6–30 tant qu'ils sont `STUB_MEASURED`.

C3 reste ouvert tant que l'intégration du solveur NS dans NX-42 n'est pas démontrée avec ses propres validations.

# 14. Registre persistant — aucun chantier antérieur abandonné

Après S181, les chantiers suivants restent actifs :

1. **FORENSIC-UNIF-002 universel** — provenance bit → transformation → résultat ;
2. **Richardson-PROTOCOL-003** — solution manufacturée ;
3. **ordre temporel Chorin complet** — dt/dt/2/dt/4 ;
4. **T04 renforcé** — fenêtre finale complète ;
5. **Lyapunov robuste** — sweep réel ;
6. **BUILD-THREAD-001** — couverture concurrente restante ;
7. **BUILD-PROOF-001** — CI C reproductible ;
8. **BUILD-PORT-002** — cible réellement portable ;
9. **BL-003 → BL-012** ;
10. **C3 NS → NX-42** ;
11. **C4 NX-42 6–30** ;
12. tout chantier documentaire ou forensique encore ouvert.

# 15. Ordre de continuation recommandé

### P0 — LUM/VORAX forensic universel

Terminer la provenance réelle des valeurs et des transformations.

### P1 — Richardson-PROTOCOL-003

Passer de la référence Ghia à une solution manufacturée/exacte.

### P2 — Chorin temporel complet

Démontrer expérimentalement l'ordre temporel global.

### P3 — T04 + Lyapunov

Renforcer les preuves de stationnarité et de robustesse dynamique.

### P4 — BUILD

CI C → portabilité → répétabilité inter-environnement.

### P5 — Blockchain historique

BL-003 → BL-012.

### P6 — NX-42

C3 puis C4.

Cette priorité ne supprime aucun chantier antérieur ; elle définit seulement l'ordre de progression.

# 16. Critère de certification globale

Le statut reste :

**CERTIFIED_100=false**  
**unique_human_proven=false**

La certification globale ne doit être activée que lorsque les preuves indépendantes couvrent simultanément :

- code ;
- tests ;
- concurrence ;
- portabilité ;
- provenance bit-level ;
- métrologie temporelle ;
- convergence spatiale ;
- convergence temporelle ;
- stationnarité ;
- Lyapunov ;
- blockchain ;
- NX-42 ;
- CI reproductible ;
- logs bruts ;
- correspondance exacte commit → source → exécution → résultat → rapport.

# Conclusion

S181 ferme réellement quatre points : la métadonnée `git_head_after`, le test TSan de `log_event()`, la qualification TSan de `check_continuity()` au niveau data-race, et la qualification métrologique des horloges.

La limite importante est maintenant explicitement connue : sur la plateforme testée, l'horloge est exprimée en nanosecondes mais sa résolution effective est d'environ **1 µs**. Cela interdit de présenter deux événements espacés de moins d'une microseconde comme temporellement distinguables avec certitude.

Le chantier LUM/VORAX n'est donc pas abandonné : au contraire, S181 renforce sa base forensique. La prochaine étape doit raccorder cette instrumentation à la **valeur réelle** des données et à leur transformation, puis reprendre Richardson-PROTOCOL-003 avec une solution exacte/manufacturée et enfin l'étude temporelle du Chorin complet.

**Aucun code scientifique n'a été modifié dans cette reprise. Seul le présent rapport documentaire est destiné à être ajouté à la série RAPPORT/.**
