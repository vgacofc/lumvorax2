# Rapport 148 — Audit critique post-session 147 : FL-005, BL-014 et cohérence cryptographique du header

**Date :** 2026-10-02  
**Session auditée :** 147  
**HEAD distant audité :** `8d29abb8e97061afa20c3ebfcac440790823e6b9`  
**HEAD précédent :** `b8cb83ee0bb329917675e41f4bdb977a684328e0`  
**Repository :** `vgacofc/lumvorax2` / branche `main`  
**CERTIFIED_100=false**  
**unique_human_proven=false**

---

## 1. Expertises activées

- Audit forensique C/C99 et gestion du cycle de vie des ressources.
- Concurrence POSIX/pthreads, mutex, races et use-after-close/use-after-free logique.
- Ingénierie des systèmes de build GNU Make et séparation native/portable.
- Cryptographie appliquée : SHA-256, double-SHA-256, vecteurs de référence.
- Conception de protocoles blockchain et sérialisation canonique de headers.
- Analyse PoW : nonce, difficulté, représentation du header et déterminisme inter-machine.
- Portabilité ABI/endianness/alignment/padding C.
- Validation scientifique et reproductibilité des tests.
- Audit de traçabilité et conservation du registre historique.

---

# 2. Synchronisation distante

Le commit `8d29abb` existe bien sur le dépôt distant.

La comparaison exacte `b8cb83e... → 8d29abb` contient quatre catégories de modifications :

1. `Makefile`
2. `src/debug/forensic_logger.c`
3. `src/tests/test_blockchain_sha256.c`
4. `RAPPORT/147_CORRECTIONS_POST146_FL005_BL014_VECTEURS_T03T04_20261002.md`

Aucune autre modification source n'est apparue dans ce commit.

Le commit est donc cohérent avec l'annonce de la session 147 sur son périmètre.

---

# 3. CI distante : résultat à ne pas confondre avec le build local

Le statut GitHub actuellement visible pour `8d29abb` contient :

- `Vercel` : **failure**

Aucune exécution GitHub Actions C dédiée n'est associée à ce commit par le connecteur disponible.

## Processus

Un build annoncé par l'opérateur comme réussi signifie qu'une exécution locale de `make` a abouti.

## Problème

Cela ne constitue pas une preuve indépendante fournie par CI.

Le statut Vercel en échec n'est pas une preuve que le code C échoue : Vercel est un contrôle différent. Mais inversement, ce statut ne permet pas de certifier le build C.

## Solution / suggestion

Conserver séparément :

- preuve de build local ;
- preuve du test blockchain local ;
- preuve CI indépendante ;
- preuve de portabilité sur une machine/ABI indépendante.

**BUILD-PROOF-001 reste donc OPEN.**

---

# 4. FL-005 — Correction annoncée : réévaluation nécessaire

## Processus

La session 147 a remplacé l'accès direct au global `forensic_log_file` dans `forensic_log_individual_lum()` par :

1. acquisition de `fl001_log_file_mutex` ;
2. copie du pointeur dans `log_snapshot` ;
3. libération du mutex ;
4. écriture via `log_snapshot`.

L'idée est de ne plus lire le global sans verrou.

## Problème

La propriété revendiquée dans le rapport 147 est trop forte.

Un pointeur `FILE *` copié n'est pas une référence qui maintient le fichier ouvert.

Séquence possible :

1. Thread A verrouille `fl001_log_file_mutex`.
2. Thread A copie `forensic_log_file` dans `log_snapshot`.
3. Thread A libère `fl001_log_file_mutex`.
4. Thread B appelle `forensic_logger_destroy()`.
5. Thread B acquiert le même mutex.
6. Thread B écrit la fin du journal.
7. Thread B appelle `fclose(forensic_log_file)`.
8. Thread B place `forensic_log_file = NULL`.
9. Thread B libère le mutex.
10. Thread A utilise maintenant `log_snapshot`, qui désigne le FILE déjà fermé.

Donc le snapshot protège la **lecture atomique du pointeur**, mais pas la **durée de vie de l'objet FILE**.

C'est-à-dire : le pointeur peut être correctement copié au moment T0 puis devenir invalide au moment T1.

La phrase du rapport 147 selon laquelle le snapshot reste valide après `forensic_logger_destroy()` n'est donc pas démontrée et est incorrecte au niveau du modèle de durée de vie.

### Classification

**FL-005 : OPEN — P1**

La correction de la lecture non protégée est réelle, mais la race de durée de vie reste possible.

## Solution / suggestions

La solution robuste est de coordonner le cycle de vie et toutes les écritures.

