# RAPPORT 146 — AUDIT CRITIQUE POST-145 : VALIDATION DES CORRECTIONS, BL-013 ET NOUVEAUX ÉCARTS DE PORTABILITÉ/CONCURRENCE

**Date :** 2026-10-02  
**Dépôt :** vgacofc/lumvorax2  
**HEAD audité :** b6da25aa9b5e18a814255f500ca9a73a7f8ede9f  
**HEAD précédent :** 640e9c82d1e1746c788888c246558237cc452bc9  
**Statut global :** CERTIFIED_100=false  
**Règle :** aucun code source modifié par cet audit ; seul ce rapport est ajouté.

---

## 1. EXPERTISES ACTIVÉES

1. Forensique Git/GitHub et traçabilité des changements.
2. C99/C11, ABI et validation des types.
3. GNU Make, graphe de dépendances et reproductibilité du build.
4. Portabilité ISA et contrôle des instructions CPU.
5. Pthreads, concurrence et races de données.
6. Instrumentation mémoire et cohérence des snapshots.
7. Cryptographie appliquée et validation SHA-256.
8. Conception de tests et qualité des vecteurs de référence.
9. Sécurité mémoire et intégrité des structures binaires.
10. Analyse blockchain/PoW et sérialisation des headers.
11. Validation scientifique et conservation du registre NS/NQubit.
12. Logging, persistance et intégrité forensique.
13. Hygiène des artefacts et séparation source/build.
14. Sécurité des secrets et gestion de CR-001.

---

# 2. SYNCHRONISATION DISTANTE

## Processus

L'état annoncé par la session 145 est confronté directement au commit b6da25a et à son delta depuis le rapport 144.

## Résultat

GitHub confirme :

- HEAD : b6da25aa9b5e18a814255f500ca9a73a7f8ede9f ;
- exactement un commit depuis 640e9c82 ;
- Makefile modifié ;
- common_types.h modifié ;
- forensic_logger.c modifié ;
- memory_tracker.c modifié ;
- test_blockchain_sha256.c ajouté ;
- rapport 145 ajouté.

Le message du commit correspond bien aux corrections annoncées.

## Problème

Le statut CI GitHub observé sur b6da25a reste Vercel = failure.

C'est-à-dire : les résultats « make clean && make = 0 warning / 0 erreur » et « 9/9 PASS » sont des preuves annoncées par la session, et le code/test correspondants existent bien dans le dépôt, mais le statut CI distant ne fournit toujours pas une validation indépendante du build C.

## Solution

Conserver deux niveaux de preuve :

- preuve d'exécution locale/session ;
- preuve CI indépendante.

**Statut : BUILD-PROOF-001 = OPEN.**

---

# 3. CT-003 v2 — CORRECTION CONFIRMÉE

## Processus

La valeur de mémoire Replit est maintenant récupérée à l'exécution avec getenv() puis convertie avec strtol(). La conversion utilise errno et endptr pour détecter les dépassements, les entrées vides et les caractères résiduels.

## Problème

L'ancien macro public REPLIT_MEMORY_LIMIT_MB a effectivement disparu de common_types.h.

Le fallback 768 reste présent sous le nom explicite REPLIT_MEMORY_LIMIT_MB_DEFAULT.

C'est-à-dire : 768 n'est plus présenté comme la valeur réelle universelle ; il est maintenant uniquement le fallback lorsque l'environnement ne fournit pas une valeur valide.

## Solution

La correction peut être considérée comme validée pour le défaut CT-003 initial.

Il reste toutefois à tester explicitement :

- variable absente ;
- 512 ;
- 768 ;
- 2048 ;
- 65536 ;
- 0 ;
- négatif ;
- texte ;
- valeur avec suffixe ;
- dépassement de long.

**Statut : CT-003 = CORRIGÉ.**

**ART-CT003 = OPEN**, car les copies historiques/artefacts ne sont pas la source active.

---

# 4. MT-004 — SNAPSHOT ATOMIQUE CONFIRMÉ

## Processus

Les compteurs sont maintenant copiés sous g_tracker_mutex dans des variables locales avant génération du JSON.

## Problème

Cette modification empêche qu'un autre thread modifie les quatre compteurs au milieu de leur capture.

C'est-à-dire : le fichier JSON correspond désormais à un snapshot cohérent des compteurs au moment où le verrou est détenu.

## Solution

Le défaut historique est corrigé.

Une validation supplémentaire reste souhaitable avec un test concurrent massif, mais elle ne doit pas empêcher de reconnaître la correction du défaut identifié.

**Statut : MT-004 = CORRIGÉ.**

---

# 5. FL-001 v2 — CORRECTION INCOMPLÈTE : ACCÈS RESTANT HORS MUTEX GLOBAL

## Processus

