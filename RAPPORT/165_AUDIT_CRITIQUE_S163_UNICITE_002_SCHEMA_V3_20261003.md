# Rapport 165 — AUDIT CRITIQUE S163 / UNICITE-002 : portée réelle du schéma LUM_ID v3

**Date :** 2026-10-03  
**Session audit :** S163 → revue post-commit `3b7478bfb4418a77f500e2fcc38b2e0855b2eabf`  
**HEAD distant vérifié :** `3b7478bfb4418a77f500e2fcc38b2e0855b2eabf`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode : audit critique, sans modification du code source**

---

## 1. Expertises activées

- **Audit forensique des identifiants LUM_ID 64 bits**
- **Analyse d’injectivité et de collisions**
- **Audit des compteurs de session et concurrence mémoire**
- **Analyse de schéma binaire / allocation de champs**
- **Audit des hash sets et sentinelles**
- **Validation de campagnes de traçabilité bit-level**
- **Audit de cohérence entre documentation, implémentation et résultats**
- **Analyse de portée des preuves expérimentales**
- **Audit de non-régression UNIF-002 / UNIF-003 / UNIF-004**
- **Suivi de continuité des chantiers historiques : Richardson, forensic, performance et build**

---

# 2. Synchronisation GitHub — état constaté

Le dépôt distant `vgacofc/lumvorax2`, branche `main`, a été relu avant l’audit.

Le HEAD réel est :

**`3b7478bfb4418a77f500e2fcc38b2e0855b2eabf`**

Le commit contient notamment :

- `RAPPORT/164_UNICITE_002_SCHEMA_V3_LUM_ID_UNIF2_3_4_20261003.md`
- `logs/20261003_s163_unicite002_schema_v3.json`
- `src/validation/lum_id_schema.h`
- modifications de `ns_forensic_unif2.c`
- modifications de `ns_forensic_unif3.c`
- une inclusion du nouveau header dans `ns_forensic_unif4.c`

Le commit est donc bien présent sur le dépôt distant.

**Point méthodologique :** le présent rapport distingue systématiquement ce qui est réellement implémenté dans le code de ce qui est seulement annoncé dans le rapport 164.

---

# 3. Verdict exécutif

## Résultat principal

La correction S163 est **réelle et utile**, mais elle ne ferme pas complètement UNICITE-002 au niveau architectural.

### Ce qui est effectivement démontré

Pour les campagnes exécutées d’UNIF-002 et UNIF-003 :

- 49 280 insertions attendues ;
- 49 280 identifiants distincts ;
- 0 `duplicate_ids` ;
- 0 `extra_events` ;
- le nouveau hash set v3 est effectivement utilisé ;
- `cell_idx` est effectivement encodé ;
- la sentinelle v3 `0x0000...` est hors de l’espace des LUM_ID v3 valides lorsque `run_seq >= 1`.

Cela constitue un **PASS expérimental sur la campagne testée**.

### Ce qui n’est pas démontré

En revanche :

1. le compteur `g_lum_run_seq_counter` n’est pas atomique ;
2. le compteur n’est pas global entre unités de compilation/binaires ;
3. le wrap à 65535 réutilise explicitement une valeur déjà utilisée ;
4. UNIF-004 n’utilise pas réellement le hash set v3 ;
5. UNIF-004 conserve son ancien générateur `run_id` par XOR ;
6. UNIF-003 ne possède pas la garde runtime annoncée dans le rapport 164 ;
7. la limite `step` est documentée de façon ambiguë : la constante vaut 65535 mais la garde UNIF-002 interdit précisément `step == 65535` ;
8. aucune preuve de l’unicité inter-processus, inter-binaire ou inter-session n’est fournie ;
9. la preuve actuelle reste limitée à la campagne de 49 280 événements.

**Verdict audit :**

> **UNICITE-002 : PASS CAMPAGNE pour UNIF-002/003, mais NOT CLOSED au niveau architectural.**

---

# 4. PC1 — Vocabulaire des métriques de doublons

## Processus

Le nouveau `LumIDHashSetV3` sépare :

- `total_insertions`
- `distinct_ids`
- `duplicate_ids`
- `extra_events`

C’est une amélioration importante.

**C’est-à-dire :**

Si 100 événements sont reçus mais seulement 98 identifiants différents existent :

- total_insertions = 100 ;
- distinct_ids = 98 ;
- extra_events = 2 ;
- duplicate_ids dépend du nombre de clés distinctes qui ont été répétées.

Cette séparation évite de confondre « nombre de clés répétées » et « nombre d’événements supplémentaires ».

## Problème

Le correctif est bon pour le hash set v3, mais la campagne UNIF-004 utilise encore l’ancien `LumIDHashSet`.