Deux architectures sont possibles :

### Option A — conserver le mutex pendant toute l'I/O

`forensic_log_individual_lum()` doit :

1. prendre `fl001_log_file_mutex` ;
2. vérifier `forensic_log_file` ;
3. écrire ;
4. flush ;
5. relâcher le mutex.

Ainsi `forensic_logger_destroy()` ne peut pas fermer le FILE pendant l'écriture.

C'est la correction la plus simple à auditer.

### Option B — système de références / état de fermeture

Un mécanisme plus complexe peut permettre de libérer le mutex pendant l'I/O, mais il faut alors une véritable gestion de durée de vie : compteur de références, état closing, condition variable ou équivalent.

Une simple copie de `FILE *` ne suffit pas.

### Test de clôture recommandé

Un test concurrent doit lancer simultanément :

- plusieurs producteurs de logs ;
- plusieurs appels à `forensic_log_individual_lum()` ;
- des cycles `init → log → destroy`.

La validation doit être réalisée sous ThreadSanitizer lorsque disponible.

---

# 5. FL-001 — état global du logger

## Processus

Le mutex `fl001_log_file_mutex` protège maintenant correctement les fonctions qui utilisent directement `forensic_log_file`, notamment :

- `forensic_logger_init()` ;
- `forensic_logger_destroy()` ;
- `forensic_log_memory_operation()` ;
- `forensic_log_lum_operation()` ;
- `forensic_log()` ;
- `unified_forensic_log()`.

## Problème

FL-005 montre cependant que « accès sous mutex » doit signifier :

> le mutex couvre toute la durée pendant laquelle la ressource protégée est utilisée.

Un verrou uniquement autour de la lecture du pointeur ne protège pas la durée de vie du FILE.

## Solution

**FL-001 reste OPEN pour clôture définitive**, jusqu'à preuve par test concurrent et correction du cycle de vie.

Le sous-problème FL-005 est conservé dans le registre au lieu d'être effacé.

---

# 6. BL-014 — isolation du build portable

## Processus

La session 147 remplace l'ancienne cible portable qui pouvait réutiliser les `.o` natifs.

La nouvelle architecture utilise :

- `build/obj/portable/` ;
- `PORTABLE_OBJECTS` ;
- une règle pattern dédiée ;
- `CFLAGS_PORTABLE` pour chaque compilation portable.

La chaîne portable ne référence plus directement `$(SOURCES:.c=.o)`.

## Problème

Le défaut précis identifié au rapport 146 était la possibilité de produire un binaire appelé « portable » en réutilisant des objets compilés avec `-march=native`.

Ce chemin n'existe plus dans la règle portable observée au HEAD `8d29abb`.

## Conclusion

**BL-014 : CLÔTURÉ sur le défaut identifié.**

La correction structurelle est bonne : la provenance des objets portables est maintenant distincte de la provenance native.

## Réserve

Cette clôture ne prouve pas encore :

- l'exécution sur CPU sans AVX-512 ;
- la compatibilité sur une autre microarchitecture ;
- l'absence d'une instruction ISA non portable introduite indirectement ;
- une validation ABI indépendante.

Ces éléments restent dans **BUILD-PORT-002**.

---

# 7. BUILD-PORT-002 — état

## Processus

La séparation des objets réduit fortement le risque de contamination du build portable.

## Problème

Un build portable réussi sur la machine de développement ne prouve pas qu'il s'exécute réellement sur une machine dépourvue des extensions utilisées par le build natif.

## Solution / suggestion

Ajouter une validation indépendante :

1. compiler avec `CFLAGS_PORTABLE` depuis un arbre propre ;
2. inspecter les instructions générées ;
3. exécuter sur CPU cible réellement dépourvu des extensions avancées ;
4. éventuellement utiliser QEMU ou une machine CI x86-64 générique ;
5. conserver le résultat dans un rapport.

**BUILD-PORT-002 : OPEN.**

---

# 8. T03 — vecteur double-SHA-256

## Processus

Le test effectue maintenant :

1. SHA-256 de `abc` ;
2. SHA-256 du digest de 32 octets ;
3. comparaison exacte avec une constante de 32 octets.

La comparaison n'est donc plus seulement « différent » ou « non nul ».

## Conclusion

**T03 : RENFORCÉ et techniquement utile.**

Le test détecte maintenant une implémentation qui produit un digest incorrect mais non nul.

La constante revendiquée correspond au double-SHA-256 de `abc`.

---

# 9. T04 — vecteur du header : amélioration réelle mais portée limitée

## Processus

