# Rapport 163 — Audit critique S162 : UNICITE-001, LUM_ID v2 et limites de preuve

**Date :** 2026-10-03  
**Session auditée :** S162  
**HEAD audité :** `75ee72dd7d7b000ff09b08f5f4e0ccc1b0c0c8bb`  
**HEAD précédent déclaré :** `6f50173`  
**CERTIFIED_100=false**  
**unique_human_proven=false**

---

## 1. Expertises activées

- **Forensique numérique et traçabilité bit-level** : vérification de l'identité persistante de chaque événement.
- **Architecture d'identifiants 64 bits** : analyse des champs, collisions, débordements et portée d'unicité.
- **Méthodes formelles / preuve d'injectivité** : distinction entre unicité mathématique du tuple et preuve expérimentale sur un journal.
- **Analyse de systèmes de hachage** : contrôle du hash set, de son compteur de doublons et de sa sentinelle.
- **Ingénierie des journaux et event sequencing** : contrôle de la relation LUM_ID / event_seq / journal.
- **Ingénierie de simulation numérique NS/MAC** : contrôle de la couverture des champs et de l'indexation cellulaire.
- **Audit logiciel C** : contrôle des conversions `uint32_t`→`uint16_t`, débordements et valeurs sentinelles.
- **Audit de reproductibilité expérimentale** : distinction entre résultat de campagne S162 et garantie générale du schéma.

---

## 2. Synchronisation GitHub

La branche `main` pointe effectivement vers :

`75ee72dd7d7b000ff09b08f5f4e0ccc1b0c0c8bb`

Le commit est donc bien présent dans le dépôt distant. Il modifie notamment `src/validation/ns_forensic_unif4.c` et ajoute la documentation/log S162.

Aucun fichier source n'est modifié par le présent audit. Ce rapport est le seul artefact ajouté.

---

# 3. UNICITE-001 — correction du défaut principal

## Processus

Dans S162, l'identifiant LUM_ID devient :

- run_id : 16 bits
- protocol : 4 bits
- module : 4 bits
- step : 16 bits
- cell_idx : 16 bits
- bit_pos : 8 bits

Soit exactement 64 bits.

C'est-à-dire que deux bits provenant de deux cellules différentes peuvent désormais avoir des identifiants différents même lorsqu'ils appartiennent au même module, au même pas et à la même position binaire.

Le champ `cell_idx` est incrémenté séparément dans les boucles de U, V, P, Utmp et Vtmp.

## Problème

Le défaut S161 était réel : le schéma précédent ne distinguait pas les cellules d'un même champ.

Pour une grille 4×4 :

- U : 12 cellules
- V : 12 cellules
- P : 16 cellules
- Utmp : 12 cellules
- Vtmp : 12 cellules
- Uout : 12 cellules
- Poisson : 1 scalaire

Le schéma v1 pouvait donc produire le même LUM_ID pour plusieurs cellules.

## Solution S162

L'intégration de `cell_idx` corrige effectivement cette collision intra-champ pour la campagne exécutée.

Le hash set de S162 rapporte :

- 49 280 événements tracés ;
- 49 280 LUM_ID distincts ;
- 0 doublon détecté ;
- 0 perte selon le compteur attendu ;
- event_seq final = 49 280 ;
- 0 gap ;
- 0 régression temporelle dans le contrôle effectué.

**Verdict : UNICITE-001 est correctement corrigé pour la campagne S162 4×4 / 10 steps.**

---

# 4. Correction importante du rapport S162 : 3 840 et 45 440 ne désignent pas la même chose

## Processus

Le hash set possède deux notions différentes :

1. `size` = nombre de clés LUM_ID distinctes ;
2. `duplicates` = nombre de clés qui ont été rencontrées au moins deux fois.

Dans S162, `duplicates` est incrémenté uniquement au passage du compteur 1 → 2.

C'est donc un **nombre de clés dupliquées**, pas le nombre total d'événements répétés.

## Problème

Le rapport S162 indique à plusieurs endroits « 3 840 doublons » puis associe ce nombre à une situation où 49 280 événements existaient.

Pour la campagne v1 décrite, le calcul cohérent est :

**49 280 événements - 3 840 clés distinctes = 45 440 insertions correspondant à des occurrences supplémentaires.**

Autrement dit :