Il existe donc actuellement **deux systèmes de mesure différents** dans le dépôt.

## Solution / suggestion

Uniformiser les trois campagnes autour d’un seul contrat de métriques :

- total_insertions ;
- distinct_ids ;
- duplicate_ids ;
- extra_events ;
- overflow_hash ;
- invalid_id ;
- expected_events ;
- missing_events.

**Statut : PC1 ADRESSÉ pour v3, mais standardisation globale encore OPEN.**

---

# 5. PC2 — Limite du champ step

## Processus

Le schéma v3 réserve 16 bits au champ `step`.

Un champ de 16 bits peut représenter 65 536 valeurs :

**0 à 65 535 inclus.**

Le header définit `LUM_ID_V3_MAX_STEPS = 65535`.

## Problème 1 — ambiguïté de vocabulaire

Le nom « MAX_STEPS » peut être compris comme :

- nombre maximal de pas = 65 535 ;

alors que la valeur maximale représentable par le champ est :

- step maximal = 65 535.

Ce ne sont pas la même chose.

**C’est-à-dire :**

Une campagne de 65 536 pas pourrait être indexée par :

0, 1, 2, ..., 65 535.

Si le système veut autoriser exactement cette plage, il faut distinguer :

- MAX_STEP_VALUE = 65535 ;
- MAX_STEP_COUNT = 65536.

## Problème 2 — UNIF-002

Dans UNIF-002, la garde teste :

`step >= 65535`

et arrête donc la campagne lorsque `step == 65535`.

La valeur 65535 n’est donc pas utilisable par cette boucle.

## Problème 3 — UNIF-003

Dans UNIF-003, la boucle ne contient pas cette garde runtime, contrairement à ce que laisse entendre le rapport 164.

La campagne actuelle ne révèle pas le problème parce qu’elle utilise seulement 10 steps.

## Solution / suggestion

Définir explicitement :

- valeur maximale : 65 535 ;
- nombre maximal de valeurs : 65 536 ;
- comportement exact lorsque la campagne dépasse cette limite.

Puis utiliser la même politique dans UNIF-002, UNIF-003 et UNIF-004.

**Statut : PC2 PARTIELLEMENT ADRESSÉ.**

---

# 6. PC3 — compteur run_seq : correction insuffisante

## Processus

Le nouveau système remplace le XOR d’un timestamp par un compteur séquentiel.

C’est conceptuellement meilleur :

run 1 → 1  
run 2 → 2  
run 3 → 3  
etc.

Tant que les valeurs restent dans la plage et que le compteur est correctement partagé, la séquence est injective.

## Problème critique 1 — le compteur n’est pas atomique

Le code réellement présent dans `lum_id_schema.h` déclare :

`static uint16_t g_lum_run_seq_counter`

et l’incrémente directement.

Il n’existe pas d’opération atomique de type C11 `atomic_fetch_add`, ni de mutex autour de l’incrément.

Le rapport 164 qualifie pourtant ce compteur d’« atomique ».

Cette affirmation n’est donc pas démontrée par l’implémentation actuelle.

**C’est-à-dire :**

Deux threads peuvent lire simultanément la même ancienne valeur, puis écrire la même nouvelle valeur.

Exemple conceptuel :

- thread A lit 10 ;
- thread B lit 10 ;
- A écrit 11 ;
- B écrit 11.

Deux appels ont alors reçu 11.

## Problème critique 2 — static dans le header

Parce que la variable est définie directement dans le header avec `static`, chaque unité de compilation possède sa propre copie.

Cela signifie qu’UNIF-002 peut commencer à 1 et qu’UNIF-003 peut également commencer à 1.

Le rapport 164 le reconnaît comme « normal », mais cela signifie que `run_seq` n’est **pas un identifiant global de run**.

Il s’agit d’un compteur local à chaque unité de compilation/processus.

## Problème critique 3 — wrap non injectif

Lorsque 65535 est atteint, le code remet le compteur à 1.

Donc :

1, 2, ..., 65535, 1

La seconde occurrence de 1 est exactement une réutilisation d’un identifiant déjà attribué.

Le message DEBUG avertit du problème, mais l’avertissement ne répare pas la collision.

## Solution / suggestions

Pour une vraie unicité de session :

- compteur atomique réel ;
- politique d’arrêt avant épuisement ;
- jamais de réutilisation silencieuse ;
- état de session explicitement défini ;
- identifiant de campagne persistant si plusieurs processus doivent partager l’espace.

Une stratégie robuste est :

**si la prochaine valeur provoquerait un wrap, la génération d’un nouveau run doit échouer plutôt que retourner une valeur déjà utilisée.**

**Statut : PC3 NON FERMÉ.**

---

# 7. PC4 — sentinelle HASH_EMPTY