T04 compare maintenant exactement le résultat de `block_header_hash()` avec une constante.

C'est une amélioration importante par rapport à l'ancien test « digest non nul ».

## Problème

Le mot « canonique » est toutefois trop fort.

Le test utilise directement :

`block_header_hash(&hdr, out)`

et la fonction actuelle effectue :

- SHA-256 des 80 premiers octets de la représentation mémoire C ;
- puis SHA-256 du digest.

Cela signifie que le vecteur teste la représentation mémoire de la structure telle qu'elle existe sur l'environnement de compilation.

Ce n'est pas automatiquement une sérialisation de protocole canonique.

---

# 10. BL-013 — problème confirmé

## Processus

La structure `block_header_t` contient notamment :

- `version` ;
- `prev_hash` ;
- `merkle_root` ;
- `timestamp` ;
- `bits` ;
- `nonce` ;
- puis des extensions LUMVORAX.

La fonction `block_header_hash()` prend arbitrairement les **80 premiers octets de la représentation mémoire**.

## Problème

Avec la disposition C observée, les champs `bits` et `nonce` se trouvent après la fenêtre des 80 premiers octets.

Donc :

- modifier `nonce` ne modifie pas le digest calculé par `block_header_hash()` ;
- modifier `bits` ne modifie pas non plus le digest calculé par cette fonction.

Le test T06b de la session 147 documente explicitement ce comportement pour le nonce.

## Conclusion

**BL-013 : OPEN — P1**

Ce n'est pas un simple problème de test. C'est un problème de définition du message cryptographique.

---

# 11. Nouvelle anomalie BL-015 — divergence de sérialisation entre les chemins blockchain

## Processus

Deux chemins de calcul de hash existent dans le module blockchain.

### Chemin A — `block_header_hash()`

La fonction utilise directement :

`(const uint8_t *)h`

sur les 80 premiers octets de la structure C.

### Chemin B — `lumvorax_genesis_compute_hash()`

Cette fonction construit explicitement un buffer de 80 octets et place séparément :

- version ;
- prev_hash ;
- merkle_root ;
- timestamp ;
- bits ;
- nonce.

Elle utilise donc une représentation sérialisée différente.

## Problème

Les deux fonctions ne définissent pas le même message de 80 octets.

Dans `block_header_hash()`, la disposition dépend de la représentation mémoire C, y compris de son padding et de son endianness.

Dans `lumvorax_genesis_compute_hash()`, les champs sont copiés explicitement dans des offsets déterminés.

C'est-à-dire que deux fonctions prétendant calculer le hash du « header » peuvent calculer le hash de **deux séquences d'octets différentes**.

Cette situation est particulièrement grave pour une blockchain : le hash utilisé pour une validation, un genesis, un mineur ou une vérification de bloc doit être défini par une sérialisation unique et déterministe.

### Classification

**BL-015 : P1 — OPEN**

## Solution / suggestions

Définir une seule sérialisation normative du header.

Il faut décider explicitement :

1. largeur exacte de chaque champ ;
2. ordre des champs ;
3. endianess de chaque entier ;
4. longueur exacte de la partie soumise au SHA-256 ;
5. inclusion ou exclusion de `bits` ;
6. inclusion ou exclusion du nonce ;
7. traitement des extensions LUMVORAX ;
8. version du protocole.

Ensuite toutes les fonctions doivent utiliser cette même sérialisation.

La solution recommandée est une fonction unique de sérialisation vers un buffer d'octets, utilisée par :

- `block_header_hash()` ;
- genesis ;
- mining ;
- validation PoW ;
- tests ;
- éventuellement réseau et persistance.

---

# 12. Pourquoi BL-015 doit être traité avec BL-013

BL-013 dit :

> le nonce n'est pas dans les 80 octets actuellement hashés par `block_header_hash()`.

BL-015 dit :

> une autre fonction blockchain sérialise le nonce dans les 80 octets d'une autre manière.

Les deux anomalies sont donc liées.

Corriger uniquement l'une sans définir le protocole global créerait une architecture incohérente.

---

# 13. Endianness — risque toujours ouvert

## Processus

Un entier C comme `uint32_t` ou `uint64_t` possède une représentation mémoire dépendante de l'architecture.

## Problème

Caster directement une structure en `uint8_t *` signifie que le hash dépend potentiellement :

- du padding ;
- de l'endianess ;
- de l'ABI ;
- de l'alignement ;
- de la disposition exacte de la structure.

Un hash blockchain doit être reproductible indépendamment de ces détails.

## Solution

La sérialisation doit écrire explicitement chaque entier dans l'ordre de protocole choisi.