- **3 840** = nombre de LUM_ID distincts ayant plusieurs occurrences, si cette métrique était bien celle calculée ;
- **45 440** = nombre d'occurrences au-delà de la première ;
- **49 280** = nombre total d'événements.

La valeur « 45 440 entrées distinctes » présentée dans le tableau S162 ne peut donc pas être interprétée comme un nombre de LUM_ID distincts pour le schéma v1.

## Solution

Pour les futurs rapports, employer trois métriques séparées :

- `events_total`
- `unique_lum_ids`
- `duplicate_keys`
- et éventuellement `duplicate_occurrences = events_total - unique_lum_ids`

Cela évite de transformer une métrique de multiplicité en métrique d'unicité.

---

# 5. La preuve d'unicité est locale à S162, pas globale au système

## Processus

Le tuple S162 est injectif tant que chacun de ses champs reste dans la plage représentée :

`(run_id, protocol, module, step, cell_idx, bit_pos)`

Le LUM_ID est alors une concaténation sans chevauchement des champs.

## Problème

Le rapport emploie une formulation générale du type « garantie d'unicité ».

Cette formulation est trop large.

Le schéma est unique seulement dans son **domaine de représentation**.

### Limite 1 — step

`step` ne possède plus que 16 bits.

Après 65 535, le code effectue :

`step = (uint16_t)(step_n & 0xFFFFU)`

Donc le step 65 536 redevient 0.

C'est-à-dire que deux événements provenant du même run, protocole, module et cellule peuvent recevoir le même identifiant après le retour modulo 65 536.

### Limite 2 — cell_idx

`cell_idx` est limité à 65 535.

Pour 256×256 :

`cells_p = 65 536`

La dernière cellule possède donc l'indice 65 535 et reste représentable.

Pour 512×512 :

`cells_p = 262 144`

Le domaine dépasse 16 bits.

### Limite 3 — run_id

`run_id` est produit par un XOR de quatre mots de 16 bits extraits du timestamp monotone.

Ce mécanisme ne fournit pas une unicité mathématique globale des runs : deux timestamps différents peuvent produire le même résultat 16 bits.

### Limite 4 — protocol/module

Protocol et module sont tronqués respectivement à 4 bits. Le système ne peut donc représenter que 16 valeurs distinctes dans chacun de ces champs.

## Solution

La propriété à déclarer doit être :

**« LUM_ID injectif pour le domaine explicitement borné de la campagne S162. »**

Et non :

**« LUM_ID globalement unique sans condition. »**

Pour une identité réellement globale, le schéma futur doit intégrer un identifiant de campagne/run non collisionnel, ou utiliser un identifiant séquentiel global accompagné d'une relation de provenance vérifiable.

---

# 6. Anomalie supplémentaire : valeur sentinelle HASH_EMPTY

## Processus

Le hash set utilise :

`HASH_EMPTY = UINT64_MAX`

pour représenter une case vide.

## Problème

Le domaine théorique du LUM_ID contient lui-même la valeur 64 bits maximale.

Elle est obtenue lorsque tous les champs encodés atteignent leur valeur maximale :

- run_id = 65535
- protocol = 15
- module = 15
- step = 65535
- cell_idx = 65535
- bit_pos = 255

Le hash set ne peut donc pas distinguer cette clé d'une case vide.

La campagne S162 ne produit manifestement pas cette valeur, mais l'architecture possède cette collision réservée.

## Solution

Utiliser une représentation séparée de l'état d'occupation, par exemple un bitset/byte d'occupation, ou réserver explicitement une valeur impossible et démontrer qu'elle est hors domaine.

**État : OPEN, faible priorité pour S162 mais important pour une bibliothèque générique.**

---

# 7. event_seq : bonne preuve de continuité, mais portée limitée

## Processus

`forensic_log_individual_lum()` incrémente `g_event_seq` sous mutex puis écrit :

- timestamp ;
- seq ;
- LUM_ID ;
- opération.

La vérification post-campagne recherche les écarts de séquence.

## Problème

Le compteur est remis à zéro dans `forensic_logger_init()`.

Il est donc continu dans un fichier/session, mais pas globalement unique entre plusieurs sessions.

C'est une distinction importante :

- **continuité intra-session : PASS démontré pour S162** ;
- **identité inter-session : NON démontrée par event_seq seul**.

## Solution

Conserver un `session_id` indépendant et associer :

`session_id + event_seq`