## Processus

Le schéma v3 utilise 0 comme valeur vide.

Avec :

run_seq >= 1

les bits hauts du LUM_ID sont non nuls.

Donc :

LUM_ID = 0

ne peut pas être produit par un LUM_ID v3 valide correctement construit.

## Problème

Pour le hash set v3 lui-même, le raisonnement est cohérent.

Mais UNIF-004 conserve son ancien hash set et son ancien schéma.

Le problème PC4 n’est donc pas fermé globalement dans les trois campagnes.

## Solution

Migrer réellement UNIF-004 vers :

- `lum_id_v3_encode` ;
- `LumIDHashSetV3` ;
- mêmes métriques ;
- mêmes validations ;
- même contrat d’erreur.

**Statut : PC4 PASS pour le hash set v3, OPEN pour l’architecture forensic globale.**

---

# 8. UNIF-004 — le point le plus important du S163

## Processus

Le commit ajoute bien l’inclusion de `lum_id_schema.h` dans UNIF-004.

Mais l’inclusion du header ne signifie pas que le binaire utilise son schéma.

L’audit du code montre encore :

- `make_run_id()` basé sur XOR de timestamp ;
- `encode_lum_id_64()` ancien ;
- `LumIDHashSet` ancien ;
- `hashset_insert()` ancien ;
- `hashset_destroy()` ancien ;
- `HASH_EMPTY` de l’ancien système.

## Problème

Le rapport 164 affirme une extension du schéma v3 aux trois fichiers, alors que pour UNIF-004 l’implémentation observée est essentiellement :

**header v3 inclus, mais chemin d’exécution v2/ancien conservé.**

Le résultat « 0 doublon » de la campagne UNIF-004 peut donc être vrai sans prouver que le nouveau contrat v3 fonctionne pour UNIF-004.

## Solution

Effectuer une vraie migration UNIF-004 :

- générateur de run v3 ;
- encodeur v3 ;
- hash set v3 ;
- métriques v3 ;
- vérification post-campagne cohérente ;
- preuve de compilation sans chemin ancien résiduel.

**Statut : OPEN — priorité P0/P1 selon objectif de certification.**

---

# 9. Preuve expérimentale : ce que signifie réellement « 49 280 / 49 280 »

## Processus

Le hash set vérifie que, dans la campagne exécutée :

`distinct_ids = expected_count`

et :

`duplicate_ids = 0`

avec :

`total_insertions = expected_count`.

C’est une preuve utile sur l’échantillon exécuté.

## Problème

Cela ne constitue pas une preuve mathématique de l’unicité de toutes les exécutions futures.

La preuve couvre :

- la grille testée ;
- le nombre de steps testé ;
- les modules réellement appelés ;
- les valeurs de `cell_idx` effectivement produites ;
- le run_seq effectivement utilisé.

Elle ne couvre pas automatiquement :

- 65 535+ steps ;
- 65 536+ cellules ;
- plusieurs processus ;
- plusieurs sessions ;
- concurrence ;
- wrap du compteur ;
- mélange des anciens et nouveaux encodeurs.

## Solution

Construire une campagne de tests de propriétés du schéma :

1. injectivité exhaustive des champs dans leurs domaines ;
2. test des frontières 0 / max ;
3. test du premier dépassement ;
4. test du wrap ;
5. test multi-thread ;
6. test multi-processus ;
7. test multi-binaire ;
8. test de collision inter-version ;
9. test de reconstruction inverse du tuple à partir du LUM_ID.

**Statut : OPEN.**

---

# 10. Cell_idx — limite réelle

Le champ `cell_idx` dispose de 16 bits.

Il représente donc :

65 536 cellules distinctes, indices 0 à 65 535.

Pour une grille carrée 256×256 :

256 × 256 = 65 536.

Donc **256×256 est encore représentable** si l’indexation est exactement linéaire 0..65535.

Le dépassement commence pour une grille carrée 257×257, car :

257 × 257 = 66 049.

Pour les champs décalés U/V, le nombre réel de cellules peut être inférieur à nx×ny ; la limite doit donc être définie en fonction du produit réellement encodé, pas uniquement du nom de la grille.

**Statut : limite correctement identifiable, mais test de frontière encore OPEN.**

---

# 11. Cohérence timestamp

UNIF-002 et UNIF-003 utilisent encore `time_ns_get_absolute()`, donc CLOCK_REALTIME selon l’architecture déjà auditée.

UNIF-004 utilise `time_ns_get_monotonic()` pour son timestamp principal.

Cela n’est pas nécessairement incorrect : CLOCK_REALTIME sert à la corrélation civile, CLOCK_MONOTONIC à la durée et à l’ordre temporel.

Le problème est plutôt l’absence d’un contrat unique explicite.