Le fichier forensic_log_file est une ressource globale partagée. Toutes les opérations qui lisent, écrivent, ferment ou remplacent ce pointeur doivent être coordonnées par le même verrou.

## Problème

Le mutex fl001_log_file_mutex protège effectivement :

- forensic_logger_init ;
- forensic_log_memory_operation ;
- forensic_log_lum_operation ;
- forensic_logger_destroy ;
- forensic_log ;
- unified_forensic_log.

Mais forensic_log_individual_lum() contient encore, avant son mutex fl001_individual_mutex :

- un test direct de forensic_log_file ;
- un fprintf direct vers forensic_log_file ;
- un fflush direct de forensic_log_file.

C'est-à-dire : la session 145 affirme que le mutex global couvre tous les accès à forensic_log_file, mais ce n'est pas exact. Une septième fonction conserve un accès direct non protégé.

Il existe alors une interférence possible :

1. Thread A entre dans forensic_log_individual_lum().
2. Thread A teste forensic_log_file.
3. Thread B appelle forensic_logger_destroy().
4. Thread B verrouille fl001_log_file_mutex, ferme le FILE et met forensic_log_file à NULL.
5. Thread A peut ensuite utiliser l'ancien pointeur.

Le risque est une utilisation après fermeture et/ou une race de pointeur.

## Solution

Le bloc de forensic_log_individual_lum() qui utilise forensic_log_file doit être placé sous fl001_log_file_mutex.

La meilleure architecture est :

- verrou global pour forensic_log_file ;
- verrou individuel uniquement pour individual_log ;
- aucun accès direct au FILE global hors du verrou global.

Ajouter un test avec :

- plusieurs threads appelant forensic_log_individual_lum ;
- plusieurs threads appelant forensic_log ;
- un thread exécutant init/destroy ;
- vérification sous ThreadSanitizer lorsque disponible.

**Statut : FL-001 = PARTIELLEMENT CORRIGÉ, PAS CLOS.**

### Nouvelle sous-anomalie

**FL-005 — accès concurrent résiduel à forensic_log_file dans forensic_log_individual_lum().**

**Priorité : P1.**

---

# 6. SHA-256 — BUILD PROOF AMÉLIORÉ ET TESTS RÉELS

## Processus

La cible blockchain_test compile explicitement :

- sha256_mini.c ;
- block_header.c ;
- test_blockchain_sha256.c.

Elle produit ensuite test_blockchain_sha256 et utilise nm pour vérifier la présence des symboles sha256_lumvorax et block_header_hash.

## Problème

Cette construction est beaucoup plus forte que la situation du rapport 144.

Elle prouve que ces deux implémentations sont liées au binaire du test.

Elle ne prouve cependant pas encore que le blockchain module est intégré au binaire principal lum_vorax_complete, ce qui n'est d'ailleurs pas nécessairement souhaité puisque le Makefile le décrit maintenant comme module indépendant.

## Solution

La séparation est acceptable si elle est intentionnelle et documentée.

Il faut simplement distinguer :

- blockchain_test intégré et prouvé ;
- produit principal ne contenant pas nécessairement le blockchain module.

**Statut : SHA-256 BUILD-PROOF = CORRIGÉ pour la cible dédiée.**

---

# 7. TESTS SHA-256 — 9/9 EST UNE PREUVE UTILE, MAIS PAS UNE VALIDATION CRYPTOGRAPHIQUE COMPLÈTE

## Processus

Les tests contiennent des vecteurs NIST pour SHA-256("abc"), SHA-256(message vide) et un message de 448 bits.

NIST FIPS 180-4 spécifie SHA-256 comme fonction produisant un digest de 256 bits et décrit les propriétés d'intégrité des fonctions de hachage. citeturn0search0turn0search13

## Problème

Les tests T01, T02 et T07 sont de véritables comparaisons avec des valeurs de référence.

En revanche, T03 est décrit comme reproduisant le comportement Bitcoin mais ne compare pas le double-SHA256 à un vecteur attendu : il vérifie seulement que le second résultat diffère du premier et qu'il n'est pas nul.

C'est-à-dire : T03 prouve une transformation non triviale, pas l'exactitude cryptographique du double-SHA256.

De même, T04 vérifie seulement « non nul », ce qui ne démontre pas une valeur correcte.

## Solution

Ajouter des vecteurs attendus :

- double-SHA256 d'un message connu ;
- block_header_hash d'un header canonique dont le résultat attendu est enregistré ;
- test d'endianess ;
- test de sérialisation exacte du header ;
- comparaison indépendante avec une implémentation de référence.

**Statut : SHA-256 primitive = bien étayée ; validation fonctionnelle blockchain = OPEN.**

---

# 8. BL-013 — ANOMALIE CONFIRMÉE ET SCIENTIFIQUEMENT IMPORTANTE

## Processus

