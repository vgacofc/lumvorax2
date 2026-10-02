# Rapport 160 — Audit critique S159 : FORENSIC-UNIF-003 et Richardson-PROTOCOL-003b

**Date :** 2026-10-03  
**Session auditée :** S159  
**HEAD vérifié :** `a3cee665259cff38dc938d9a9c9514cc731acb9d`  
**Branche :** `main`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG**

---

## 1. Expertises activées

1. **Audit forensique des journaux** — identité LUM 64 bits, séquence d'événements, unicité, pertes et doublons.
2. **Métrologie temporelle POSIX** — distinction `CLOCK_MONOTONIC` / `CLOCK_REALTIME`, cohérence des horodatages.
3. **CFD / méthode des différences finies** — grille MAC décalée et conditions limites périodiques.
4. **Analyse Navier–Stokes incompressible** — ordre d'application des conditions limites dans la projection de Chorin.
5. **Validation numérique / Richardson** — séparation entre correction d'implémentation, exactitude du problème et convergence.
6. **Audit C / concurrence** — portée réelle des mutex et des garanties de séquence.
7. **Reproductibilité Git** — confrontation du rapport S159 avec le contenu réellement présent dans `main`.

---

## 2. Synchronisation GitHub préalable

Le dépôt distant `vgacofc/lumvorax2` a été relu avant l'analyse.

Le HEAD réel correspond bien à :

`a3cee665259cff38dc938d9a9c9514cc731acb9d`

Le commit porte le titre :

**Add S159 FORENSIC-UNIF-003 PASS + Richardson-PROTOCOL-003b BUG-1/BUG-3/PERF-1 closed**

Aucun fichier source n'a été modifié par cet audit. Seul le présent rapport est ajouté sous `RAPPORT/`.

---

# 3. FORENSIC-UNIF-003

## 3.1 BUG-1 — LUM_ID 64 bits

### Processus

Le logger expose maintenant `forensic_log_individual_lum(uint64_t lum_id, ...)`. Le format d'écriture utilise `PRIu64` et `PRIx64`, donc le champ 64 bits n'est plus réduit à 32 bits lors de l'écriture.

Le code de S159 transmet également le `uint64_t` produit par `encode_lum_id_64()`.

### Problème

Le défaut 32→64 identifié au rapport 158 est donc effectivement supprimé au niveau du type de l'API et de l'écriture du log.

Cependant, **« identifiant 64 bits » ne signifie pas automatiquement « unicité démontrée »**.

Dans S159, la vérification post-campagne calcule :
- le nombre d'événements ;
- les gaps de `event_seq` ;
- un XOR global des `lum_id`.

Elle ne construit pas de table d'identifiants et ne compare pas chaque nouveau `lum_id` avec les précédents.

Le XOR est un résumé algébrique, pas un test d'unicité. Deux ensembles différents peuvent avoir le même XOR, y compris zéro.

### Solution et suggestions

**Verdict : BUG-1 type/troncature = CLOSED. Unicité des LUM_ID = NON PROUVÉE.**

Pour fermer complètement la propriété « unique », le test doit utiliser un ensemble exact des `lum_id` :
- insertion de chaque ID ;
- détection immédiate si l'ID existe déjà ;
- comparaison `unique_count == total_events`.

Pour les grandes campagnes, un hash set ou un tri externe permet de conserver une vérification déterministe sans conserver tout le journal en mémoire.

---

# 4. BUG-3 — horloges : fermeture annoncée trop forte

## 4.1 Processus

Le logger initialise son header avec :

- `forensic_get_monotonic_ns()` → `CLOCK_MONOTONIC`
- `CLOCK_REALTIME` séparément pour la corrélation civile.

C'est une bonne séparation conceptuelle.

Mais `forensic_log_individual_lum()` écrit exactement le timestamp reçu par son appelant.

## 4.2 Problème

Dans `ns_forensic_unif3.c`, les timestamps transmis aux événements sont obtenus par :

`time_ns_get_absolute()`

Or `src/common/time_ns.c` définit cette fonction avec :

`clock_gettime(CLOCK_REALTIME, ...)`

Le fichier S159 le reconnaît lui-même lorsqu'il affiche :

**« CLOCK_REALTIME pour les événements | CLOCK_MONOTONIC pour le header log »**

Cela contredit donc la déclaration :

**« header + footer log = CLOCK_MONOTONIC (cohérent avec les événements) »**

