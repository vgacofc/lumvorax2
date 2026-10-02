# Rapport 158 — Audit critique S157 : FORENSIC-UNIF-002 + Richardson-PROTOCOL-003

Date : 2026-10-03
HEAD vérifié : 5d3fd542bb229c9aabd42232466ee9e6c5cd3e01
HEAD précédent : 2ec3a85eb9517ddcf1128e5c3c7c207f7fb96059
CERTIFIED_100=false
unique_human_proven=false
Mode DEBUG actif

## 1. Expertises activées

- Audit forensique Git/GitHub
- Analyse statique C99, ABI et types entiers
- Traçabilité bit-level et provenance des événements
- Métrologie des horloges et sémantique timestamp
- CFD / Navier–Stokes 2D
- Méthode des différences finies et grille MAC/staggered
- Conditions aux limites Lid-Driven / Couette
- Analyse Richardson et ordre de convergence
- Méthodes exactes / manufactured solutions
- Analyse numérique L1/L2/Linf
- Concurrence pthread / mutex
- Performance I/O
- Architecture mémoire / cache
- Continuité des chantiers

## 2. Synchronisation GitHub

Le dépôt distant confirme que S157 est exactement un commit après le rapport 156.

Comparaison : base 2ec3a85 → HEAD 5d3fd54, avance de 1 commit. Les fichiers concernés sont le Makefile, le rapport 157, le JSON S157, ns_forensic_unif2.c et ns_richardson_manufactured.c.

Aucun code source n'a été modifié pendant cet audit.

## 3. FORENSIC-UNIF-002

### Processus

S157 extrait les 64 bits IEEE-754 d'un double, calcule la valeur de chaque bit, construit un LUM_ID contenant notamment run_id, protocole, module, step et position du bit, puis journalise les événements.

### Problème — BUG-1 confirmé

ns_forensic_unif2.c appelle forensic_log_individual_lum avec une conversion explicite vers uint32_t alors que le protocole construit un LUM_ID sur 64 bits.

Conséquence : les champs de poids fort du LUM_ID sont perdus dans le logger. La couverture interne peut compter 49 280 bits, mais le fichier externe ne conserve pas l'identité complète de ces 49 280 événements.

Le chiffre rapporté de 640 IDs distincts pour 49 280 événements est donc cohérent avec la troncature observée.

Verdict : BUG-1 CONFIRMÉ, priorité P0.

### Solution

Le logger doit accepter et conserver un uint64_t jusqu'au format final. Il faut ensuite vérifier automatiquement unicité, duplication et perte sur le fichier produit.

## 4. Timestamps

### Processus

Le test utilise time_ns_get_absolute(), qui repose sur CLOCK_REALTIME. Il s'agit bien d'une horloge système réelle.

### Problème A — timestamp partagé

Le timestamp est obtenu au niveau du pas puis réutilisé pour de nombreux bits. Le rapport S157 observe 20 timestamps uniques pour 49 280 événements.

Verdict : horloge réelle PASS ; timestamp individuel par événement FAIL.

### Problème B — deux sémantiques d'horloge

lum_get_timestamp() essaie d'abord CLOCK_MONOTONIC puis CLOCK_REALTIME, tandis que le test fournit des timestamps CLOCK_REALTIME. Le header du logger peut donc être issu d'une horloge différente de celle des événements.

Verdict : BUG-3 CONFIRMÉ, priorité P1.

Solution : séparer explicitement horloge monotone pour ordre/durée et horloge civile pour corrélation externe, avec un compteur séquentiel global pour l'ordre exact.

## 5. Richardson-PROTOCOL-003

### Processus

S157 compare le champ numérique à u_exact(x,y)=y et tente de séparer les effets spatiaux et temporels avec trois lois de dt.

### Problème principal

set_couette_bc() appelle directement ns_solver_set_lid_bc(). Cette fonction impose des parois latérales no-slip, une paroi Sud no-slip et un couvercle Nord mobile, avec pression à Neumann.

Le commentaire du test disant que cette condition est identique à Couette plan est trop fort. Une solution analytique u=y n'est une référence valide que si elle satisfait réellement le problème mathématique imposé, y compris les conditions aux limites.

Verdict : Richardson-PROTOCOL-003 n'est pas une validation exacte Couette démontrée dans sa forme actuelle.

### BUG-2 — coordonnée staggered

Le code parcourt les valeurs physiques u[i][j] pour j=1..ny, avec u[i][0] et u[i][ny+1] utilisés comme couches de bord. Le code de mesure utilise y_node=j*dy alors que sa propre documentation décrit une grille décalée.

La convention MAC doit donc être vérifiée explicitement. Avec la convention décrite, la position intérieure attendue est à tester comme (j-0.5)*dy.

Verdict : BUG-2 / anomalie de coordonnée CONFIRMÉE.

Attention : le rapport 157 affirme que cette anomalie explique à elle seule Linf > 1. Cette causalité n'est pas démontrée. Il faut d'abord mesurer min(u) et max(u). Si le solveur produit un overshoot u>1 ou un undershoot u<0, une erreur supérieure à 1 n'est pas mathématiquement impossible.

## 6. L1/L2/Linf et ordre Richardson