Le test vérifie les offsets réels de block_header_t et constate que :

- version commence à 0 ;
- prev_hash à 4 ;
- merkle_root à 36 ;
- timestamp à 72 ;
- bits à 80 ;
- nonce à 88.

La fonction hashée utilise seulement les 80 premiers octets.

## Problème

bits et nonce sont donc hors de la fenêtre.

Le test T06b confirme expérimentalement le problème : modifier nonce de 42 à 999 ne modifie pas le hash.

C'est-à-dire : deux headers ayant des nonce différents peuvent actuellement produire exactement le même block_header_hash().

Pour un mécanisme de preuve de travail dépendant du nonce, c'est une anomalie fonctionnelle majeure.

## Solution

Il faut choisir explicitement une architecture de format :

### Option A — header canonique compact

Redéfinir le format pour que les champs destinés au PoW se trouvent dans la zone hashée.

### Option B — longueur de hash explicite

Hasher la représentation sérialisée complète et définie du header, au lieu de supposer 80 octets.

### Option C — domaine de hash ARTCB spécifique

Définir une sérialisation binaire normative :

- ordre des champs ;
- largeur de chaque champ ;
- endianess ;
- longueur ;
- version du format ;
- domaine de hachage.

Cette option est préférable si le format LUMVORAX n'a pas vocation à être compatible avec Bitcoin.

Il ne faut surtout pas corriger BL-013 en augmentant arbitrairement 80 vers 96 sans spécification du protocole.

**Statut : BL-013 = OPEN P1.**

---

# 9. BL-008 — « MAKE PORTABLE » AJOUTÉ, MAIS LA CIBLE N'EST PAS ENCORE UNE PREUVE DE PORTABILITÉ

## Processus

Le Makefile définit CFLAGS_PORTABLE sans -march=native.

L'intention est correcte : compiler une variante avec un ISA baseline.

## Problème

La cible portable commence par :

- utiliser $(SOURCES:.c=.o) ;
- produire un lien avec ces objets existants ;
- ne recompile en portable qu'en cas d'échec du premier lien.

Or les objets .o existants sont normalement générés avec CFLAGS, et CFLAGS contient encore -march=native.

C'est-à-dire : si ces objets natifs existent déjà et que le premier lien réussit, la cible appelée « portable » peut être construite avec des objets compilés pour la machine locale.

La cible peut donc annoncer une portabilité qu'elle n'a pas réellement obtenue.

## Solution

La cible portable doit :

1. utiliser un répertoire d'objets distinct ;
2. compiler chaque source avec CFLAGS_PORTABLE ;
3. ne jamais réutiliser les .o produits par le build natif ;
4. lier uniquement ces objets portables ;
5. vérifier éventuellement le binaire avec objdump/readelf ou un test sur une machine baseline.

Architecture recommandée :

- build/obj/native/...
- build/obj/portable/...

Le test de portabilité doit ensuite démontrer que le binaire fonctionne sur une cible sans AVX-512.

**Statut : BL-008 = PARTIELLEMENT CORRIGÉ.**

### Nouvelle sous-anomalie

**BL-014 — cible portable susceptible de réutiliser des objets compilés avec -march=native.**

**Priorité : P1.**

---

# 10. MAKEFILE — AUTRE POINT DE REPRODUCTIBILITÉ

## Processus

Un build reproductible doit avoir des frontières claires entre objets issus de configurations différentes.

## Problème

Les objets sont directement placés à côté des sources sous src/, et le même nom d'objet peut être partagé entre build natif et build portable.

C'est-à-dire : même indépendamment du défaut logique de la cible portable, l'arbre de build n'isole pas les configurations.

## Solution

Utiliser des répertoires d'objets séparés par configuration.

Cette correction supprimera simultanément :

- le risque de mélange natif/portable ;
- les rebuilds ambigus ;
- les artefacts difficiles à attribuer ;
- une partie du risque ART-001.

**Statut : BUILD-PORT-002 = OPEN.**

---

# 11. CT-003 — ART-CT003 RESTE DISTINCT

## Processus

Le fichier source canonique est corrigé.

## Problème

Les copies historiques de common_types.h et artefacts ne sont pas modifiés.

C'est-à-dire : il ne faut pas les modifier simplement pour faire disparaître l'anomalie documentaire. Leur rôle doit être déterminé.

## Solution

Classifier les copies :

- source active ;
- copie générée ;
- snapshot historique ;
- artefact de test ;
- fichier inutilisé.

Puis ne modifier que les fichiers réellement actifs si nécessaire.

**Statut : ART-CT003 = OPEN.**

---

# 12. BUILD-PROOF-001 — TOUJOURS OPEN

## Processus

Le dépôt prouve maintenant davantage de choses localement :

- build principal annoncé ;
- blockchain_test annoncé ;
- 9/9 annoncé ;
- nm confirme les symboles.