**SHA-256 blockchain : validation endianess toujours OPEN.**

---

# 14. T04 ne clôt donc pas BL-013

Le nouveau vecteur T04 prouve une propriété utile :

> pour cette représentation mémoire précise et cet environnement précis, `block_header_hash()` retourne la valeur attendue.

Il ne prouve pas :

> le protocole blockchain possède une sérialisation canonique correcte et interopérable.

Cette distinction est obligatoire pour éviter une fausse clôture.

---

# 15. Registre persistant — état post-148

## P0

- **CR-001** — exposition historique potentielle d'une clé Kaggle : OPEN, action opérateur requise.

## P1 / sécurité et architecture

- **FL-001** — thread-safety/cycle de vie global du logger : OPEN.
- **FL-005** — snapshot FILE* insuffisant pour garantir la durée de vie : OPEN.
- **BL-003 → BL-012** — registre historique à poursuivre.
- **BL-013** — nonce/bits hors fenêtre de hash de `block_header_hash()` : OPEN.
- **BL-015** — divergence de sérialisation blockchain : OPEN.
- **BUILD-PROOF-001** — absence de CI C indépendante : OPEN.
- **BUILD-PORT-002** — validation d'exécution portable indépendante : OPEN.
- **NQubit** — superposition simulée par RNG classique : OPEN scientifique.
- **ART-CT003** — copies divergentes de `common_types.h` dans artefacts historiques : OPEN.

## Logging

- **LL-002**
- **LL-004**
- **LL-005**

toujours à revalider.

## Navier-Stokes

- **NS-001 → NS-012** toujours conservés.
- **NS-007** reste requalifié selon les conclusions précédentes.

## IBM / synchronisation

- **IBM-001**
- **IBM-002**

toujours ouverts.

## Tests / traçabilité

- **TT-001**
- **TT-002**

toujours ouverts.

## Chantiers scientifiques

- Richardson / convergence ;
- T04 énergie ;
- Lyapunov ;
- NX-42 ;
- isolation LumVorax / ARTCB.

Aucun de ces chantiers n'est supprimé par les corrections de la session 147.

---

# 16. État des corrections session 147

| Élément | Verdict audit |
|---|---|
| BL-014 isolation des objets portables | **CLÔTURÉ** pour le défaut précis audité |
| T03 double-SHA256 exact | **RENFORCÉ / VALIDÉ** |
| T04 comparaison exacte | **RENFORCÉ / VALIDÉ pour la représentation testée** |
| FL-005 snapshot FILE* | **OPEN — correction insuffisante** |
| FL-001 thread-safety globale | **OPEN** |
| BL-013 | **OPEN** |
| BL-015 nouveau — sérialisation divergente | **OPEN** |
| BUILD-PORT-002 | **OPEN** |
| BUILD-PROOF-001 | **OPEN** |
| CR-001 | **OPEN P0** |

---

# 17. Critères de clôture renforcés

Pour une anomalie concurrente ou cryptographique, le simple fait que le programme compile et qu'un test nominal passe ne suffit plus.

Une clôture définitive doit démontrer :

1. correction du mécanisme ;
2. absence du chemin défectueux ;
3. test nominal ;
4. test adversarial ;
5. test concurrent lorsque pertinent ;
6. déterminisme inter-machine lorsque pertinent ;
7. référence indépendante lorsque cryptographie ;
8. intégration dans tous les chemins consommateurs ;
9. commit exact ;
10. résultat reproductible.

---

# 18. Conclusion technique

La session 147 contient de vraies corrections.

**BL-014 est effectivement corrigé au niveau de la structure du Makefile.**

Les vecteurs T03/T04 améliorent également nettement la qualité des tests SHA-256.

En revanche, **FL-005 ne peut pas être déclaré définitivement clos avec le mécanisme de snapshot actuel** : le mutex protège la copie du pointeur, mais pas la durée de vie du `FILE *` après libération du verrou.

Le point blockchain est encore plus important : **BL-013 reste ouvert et BL-015 est ajouté**, car `block_header_hash()` et `lumvorax_genesis_compute_hash()` ne reposent pas sur une sérialisation explicitement identique du header.

La prochaine correction blockchain doit donc commencer par une décision de protocole : **définir exactement les 80 octets canoniques qui constituent le message cryptographique**, puis faire utiliser cette définition par tous les chemins de hash, de mining et de validation.

**État global : CERTIFIED_100=false.**

Aucune modification du code source n'a été effectuée par cet audit. Seul ce rapport doit être ajouté au dossier `RAPPORT/`.
