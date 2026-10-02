# RAPPORT 142 — AUDIT CROISÉ POST-141 : PORTÉE DU COMMIT, PREUVES DE BUILD, PORTABILITÉ ET REGISTRE PERSISTANT

**Date :** 2026-10-02  
**HEAD distant vérifié :** `a1957b767d9315c0f42a48424e59995c1e836f1d`  
**Commit code précédent :** `5929978d74af7ced0c4c0ba34bb0042dbc39cda4`  
**Commit rapport précédent :** `a1957b7`  
**Branche :** main  
**CERTIFIED_100 :** false  
**Règle de ce rapport :** aucune modification du code source ; ce fichier est le seul artefact ajouté par cet audit.

---

## 1. EXPERTISES ACTIVÉES

- Audit forensique Git/GitHub et analyse de provenance des commits.
- C/C99 et conformité des API standard/POSIX.
- Portabilité Linux/macOS, feature-test macros et édition de liens.
- Allocation mémoire, alignement et durée de vie des objets.
- Concurrence pthread et thread-safety des flux de journalisation.
- Analyse mémoire et instrumentation du memory tracker.
- BIT-LUM/VORAX : identifiants, checksums, formats, B-tree, SIMD et persistance.
- Journalisation structurée et intégrité des données.
- Validation scientifique des solveurs numériques et distinction entre smoke-test et preuve scientifique.
- Cryptographie appliquée et vérification du branchement SHA-256.
- Audit d'authenticité du simulateur NQubit.
- Hygiène du dépôt : artefacts compilés, reproductibilité et séparation source/build.
- Cybersécurité et gestion des secrets.
- Analyse de régression et maintien des tâches héritées des rapports 135 à 141.

---

# 2. SYNCHRONISATION DISTANTE — ÉTAT RÉEL

## Processus

Le dépôt est resynchronisé par interrogation directe des commits récents de `vgacofc/lumvorax2`, puis par comparaison des SHA.

Le HEAD distant actuel est `a1957b7...`.

La chaîne immédiate est :

- `1ddddea` : rapport 140.
- `5929978` : modifications de code et artefacts de build.
- `a1957b7` : ajout du rapport 141 uniquement.

La comparaison `5929978 → a1957b7` montre exactement un fichier ajouté : `RAPPORT/141_CORRECTIONS_C1_C2_PORTABILITE_MACOS_LINUX_20261002.md`.

## Problème

Le rapport 141 présente `5929978` comme le commit des corrections C1/C2/MK. Cette affirmation est correcte quant à l'existence de ces corrections, mais elle ne décrit pas toute la portée du commit.

La comparaison `1ddddea → 5929978` montre également :

- plusieurs dizaines d'objets `.o` modifiés ;
- plusieurs exécutables présents/modifiés dans `bin/` ;
- plusieurs répertoires dSYM macOS ajoutés ;
- `liblumvorax.so` modifiée ;
- `ns_conv`, `ns_lyapunov`, `nx42_v2` et `test_lid_driven` présents comme artefacts binaires ;
- plusieurs sources supplémentaires modifiées, notamment `lum_btree.c`, `lum_memory_tracer.c`, `lum_query.c`, `test_forensic_complete_system.c`, ainsi que les fichiers explicitement associés à C1/C2.

C'est-à-dire : le commit de correction est aussi un commit d'artefacts de compilation et de modifications annexes.

## Solution et suggestions

Le dépôt doit distinguer strictement :

1. les sources ;
2. les rapports ;
3. les tests reproductibles ;
4. les artefacts de build temporaires.

Suggestion immédiate : auditer le contenu de `.gitignore` et décider explicitement si les exécutables, `.o`, dSYM et bibliothèques générées doivent être versionnés. Pour une chaîne de build reproductible, leur présence doit être volontaire, documentée et reproductible.

**Statut : OPEN — nouvel axe ART-001.**

---

# 3. C1 — ALLOCATION ALIGNÉE

## Processus

`aligned_alloc()` appartient au standard C11. La documentation de référence confirme qu'il s'agit d'une fonction introduite en C11 et que la taille demandée doit être un multiple de l'alignement. citeturn0search0turn0search4

Le HEAD courant utilise désormais `posix_memalign()` comme chemin primaire et ne compile l'appel `aligned_alloc()` que lorsque `__STDC_VERSION__ >= 201112L`.

POSIX spécifie également les contraintes d'alignement de `posix_memalign()` : puissance de deux et multiple de `sizeof(void*)`. citeturn0search3turn0search6

## Problème