## Problème

Le statut GitHub disponible sur b6da25a est Vercel en échec, et non une CI C reproductible.

C'est-à-dire : aucun pipeline indépendant ne démontre actuellement les affirmations de compilation GCC/Linux/macOS.

## Solution

Ajouter une matrice CI :

- Linux + GCC ;
- Linux + Clang ;
- macOS + Clang ;
- build natif ;
- build portable ;
- blockchain_test ;
- tests unitaires ;
- tests concurrence ;
- tests mémoire.

**Statut : BUILD-PROOF-001 = OPEN P1.**

---

# 13. REGISTRE PERSISTANT APRÈS SESSION 145

## Sécurité P0

- CR-001 — secret Kaggle historique potentiellement exposé.
- CR-001A — rotation/révocation.
- CR-001B — traitement historique Git.

## BIT-LUM/VORAX

- BL-003 à BL-007 — OPEN.
- BL-008 — partiellement corrigé ; portable à fiabiliser.
- BL-009 à BL-012 — OPEN.
- BL-013 — nonce/bits hors fenêtre hashée.
- BL-014 — cible portable pouvant réutiliser des objets natifs.

## Navier-Stokes / scientifique

- NS-001 à NS-012 — OPEN.
- Richardson convergence — OPEN.
- T04 énergie — OPEN.
- Lyapunov — OPEN.
- NX-42 — OPEN.
- C2/C3/C4 historiques — OPEN.

## NQubit

- modèle fake_superposition / RNG classique — OPEN scientifique.

## Concurrence

- MT-004 — CORRIGÉ.
- FL-001 — PARTIELLEMENT CORRIGÉ.
- FL-005 — OPEN.
- CT-001 — OPEN.

## Logging

- LL-002 — OPEN.
- LL-004 — OPEN.
- LL-005 — OPEN.

## IBM

- IBM-001 — OPEN.
- IBM-002 — OPEN.

## Tests/build

- TT-001 — OPEN.
- TT-002 — OPEN.
- BUILD-PROOF-001 — OPEN.
- BUILD-PORT-002 — OPEN.

## Hygiène

- ART-001 — OPEN.
- ART-CT003 — OPEN.

---

# 14. CRITÈRE DE CLÔTURE RENFORCÉ

À partir de ce rapport, une anomalie technique ne sera considérée comme définitivement close que si :

1. la modification existe dans le code actif ;
2. le code appartient au graphe de build concerné ;
3. un test reproductible exerce réellement le chemin corrigé ;
4. une référence attendue ou un invariant permet de détecter une régression ;
5. la configuration de build utilisée par le test est explicitement identifiée ;
6. lorsqu'il s'agit de concurrence, un test multi-thread est fourni ;
7. lorsqu'il s'agit de cryptographie, un vecteur externe de référence est utilisé ;
8. lorsqu'il s'agit de portabilité, les objets eux-mêmes sont compilés avec la configuration portable ;
9. lorsqu'il s'agit d'un protocole, la sérialisation et les offsets sont spécifiés avant la correction.

---

# 15. ÉTAT FINAL

### Clôtures confirmées par le code

- CT-003 : CORRIGÉ.
- MT-004 : CORRIGÉ.
- SHA-256 build dédié : CORRIGÉ / PROUVÉ au niveau de la cible dédiée.

### Clôtures seulement partielles

- FL-001 : mutex global ajouté mais accès résiduel dans forensic_log_individual_lum().
- BL-008 : infrastructure portable ajoutée, mais isolation des objets insuffisante.
- SHA-256 : primitive testée avec plusieurs vecteurs, mais validation complète du protocole blockchain encore ouverte.

### Nouvelles anomalies

- FL-005 — accès non protégé à forensic_log_file dans forensic_log_individual_lum().
- BL-014 — cible portable pouvant réutiliser des objets compilés avec -march=native.
- BUILD-PORT-002 — absence d'isolation native/portable des répertoires d'objets.

### Anomalie confirmée

- BL-013 — nonce et bits hors fenêtre des 80 octets hashés ; T06b démontre que modifier le nonce ne modifie pas le hash.

### Toujours P0

- CR-001.

**CERTIFIED_100 = false.**

Le résultat de la session 145 est donc substantiel et améliore nettement la qualité des preuves, mais il ne permet pas encore de passer à une certification globale. Le registre historique est conservé sans suppression des chantiers non démontrés.

---

## 16. VALIDATION EXTERNE — SHA-256

NIST FIPS 180-4 définit SHA-256 parmi les fonctions du Secure Hash Standard et précise son digest de 256 bits. Les vecteurs NIST utilisés dans le dépôt constituent donc une référence externe pertinente pour les tests de la primitive. citeturn0search0turn0search13

**Fin du rapport 146.**