Les événements de FORENSIC-UNIF-003 ne sont pas horodatés avec `CLOCK_MONOTONIC`. Ils utilisent `CLOCK_REALTIME`.

### C'est-à-dire

Le logger possède maintenant deux horloges correctement étiquetées, mais la campagne S159 mélange encore leur sémantique :

- header/footer : monotonic + realtime explicitement étiquetés ;
- événements : realtime ;
- durée de campagne : monotonic.

Ce n'est pas nécessairement dangereux pour la chronologie locale si `CLOCK_REALTIME` reste monotone pendant le run, mais ce n'est **pas la garantie contractuelle recherchée** par BUG-3.

### Solution et suggestions

Il faut choisir explicitement le contrat.

**Option recommandée :**
- `ts_monotonic_ns` pour tous les événements forensiques destinés à l'ordre et aux durées ;
- `ts_realtime_ns` séparé lorsque la corrélation avec l'heure civile est nécessaire ;
- `event_seq` comme ordre discret définitif.

Le format idéal devient donc conceptuellement :

**événement = sequence + timestamp monotonic + timestamp realtime optionnel + LUM_ID + opération**

**Verdict : BUG-3 = PARTIELLEMENT CORRIGÉ, pas CLOSED.**

---

# 5. PERF-1 — batch flush

## Processus

`forensic_log_individual_lum()` incrémente un compteur et appelle `fflush()` toutes les 1024 écritures, avec un flush final dans `forensic_logger_destroy()`.

Cela supprime bien le `fflush()` par événement individuel pour cette voie d'écriture.

## Problème

La fermeture de PERF-1 doit rester limitée au chemin concerné.

Le fichier contient encore des appels `fflush()` dans d'autres fonctions de journalisation, notamment :
- opérations mémoire ;
- opérations LUM agrégées ;
- journal forensic générique ;
- journal unified.

Le passage de 1 flush/événement à 1 flush/1024 événements est donc réel pour `forensic_log_individual_lum()`, mais ne constitue pas encore une preuve de performance globale du logger.

Le chiffre « PERF-1 CLOSED » est acceptable uniquement si PERF-1 est défini précisément comme « flush individuel du chemin individual_lum ».

### Solution et suggestions

Conserver :

**PERF-1 narrow = CLOSED**

et créer séparément :

**PERF-FORENSIC-001 = OPEN**

avec mesure :
- flush/event ;
- flush/256 ;
- flush/1024 ;
- flush/4096 ;
- flush final ;
- temps mur ;
- temps CPU ;
- nombre d'appels système si mesurable ;
- quantité potentiellement perdue en cas d'arrêt brutal.

---

# 6. event_seq — bonne correction, mais portée à préciser

## Processus

Le logger incrémente `g_event_seq` sous `fl001_log_file_mutex`. Chaque événement individual_lum reçoit donc une séquence unique dans cette instance du logger.

S159 obtient :
- 49 280 événements attendus ;
- 49 280 événements tracés ;
- séquence continue ;
- zéro gap.

## Problème

Cela prouve une séquence continue **dans cette session de logger**, pas une identité globale inter-session.

Le compteur est remis à zéro dans `forensic_logger_init()`.

Donc :

- session A : seq 1…49280 ;
- session B : seq 1…49280.

Il n'y a pas de collision de séquence si les sessions sont séparées par leur fichier, mais `event_seq` seul n'est pas un identifiant global persistant.

### Solution et suggestions

Conserver le contrat :

**event_seq = ordre total local à un fichier/session**

et utiliser :

**run_id + event_seq**

si une identité globale de campagne est requise.

**Verdict : event_seq = PASS pour la continuité intra-session.**

---

# 7. Richardson-PROTOCOL-003b — le diagnostic est juste sur un point, mais incomplet

## 7.1 Processus réel du solveur

`ns_solver_step()` exécute :

1. copie des champs temporaires ;
2. calcul de vitesse intermédiaire ;
3. Poisson ;
4. correction des vitesses ;
5. appel systématique à `ns_solver_set_lid_bc()` ;
6. incrément du pas.

Le code S159 réapplique ensuite `set_couette_periodic_bc()`.

Ainsi, la condition Couette n'est pas utilisée pendant toute la chaîne d'un pas.

### C'est-à-dire

Le solveur fait d'abord :

**« calcul avec les hypothèses lid-driven »**

puis le test fait :

**« remettre les frontières Couette/périodiques après coup »**.

Ce n'est pas équivalent à faire évoluer directement le problème Couette.