Les valeurs L1 annoncées suivent presque exactement (N+1)/(2N). Ce motif montre que la mesure compare une solution numérique et une référence qui ne sont pas encore démontrées comme appartenant au même problème.

Les tests T01, T02, T03 et T04 doivent donc rester FAIL. Ils ne doivent pas être transformés en PASS par une correction interprétative.

Avant tout nouvel ordre Richardson : vérifier les conditions aux limites, les coordonnées MAC, min/max des champs, le résidu PDE et la satisfaction analytique de la solution de référence.

## 7. BUG-4 mutex

Le rapport 157 donne un déséquilibre lexical entre occurrences lock et unlock dans plusieurs fichiers.

Ce comptage est un signal d'audit, mais il ne prouve pas à lui seul une race condition. Les branches conditionnelles et retours anticipés doivent être analysés chemin par chemin.

Verdict : suspicion P1, race condition NON DÉMONTRÉE.

BUILD-THREAD-001 reste nécessaire avec un outil de détection de concurrence adapté à la plateforme.

## 8. PERF-1 fflush

Le logger effectue un fflush après chaque événement individuel. Le coût de ce mécanisme est réel comme risque de performance.

En revanche, une valeur fixe comme 14 % du wall time dépend du matériel, du système de fichiers et du buffering. Elle ne doit pas être présentée comme une constante universelle.

Solution : benchmarker flush par événement, 256, 1024 et 4096 événements, puis mesurer wall time, CPU time, appels système et comportement en cas d'arrêt brutal.

## 9. Optimisations mémoire

Le pool LUM n'est pas une fonctionnalité entièrement nouvelle à créer : lum_core.c contient déjà un pool global de 1M LUM et un pool thread-local de 1024 LUM, avec allocation alignée.

De même, lum_group_create() possède déjà un chemin Linux mmap/huge pages et un fallback posix_memalign.

Les travaux réellement ouverts sont donc surtout le benchmark de ces mécanismes, le ring buffer forensic, le batch flush et la mesure de tout gain de cache/prefetch.

Il faut éviter de remplacer une optimisation existante par une nouvelle architecture sans mesure comparative.

## 10. Ordre de travail corrigé

### P0 — FORENSIC-UNIF-003

Conserver le LUM_ID 64 bits sans troncature ; ajouter event_seq ; conserver run_id, protocol_id, module_id, step et bit_pos ; vérifier duplication/perte ; séparer timestamp monotone et timestamp civil ; ajouter une vérification de campagne.

### P0 — Richardson-PROTOCOL-003b

Choisir soit de vraies conditions Couette cohérentes, soit une MMS avec terme source. Vérifier analytiquement que la référence satisfait le problème effectivement résolu.

### P1 — BUILD-THREAD-001

Analyser les chemins lock/unlock et effectuer un test multi-thread instrumenté.

### P1 — PERF-FORENSIC-001

Mesurer plusieurs politiques de flush avant de fixer une valeur définitive.

### P2 — ARCH-MEM-001

Benchmark du pool existant, du TLP, des allocations alignées et des stratégies mmap avant toute nouvelle modification.

### Continuité

Maintenir BUILD-PROOF-001, BUILD-PORT-002, BL-003→BL-012, C3 NS→NX-42, C4 NX-42 problèmes 6–30, Lyapunov robuste, T04 renforcé et FORENSIC Richardson/Lyapunov/NX-42.

## 11. État de vérité

| Élément | État |
|---|---|
| Commit S157 présent sur main | PASS |
| 49 280 événements internes annoncés | PASS rapporté |
| Valeur réelle des bits calculée | PASS |
| Identité 64 bits conservée dans le log | FAIL — BUG-1 |
| Horloge système réelle utilisée | PASS |
| Timestamp individuel par bit | FAIL |
| Cohérence d'horloge header/events | FAIL — BUG-3 |
| Couverture de 7 modules | PASS rapporté |
| Référence analytique Couette démontrée | FAIL |
| Coordonnée staggered de référence | BUG-2 confirmé |
| Cause exacte de Linf > 1 | NON DÉMONTRÉE |
| Race condition mutex | NON DÉMONTRÉE |
| fflush par événement comme risque de performance | CONFIRMÉ |
| Pool LUM | Déjà partiellement implémenté |
| Ring buffer forensic | OPEN |
| CERTIFIED_100 | false |
| unique_human_proven | false |

## 12. Conclusion

S157 représente une progression technique réelle : les valeurs binaires sont maintenant calculées et la campagne couvre davantage de modules.

Mais la formulation 100 % bit-level est trop large : la couverture interne est PASS, tandis que la provenance externe reste FAIL à cause de la troncature 64→32.

Le verrou scientifique Richardson est également précis : la validation appelle encore les conditions aux limites Lid-Driven du solveur alors qu'elle utilise u=y comme référence Couette.

Le diagnostic staggered y_node est une anomalie réelle, mais son rôle exact dans Linf > 1 doit être démontré par mesure de min/max et vérification de la convention géométrique.

Verdict global : progrès confirmé ; certification forensic complète et validation Richardson restent ouvertes.

CERTIFIED_100=false reste inchangé.