La correction élimine bien l'appel direct C11 dans le chemin C99 du wrapper.

Cependant, le rapport 141 affirme que cette modification constitue une preuve complète de portabilité macOS/Linux. Ce n'est pas encore démontré par le dépôt :

- aucun artefact CI macOS vérifié n'est présent dans les statuts GitHub observables ;
- le Makefile choisit encore `-march=native`, ce qui rend la portabilité du binaire dépendante du processeur de construction ;
- la présence de `posix_memalign()` dépend elle aussi de l'environnement POSIX exposé au préprocesseur.

C'est-à-dire : la correction supprime l'erreur observée, mais elle ne constitue pas à elle seule une certification de portabilité multi-machine.

## Solution et suggestions

Ajouter une validation réellement reproductible au moins sur :

- Linux x86-64 ;
- macOS/Clang ;
- configuration C99 stricte ;
- configuration sans AVX-512 disponible.

Tester séparément le wrapper pour les alignements 8, 16, 32, 64 et les tailles proches des multiples d'alignement, ainsi que le cas d'overflow.

**Statut C1 : CORRIGÉE SUR LE CODE, PREUVE DE PORTABILITÉ COMPLÈTE NON ÉTABLIE.**

---

# 4. C2 — FORMATAGE uint64_t

## Processus

Le format correct d'un `uint64_t` dépend de sa représentation effective sur la plateforme. L'utilisation de `PRIu64` via `<inttypes.h>` permet de déléguer le format exact à l'implémentation C.

Le HEAD contient désormais plusieurs corrections de ce type.

## Problème

Le rapport 141 indique « 10 fichiers » puis fournit une table contenant **13 fichiers**.

C'est une incohérence documentaire simple mais objective.

De plus, la présence de corrections dans le commit ne prouve pas qu'aucun autre `%lu` incorrect ne subsiste dans l'ensemble du dépôt.

## Solution et suggestions

Faire un scan global du dépôt pour les couples :

- `uint64_t` + `%lu` ;
- `uint64_t` + casts incompatibles ;
- `PRIu64` utilisé sans `<inttypes.h>`.

Puis ajouter un test de compilation avec Clang/macOS.

**Statut : CORRECTION C2 PRÉSENTE ; AUDIT GLOBAL DES FORMATS À FINALISER.**

---

# 5. MK-003 / MK-004 — MAKEFILE

## Processus

Le Makefile actuel distingue Linux et les autres systèmes via `uname -s`.

Linux conserve :

- `-lrt` ;
- `-Wl,-z,stack-size=16777216` ;
- `-lmvec` pour la cible d'intégration.

La branche non-Linux retire ces options et ajoute `-D_DARWIN_C_SOURCE`.

## Problème

Les corrections MK-003 et MK-004 sont présentes.

Mais `-march=native` reste dans les deux branches.

C'est-à-dire : le linker est mieux séparé par système, mais le binaire reste optimisé pour la machine qui compile. Cela n'est pas une erreur de compilation, mais c'est un facteur de reproductibilité et de portabilité du binaire.

Par ailleurs, le Makefile principal ne compile pas `src/blockchain_lumvorax/block_header.c`. Le fait que `make` produise quatre exécutables ne constitue donc pas une preuve que le sous-système blockchain contenant le SHA-256 stub est compilé et testé.

## Solution et suggestions

Séparer :

- portabilité de compilation ;
- compatibilité ISA ;
- couverture des modules ;
- reproductibilité des binaires.

Le build de certification doit déclarer explicitement quels sous-systèmes sont compilés et lesquels sont exclus.

**MK-003 : CORRIGÉE.**  
**MK-004 : CORRIGÉE.**  
**Portabilité ISA globale : OPEN, rattachée à BL-008.**

---

# 6. PREUVE DE BUILD — RECLASSIFICATION OBLIGATOIRE

## Processus

Une preuve de build fiable doit être rattachée à une commande, un environnement, un compilateur, des options et un résultat vérifiable.

## Problème

Le rapport 141 affirme :

« make clean && make → 0 erreur, 0 warning ».

L'état GitHub actuellement observable ne fournit pas de check CI C confirmant cette affirmation.

Le statut exposé pour `a1957b7` est **Vercel : failure**. Ce check n'est pas une preuve de compilation C du projet.

Le rapport 141 contient donc une observation locale rapportée, mais pas une preuve distante indépendante.

## Solution et suggestions

Pour passer cette assertion en preuve :