Le diagnostic architectural de S159 est donc fondé.

---

# 8. Nouvelle anomalie P0/P1 — périodicité de u incompatible avec la convention MAC du solver

## Processus

Dans `ns_solver_2d.c`, la convention documentée est :

- `u[i][j]` : faces verticales ;
- `u[0][j]` : frontière Ouest ;
- `u[nx][j]` : frontière Est ;
- intérieur : `i=1...nx-1`.

Le solveur utilise notamment :

`(U(i,j)-U(i-1,j))/dx`

pour construire la divergence.

## Problème

S159 définit les bords périodiques de `u` ainsi :

- `U(0,j) = U(nx-1,j)`
- `U(nx,j) = U(1,j)`

Cette règle ne correspond pas à la périodicité naturelle de deux faces physiques identifiées.

Pour une composante `u` placée sur les faces verticales, les deux faces périodiques situées à x=0 et x=L doivent représenter la même valeur physique. Le schéma doit donc expliciter une relation du type :

**face Est = face Ouest**

et non remplacer chaque frontière par une copie décalée d'une cellule intérieure.

### Pourquoi c'est important

Le choix S159 introduit un décalage spatial dans la condition périodique.

Autrement dit, au lieu de dire :

**« la valeur à x=L est la même que celle à x=0 »**

le code dit :

**« la valeur de frontière est une copie de la première/dernière face intérieure »**.

Cela peut créer exactement le type de contamination que le test cherche à diagnostiquer.

### Solution et suggestions

Avant de modifier le solveur, faire une preuve discrète minimale :

1. imposer analytiquement `u(y)=y` dans tout le champ ;
2. appliquer les CL périodiques proposées ;
3. vérifier bit/exactement les résidus :
   - périodicité de `u` ;
   - divergence ;
   - Laplacien ;
   - terme d'advection ;
4. vérifier que chaque terme discrétisé est nul ou conforme à la solution exacte.

Ensuite seulement choisir la convention définitive.

**Verdict : nouvelle anomalie Richardson-003b = OPEN.**

Le callback dans `ns_solver_step()` reste pertinent, mais il ne suffit pas à lui seul : la définition discrète des CL périodiques doit également être corrigée et démontrée.

---

# 9. BUG-2 — correction de la coordonnée : utile, mais formulation à resserrer

S159 utilise :

`y_node = (j - 0.5) * dy`

pour comparer la composante `u`.

Cela est cohérent avec la convention actuellement documentée par le solver pour les valeurs intérieures utilisées dans ses extractions.

Cependant, le rapport S159 affirme encore de façon trop absolue que BUG-2 expliquait le `Linf > 1` antérieur.

Le nouveau test mesure correctement `u_min` et `u_max`, mais une preuve de causalité exige une comparaison contrôlée :

- même état numérique ;
- ancienne coordonnée ;
- nouvelle coordonnée ;
- mêmes champs ;
- comparaison des métriques uniquement.

### Verdict

**Correction de référence = VALIDÉE comme changement de convention cohérent.**

**Causalité historique « BUG-2 seul ⇒ Linf > 1 » = NON PROUVÉE.**

---

# 10. L1 ≈ 0,5 — le résultat est un signal, pas encore une preuve complète de cause racine

Les résultats S159 montrent notamment :

- L1 ≈ 0,500 ;
- L2 ≈ 0,547 ;
- `u_min < 0` ;
- `u_max < 1`.

Le fait que L1 reste proche de 0,5 montre que la solution calculée n'est pas la solution Couette attendue.

Le lien avec `set_lid_bc()` interne est fortement plausible et cohérent avec le code.

Mais la nouvelle périodicité de `u` constitue maintenant une seconde cause potentielle directement visible dans le code.

### Solution

Richardson-003c doit isoler les causes dans cet ordre :