**Suggestion :**

Chaque événement forensic devrait porter explicitement :

- timestamp monotonic ;
- timestamp civil facultatif ;
- source de l’horloge ;
- unité ;
- séquence événementielle.

Cela évite de déduire la sémantique de l’horloge depuis le contexte.

---

# 12. Registre des tâches antérieures — continuité obligatoire

Les corrections S163 ne doivent pas effacer les chantiers précédemment ouverts.

## Toujours OPEN

### P0/P1 forensic

- UNICITE-002 architecture globale ;
- migration réelle UNIF-004 vers v3 ;
- unicité inter-processus/inter-session ;
- compteur run réellement atomique ;
- politique anti-wrap ;
- limites step/cell_idx.

### Richardson

- Richardson-PROTOCOL-003c ;
- cohérence Couette / conditions aux limites ;
- résolution du problème des BC internes du solveur ;
- protocole manufactured/exact solution ;
- Protocol C dt∝dx² complet ;
- T01b–T06b non-évaluables tant que la campagne complète n’est pas établie.

### Forensic / performance

- PERF-FORENSIC-001 ;
- BUILD-THREAD-001 ;
- BUILD-PROOF-001 ;
- BUILD-PORT-002 ;
- séparation timestamp monotonic/civil ;
- preuve d’absence de pertes sur toutes les voies logger.

### Validation scientifique

- T04 Richardson renforcé ;
- sweep robuste Lyapunov ;
- FORENSIC Richardson ;
- FORENSIC Lyapunov ;
- FORENSIC NX-42.

### NX-42

- C3 NS → NX-42 ;
- C4 NX-42 problèmes 6–30 ;
- BL-003 → BL-012.

---

# 13. Nouvelle hiérarchie de travail après S163

## P0 — UNICITE-003

**Objectif : rendre le schéma v3 réellement global et non seulement local à une campagne.**

À traiter :

1. compteur atomique réel ;
2. refus du wrap ;
3. définition formelle de « session » ;
4. stratégie inter-processus ;
5. test frontière 65535 ;
6. test multi-thread ;
7. test multi-binaire.

## P0 — UNIF-004-V3

Migrer réellement le chemin UNIF-004 vers le même schéma et le même hash set.

## P1 — SCHEMA-BOUNDARY-001

Tester systématiquement :

- run_seq = 0 ;
- run_seq = 1 ;
- run_seq = 65534 ;
- run_seq = 65535 ;
- tentative suivante ;
- step = 0 ;
- step = 65534 ;
- step = 65535 ;
- step = 65536 ;
- cell_idx = 0 ;
- cell_idx = 65535 ;
- cell_idx = 65536 ;
- bit_pos = 0 ;
- bit_pos = 63 ;
- bit_pos = 64.

## P1 — FORENSIC-CONSISTENCY-001

Faire converger UNIF-002/003/004 vers exactement le même contrat de preuve.

## P1 — PERF-FORENSIC-001

Poursuivre l’audit du logger, notamment les voies qui ne passent pas nécessairement par le batch flush déjà corrigé.

## P1 — BUILD-THREAD-001 / BUILD-PROOF-001 / BUILD-PORT-002

Maintenir les chantiers de reproductibilité et portabilité.

## P2 — Richardson / Lyapunov / NX-42

Reprendre les campagnes scientifiques précédemment ouvertes sans les considérer comme résolues par UNICITE-002.

---

# 14. Conclusion d’audit

S163 apporte une amélioration structurelle importante : le dépôt dispose désormais d’un header partagé pour le schéma LUM_ID v3 et d’un hash set permettant de distinguer précisément plusieurs catégories de doublons.

La campagne UNIF-002/003 montre bien :

**49 280 insertions → 49 280 identifiants distincts → 0 duplicate_ids → 0 extra_events.**

Cette conclusion est valide **pour la campagne effectivement exécutée**.

En revanche, plusieurs affirmations du rapport 164 sont trop fortes par rapport au code réel :

- le compteur n’est pas atomique ;
- le compteur est local à chaque unité de compilation ;
- le wrap réutilise une valeur ;
- UNIF-004 n’est pas réellement migré vers le v3 ;
- UNIF-003 n’a pas la garde runtime annoncée ;
- la limite step est sémantiquement ambiguë.

Le bon état de certification est donc :

**UNICITE-002 — PASS expérimental UNIF-002/003 / NOT CLOSED architecturalement.**

**UNIF-004 — PASS de sa campagne historique, mais migration v3 encore OPEN.**

**CERTIFIED_100=false.**

**unique_human_proven=false.**

Aucune modification du code source n’est recommandée dans ce rapport : les corrections restent à réaliser dans une session d’implémentation distincte, puis à auditer indépendamment.