1. enregistrer le compilateur et sa version ;
2. enregistrer l'OS ;
3. exécuter `make clean && make` ;
4. conserver stdout/stderr ;
5. exécuter les tests produits ;
6. enregistrer les codes de retour ;
7. rattacher les résultats au commit exact ;
8. répéter sur Linux et macOS si la portabilité est revendiquée.

**Statut : OPEN — BUILD-PROOF-001.**

---

# 7. FL-001 — individual_log THREAD-SAFETY

## Processus

`forensic_log_individual_lum()` contient toujours un `static FILE* individual_log`.

Le premier thread qui constate que le pointeur est nul ouvre le fichier. Les appels suivants réutilisent ce flux.

## Problème

Aucune protection mutex n'entoure :

- le test `if (!individual_log)` ;
- l'ouverture ;
- l'affectation ;
- les écritures concurrentes ;
- la fermeture.

Deux threads peuvent donc initialiser simultanément le flux ou écrire concurremment.

C'est-à-dire : le mot « static » donne une durée de vie globale, mais ne fournit aucune synchronisation.

## Solution et suggestions

Utiliser un mutex dédié au flux individuel, ou une initialisation `pthread_once` suivie d'un verrou d'écriture.

Ajouter un test multi-thread qui force simultanément l'entrée dans `forensic_log_individual_lum()` et vérifie :

- un seul fichier ouvert ;
- aucune ligne tronquée ;
- aucun interleave illisible ;
- fermeture propre.

**Statut : OPEN P1.**

---

# 8. MT-004 — LEAK DETECTION

## Processus

Le tracker conserve un nombre d'allocations actives `g_count`, des octets cumulés alloués et des octets cumulés libérés.

## Problème

Le champ exporté `leak_detection` est calculé par :

`total_allocated > total_freed`.

C'est-à-dire : une différence positive entre les octets historiques signifie seulement qu'il y a eu plus d'octets alloués que libérés à l'instant du calcul. Ce n'est pas une preuve directe de fuite, notamment dans un système qui contient encore des allocations légitimes vivantes.

Le registre MT-004 reste donc justifié.

## Solution et suggestions

Utiliser explicitement :

- nombre d'allocations actives ;
- somme des tailles actives ;
- liste des allocations actives ;
- état de fermeture du sous-système.

Le champ `leak_detection` doit être dérivé de l'état actif, pas seulement de compteurs historiques.

**Statut : OPEN P1.**

---

# 9. CT-003 — LIMITE MÉMOIRE HARDCODÉE

## Processus

`common_types.h` définit `REPLIT_MEMORY_LIMIT_MB 768`.

## Problème

La limite mémoire d'un environnement d'exécution devient une constante compilée.

C'est-à-dire : un conteneur possédant une limite différente peut appliquer une valeur de sécurité incorrecte.

## Solution et suggestions

Préférer une valeur runtime issue de l'environnement réel, avec :

- détection ;
- valeur par défaut ;
- borne minimale ;
- borne maximale ;
- journalisation de la valeur effectivement détectée.

**Statut : OPEN P2.**

---

# 10. BLOCKCHAIN — SHA-256 STUB CONFIRMÉ

## Processus

`src/blockchain_lumvorax/block_header.c` calcule le hash d'un header Bitcoin en appelant `sha256_stub()`.

## Problème

Le stub ignore les données d'entrée et remplit la sortie avec 32 octets nuls.

C'est-à-dire : le résultat n'est pas un SHA-256. Toute décision de difficulté basée sur ce résultat n'est donc pas une validation cryptographique réelle.

Le fichier n'est en outre pas inclus dans la liste principale `SOURCES` du Makefile courant.

## Solution et suggestions

Brancher une implémentation SHA-256 réellement vérifiée, puis tester :

- vecteurs SHA-256 connus ;
- double SHA-256 ;
- header Bitcoin de référence ;
- résultat attendu ;
- test de difficulté.

Ne pas déclarer le sous-système blockchain cryptographiquement fonctionnel tant que ces tests ne passent pas.

**Statut : OPEN P0/P1 selon usage effectif.**

---

# 11. NQUBIT — FAKE_SUPERPOSITION CONFIRMÉE

## Processus

Les variantes NQubit utilisent un PRNG classique et une fonction gaussienne pour produire `fake_superposition`.

## Problème

Ce mécanisme produit du bruit aléatoire classique ; il ne démontre pas une représentation d'état quantique, des amplitudes complexes, une évolution unitaire ou un échantillonnage de mesure quantique.

C'est-à-dire : le nom du module ne suffit pas à transformer un processus stochastique classique en simulation quantique.