1. supprimer l'appel interne imposé à `set_lid_bc()` par callback ou variante de step ;
2. définir les CL périodiques selon la convention MAC exacte ;
3. imposer `u=y`, `v=0), `p=constante` comme état de test ;
4. calculer les résidus discrets ;
5. seulement après, lancer les campagnes temporelles ;
6. seulement après, calculer les ordres Richardson.

---

# 11. Un point supplémentaire sur le protocole Richardson

Le protocole C utilise `dt ∝ dx²`, ce qui est raisonnable pour séparer la contrainte diffusive.

Mais S159 n'a pas terminé la grille 128×128 dans la campagne annoncée.

Donc :

- T01b : non évalué tant que le résultat C complet n'est pas établi ;
- T02b : non évalué ;
- T03b : non évalué ;
- T04b : non évalué ;
- T05b : non évalué ;
- T06b : non évalué.

Il faut conserver ces états comme **NON ÉVALUABLES**, et non les transformer en FAIL artificiel ni en PASS anticipé.

---

# 12. État de vérité après audit S159

| Élément | Verdict audit 160 |
|---|---|
| LUM_ID API 64 bits | ✅ CLOSED |
| LUM_ID réellement écrit en 64 bits | ✅ CLOSED |
| Unicité exacte des LUM_ID | ⚠ NON PROUVÉE |
| event_seq intra-session | ✅ PASS |
| absence de gap sur 49 280 événements | ✅ PASS documenté |
| event_seq global inter-session | ⚠ NON |
| BUG-3 séparation header monotonic/realtime | ✅ architecture partiellement correcte |
| BUG-3 timestamps événements monotonic | 🔴 NON — événements S159 = CLOCK_REALTIME |
| PERF-1 individual_lum batch 1024 | ✅ CLOSED dans ce chemin |
| performance globale du logger | ⚠ OPEN |
| BUG-2 coordonnée | ✅ correction appliquée |
| causalité BUG-2 → ancien Linf>1 | ⚠ NON PROUVÉE |
| lid_bc interne pendant step | 🔴 CONFIRMÉ |
| périodicité `u` selon convention MAC | 🔴 À corriger/démontrer |
| Couette exact validé | 🔴 NON |
| Richardson-003b | 🔴 FAIL / DIAGNOSTIC PARTIEL |
| Richardson-003c | 🔴 OPEN |
| CERTIFIED_100 | **false** |
| unique_human_proven | **false** |

---

# 13. Ordre de travail recommandé

## P0 — Richardson-PROTOCOL-003c

### Processus

Construire une étape de solveur capable de recevoir les CL du problème au bon moment.

### Problème

Réappliquer les CL après `ns_solver_step()` ne supprime pas la contamination interne.

### Solution

Introduire une abstraction de conditions limites appliquée :

- avant calcul intermédiaire ;
- après calcul intermédiaire si nécessaire ;
- avant Poisson si nécessaire ;
- après correction.

Puis définir une implémentation Couette périodique conforme à la grille MAC.

**Ne pas considérer 003c comme PASS avant preuve discrète.**

---

## P0 — FORENSIC-UNIF-004

### Objectif

Fermer réellement les deux propriétés encore ouvertes :

1. timestamps monotonic des événements ;
2. unicité exacte des LUM_ID.

Ajouter également un identifiant de session explicite si une traçabilité inter-fichiers est nécessaire.

---

## P1 — BUILD-THREAD-001

Poursuivre l'audit des mutex et tester réellement les chemins concurrents avec les outils de détection de courses disponibles dans le dépôt/environnement.

Un simple comptage lexical lock/unlock ne suffit pas à prouver une race.

---

## P1 — PERF-FORENSIC-001

Mesurer réellement les stratégies de flush au lieu de qualifier une valeur de 1024 comme universellement optimale.

---

## P2 — ARCH-MEM-001

Le pool LUM et les mécanismes mémoire existants doivent être benchmarkés avant toute nouvelle optimisation.

---

# 14. Conclusion

La session 159 contient de vraies corrections importantes :

- le passage de `lum_id` à 64 bits est réel ;
- la séquence `event_seq` apporte une traçabilité nettement meilleure ;
- le batch flush de 1024 réduit bien le coût du chemin individual_lum ;
- le défaut architectural de `ns_solver_step()` est correctement identifié.

Mais le label **« BUG-3 CLOSED » est prématuré** : les événements FORENSIC-UNIF-003 utilisent toujours `CLOCK_REALTIME`.

De même, le Richardson-003b ne peut pas encore servir de validation analytique Couette. En plus du `set_lid_bc()` interne, la définition des faces périodiques de `u` doit être démontrée compatible avec la convention MAC du solveur.

La conclusion conservatrice reste donc :

**FORENSIC-UNIF-003 : PASS partiel — identité 64 bits et séquence validées, unicité exacte et contrat temporel monotonic à compléter.**

**Richardson-PROTOCOL-003b : FAIL diagnostique utile — cause architecturale identifiée, mais problème discret encore non fermé.**

**CERTIFIED_100=false.**

**Aucun code source n'a été modifié pendant cet audit.**