ou utiliser un identifiant global persistant si une unicité inter-session est exigée.

---

# 8. Timestamp : correction S162 correctement orientée

## Processus

S162 passe `time_ns_get_monotonic()` comme timestamp principal des événements UNIF-004.

Le logger conserve également une horloge civile pour la corrélation.

## Problème

La séparation est maintenant conceptuellement correcte, mais le commentaire du logger affirme encore de manière générale que header et événements utilisent la même source alors que le logger accepte aussi d'autres fonctions utilisant `lum_get_timestamp()`.

La propriété doit donc être formulée au niveau du chemin UNIF-004, pas comme une garantie universelle de tout le logger.

## Solution

Documenter explicitement :

- chemin UNIF-004 : timestamp monotone ;
- horodatage civil : corrélation ;
- autres API du logger : comportement à auditer séparément.

---

# 9. Couverture des autres campagnes

Le commit S162 laisse explicitement `ns_forensic_unif2.c` et `ns_forensic_unif3.c` avec l'ancien schéma.

## Processus

Le correctif est appliqué à UNIF-004.

## Problème

Il serait incorrect d'étendre automatiquement le verdict S162 à UNIF-002 et UNIF-003.

Ces fichiers conservent notamment un encodage basé sur :

`run_id + protocol + module + step32 + bit_pos`

sans `cell_idx` explicite.

## Solution

Conserver trois états distincts :

- **UNIF-004 : unicité corrigée et testée sur S162** ;
- **UNIF-002 : OPEN** ;
- **UNIF-003 : OPEN**.

Aucune certification globale ne doit être déduite du seul PASS UNIF-004.

---

# 10. Conclusion technique

S162 constitue une correction réelle et vérifiée du défaut de collision intra-champ observé dans UNIF-004.

Le résultat **0 doublon / 49 280 LUM_ID distincts** est cohérent avec l'implémentation du hash set et avec la campagne 4×4 / 10 steps.

En revanche, trois formulations doivent rester strictement bornées :

1. **L'unicité est démontrée pour la campagne S162**, pas pour tous les runs futurs.
2. **Le schéma 64 bits possède des limites structurelles** : step 16 bits, cell_idx 16 bits, run_id 16 bits, protocol/module 4 bits.
3. **Le hash set possède une valeur sentinelle théoriquement atteignable**, à traiter avant de considérer l'implémentation comme composant générique définitif.

Le chantier UNICITE-001 peut donc être considéré comme **FERMÉ pour FORENSIC-UNIF-004 dans son domaine de test S162**, mais **pas comme une preuve d'unicité globale de toute l'architecture LumVorax**.

---

# 11. Ordre de continuation

### P0 — à traiter ensuite

**UNICITE-002 — identité inter-session et domaine complet**
- supprimer la dépendance à un `run_id` 16 bits non injectif ;
- formaliser l'espace global des LUM_ID ;
- décider si l'identité doit être globale, sessionnelle ou seulement locale à une campagne ;
- tester explicitement les frontières step=65535/65536 ;
- tester cell_idx=65535/65536 ;
- traiter `HASH_EMPTY`.

### P0 — FORENSIC-UNIF-002 / 003

Porter le principe `cell_idx` dans les campagnes encore basées sur l'ancien schéma, sans supprimer les anciennes preuves historiques.

### P1 — Richardson-PROTOCOL-003c

Poursuivre la correction du problème indépendant d'unicité identifié précédemment : `ns_solver_step()` réapplique encore la condition aux limites lid-driven cavity, ce qui empêche de considérer le test périodique Couette comme mathématiquement pur.

### P1 — PERF-FORENSIC-001

Mesurer réellement les effets du batch flush 1024 au lieu de considérer cette valeur comme universellement optimale.

### P1 — BUILD-THREAD-001 / BUILD-PROOF-001 / BUILD-PORT-002

Conserver ces chantiers ouverts jusqu'à preuve reproductible.

### P2 — architecture LUM_ID > 256×256

Préparer le schéma d'identité pour les grilles dépassant 65 535 cellules par champ.

---

**Verdict audit S162 :**

**UNICITE-001 / UNIF-004 : PASS dans le domaine S162.**

**Unicité globale de l'architecture : NON PROUVÉE.**

**CERTIFIED_100=false — maintenu.**

**unique_human_proven=false — maintenu.**