## Solution et suggestions

Deux voies doivent être distinguées :

1. documenter explicitement le module comme modèle classique stochastique ;
2. ou implémenter un véritable simulateur d'état quantique avec vecteur d'amplitudes, portes unitaires et mesure probabiliste, accompagné de tests contre des cas analytiques connus.

**Statut : OPEN scientifique.**

---

# 12. NS-007 — REQUALIFICATION

## Processus

Le module audio actuel contient une FFT Cooley-Tukey radix-2 et vérifie que `n` est une puissance de deux. Il remplit également par zéro dans le chemin `audio_apply_fft_vorax()` lorsqu'une taille de FFT demandée dépasse la taille réelle des données.

## Problème

L'ancienne formulation « FFT sans traitement explicite des longueurs non puissance de deux » est trop générale pour être maintenue telle quelle.

Le code refuse explicitement une taille non puissance de deux.

Cela ne prouve pas encore que toutes les API d'entrée et tous les chemins sont correctement documentés, mais la prémisse « aucune vérification » est réfutée pour le chemin observé.

## Solution et suggestions

Reclasser NS-007 :

- **prémisse historique réfutée pour le chemin vérifié** ;
- vérifier néanmoins le comportement API attendu lorsqu'un utilisateur fournit une taille non puissance de deux ;
- ajouter un test de régression si cette restriction est volontaire.

**Statut : RÉFUTATION PARTIELLE — ne pas compter automatiquement comme anomalie ouverte.**

---

# 13. AUDIO — NOUVELLE OBSERVATION DE ROBUSTESSE

## Processus

Le buffer FFT est alloué à `processor->buffer_size`, alors que `audio_apply_fft_vorax()` accepte un `fft_size`.

## Problème

La fonction limite `actual_size` à `buffer_size`, mais écrit ensuite jusqu'à `fft_size` dans `processor->fft_real` et `processor->fft_imag`.

Si `fft_size > buffer_size`, la boucle de padding écrit au-delà de la taille allouée.

C'est-à-dire : le calcul du `actual_size` protège la lecture des LUMs, mais ne protège pas la capacité des buffers FFT.

## Solution et suggestions

Deux solutions cohérentes :

- refuser `fft_size > buffer_size` ;
- ou redimensionner/allouer les buffers selon `fft_size`.

Ajouter un test `fft_size > buffer_size` avec ASan/UBSan.

**Statut : NOUVEAU P1 — AUDIO-001.**

---

# 14. PERSISTANCE WAL — PREUVE DE DURABILITÉ

## Processus

La persistance écrit des fichiers et des enregistrements transactionnels avec `fwrite()`.

## Problème

Une écriture réussie par `fwrite()` ne constitue pas une preuve que les données sont physiquement durables après une coupure d'alimentation.

Le registre NS-012 reste donc ouvert concernant l'absence de preuve `fsync()` avant le commit logique.

## Solution et suggestions

Définir explicitement le contrat de durabilité :

- flush du flux ;
- synchronisation du descripteur ;
- commit ;
- éventuellement renommage atomique ;
- test de récupération après interruption.

**Statut : OPEN.**

---

# 15. CR-001 — SECRET KAGGLE

## Processus

Le registre historique contient une clé Kaggle partiellement exposée dans l'historique.

## Problème

Modifier ou supprimer le fichier courant ne rend pas automatiquement un secret historique sûr.

GitHub recommande de révoquer/faire tourner le secret en premier. GitHub indique également que Secret Scanning inspecte l'historique Git et recommande la rotation immédiate lorsqu'une fuite est détectée. citeturn0search5turn0search7turn0search16

## Solution et suggestions

La procédure de clôture reste :

1. révoquer/rotater la clé ;
2. vérifier les journaux du fournisseur ;
3. rechercher toutes les occurrences historiques ;
4. décider ensuite si la réécriture de l'historique est nécessaire ;
5. activer Secret Scanning/Push Protection lorsque disponible.

**Statut : OPEN P0.**

---

# 16. REGISTRE PERSISTANT — AUCUNE DISPARITION

Les éléments suivants restent obligatoirement ouverts ou à revalider :

### P0 / sécurité et intégrité
- CR-001A — secret historique.
- BUILD-PROOF-001 — absence de preuve CI C indépendante.
- Blockchain SHA-256 stub.

### P1
- BL-003 — contrôle de charge fixe.
- BL-004 — identifiants potentiellement indépendants.
- BL-005 — formats LUMT/LUML.
- BL-006 — checksum BYTE.
- BL-007 — couverture checksum HUGEPAGE.
- BL-008 — AVX-512 et `-march=native`.
- BL-009 — B-tree upsert.
- BL-010 — dirty flag catalogue.
- BL-011 — double visite B-tree.
- BL-012 — compression destructive.
- FL-001 — `individual_log` non thread-safe.
- MT-004 — leak detection.
- AUDIO-001 — dépassement potentiel de buffer FFT.
- NS-001 à NS-006, NS-008 à NS-012, sous réserve de revalidation.
- CT-001.
- LL-002, LL-004, LL-005.
- TT-001, TT-002.

### P2 / documentation et architecture
- CT-003.
- FL-004.
- IBM-001.
- IBM-002.
- CR-001B.
- Tâches C2/C3/C4 héritées.
- Richardson.
- T04 énergie.
- Lyapunov.
- NQubit.
- NX-42.
- blockchain.

### Statuts à conserver séparément
- NS-007 : prémisse historique partiellement réfutée.
- FL-003 : réfutée sur le HEAD observé.
- MK-003 : corrigée.
- MK-004 : corrigée.
- C1 : correction de code présente, preuve multi-plateforme encore à produire.
- C2 : corrections présentes, audit global des formats encore à terminer.

---

# 17. ORDRE DE TRAVAIL RECOMMANDÉ POUR LE PROCHAIN CYCLE

## Bloc A — preuves et sécurité

1. Clôturer CR-001 uniquement après rotation/révocation.
2. Établir une preuve de build reproductible.
3. Scanner l'historique pour secrets.
4. Décider du traitement des artefacts binaires.

## Bloc B — sécurité mémoire

5. Corriger AUDIO-001.
6. Corriger FL-001.
7. Corriger MT-004.
8. Revalider MT-001 après test d'exécution.

## Bloc C — scientificité

9. Revalider NS-001 à NS-012.
10. Refaire Richardson avec erreur réellement calculée.
11. Revalider T04 avec invariant énergétique explicite.
12. Revalider Lyapunov avec étude de sensibilité.
13. Requalifier NX-42 problème par problème.
14. Requalifier NQubit comme modèle classique ou implémenter un simulateur quantique réel.

## Bloc D — cryptographie et BIT-LUM

15. Remplacer le SHA-256 stub.
16. Tester les vecteurs cryptographiques.
17. Traiter BL-003 à BL-012.
18. Séparer clairement portabilité ISA et optimisation locale.

## Bloc E — hygiène dépôt

19. Déterminer la politique de versionnement des `.o`, exécutables, dSYM et bibliothèques.
20. Vérifier que les rapports reflètent exactement la portée des commits.
21. Ne jamais utiliser un build local rapporté comme preuve CI indépendante.

---

# 18. CRITÈRE DE CLÔTURE « SANS EXCEPTION »

Une anomalie est **CORRIGÉE** uniquement si :

1. le défaut n'existe plus dans le code courant ;
2. un test reproductible démontre le comportement attendu ;
3. le test échoue avec l'ancien comportement ou une preuve équivalente existe ;
4. le commit, fichier, test et résultat sont traçables.

Une anomalie est **RÉFUTÉE** uniquement si la prémisse initiale est démontrée fausse sur le HEAD courant.

Une anomalie de sécurité historique n'est pas clôturée par la suppression du fichier courant lorsque le secret a déjà été publié : la rotation/révocation reste prioritaire. citeturn0search7

---

# 19. ÉTAT FINAL

**CERTIFIED_100 = false.**

Le bilan « Avancement : 100 % » doit être interprété uniquement comme :

**les corrections annoncées dans le périmètre C1/C2/MK ont été intégrées au code.**

Il ne signifie pas :

**100 % des anomalies du dépôt sont corrigées et prouvées.**

Le nouvel audit révèle en outre :

- une portée de commit `5929978` beaucoup plus large que le résumé 141 ;
- des artefacts de build versionnés ;
- l'absence de preuve CI C indépendante ;
- FL-001 toujours confirmée ;
- MT-004 toujours confirmée ;
- CT-003 toujours confirmée ;
- SHA-256 stub toujours présent ;
- NQubit toujours classique/stochastique ;
- un nouveau risque potentiel de dépassement de buffer dans le chemin FFT ;
- NS-007 à requalifier plutôt qu'à maintenir aveuglément comme anomalie.

**Aucune tâche antérieure n'est supprimée du registre.**

---

**Rapport 142 — audit croisé post-141 — HEAD distant `a1957b7` — CERTIFIED_100=false.**
