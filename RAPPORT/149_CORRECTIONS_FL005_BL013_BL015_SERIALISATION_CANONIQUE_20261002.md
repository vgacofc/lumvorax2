# Rapport 149 — Corrections FL-005 (durée de vie FILE*) + BL-013/BL-015 (sérialisation canonique blockchain)

**Date :** 2026-10-02  
**Session :** 149  
**HEAD avant correction :** `31e0f7b`  
**HEAD après correction :** *voir commit ci-dessous*  
**Repository :** `vgacofc/lumvorax2` / branche `main`  
**CERTIFIED_100=false**  
**unique_human_proven=false**  
**Mode DEBUG actif**

---

## 1. Expertises activées

- Concurrence POSIX/pthreads — cycle de vie des ressources FILE*, race use-after-close.
- Cryptographie appliquée — SHA-256, double-SHA256, sérialisation canonique LE, interopérabilité blockchain.
- Protocoles blockchain — définition normative du message cryptographique, PoW, nonce, bits.
- Portabilité C99 — endianness, padding, ABI, sérialisation explicite vs cast struct.
- Tests de régression — vecteurs indépendants Python hashlib, tests adversariaux nonce/bits.

---

## 2. Anomalies traitées

| ID | P | Fichier | Statut avant | Statut après |
|----|---|---------|--------------|--------------|
| FL-005 | 1 | `src/debug/forensic_logger.c` | OPEN | **CLÔTURÉ** |
| BL-013 | 1 | `src/blockchain_lumvorax/block_header.c` | OPEN | **CLÔTURÉ** |
| BL-015 | 1 | `src/blockchain_lumvorax/genesis.c` | OPEN | **CLÔTURÉ** |

---

## 3. FL-005 — Correction use-after-close FILE* (forensic_log_individual_lum)

### Problème (rapport 148 §4)

La session 147 avait introduit un snapshot :

```c
pthread_mutex_lock(&fl001_log_file_mutex);
FILE* log_snapshot = forensic_log_file;   /* copie sous verrou */
pthread_mutex_unlock(&fl001_log_file_mutex);
/* ... */
fprintf(log_snapshot, ...);   /* écriture HORS verrou — race possible */
```

Séquence problématique :
1. Thread A copie `forensic_log_file` → `log_snapshot` sous verrou
2. Thread A relâche le mutex
3. Thread B appelle `forensic_logger_destroy()` → `fclose(forensic_log_file)` → `forensic_log_file = NULL`
4. Thread A utilise `log_snapshot` (alias du FILE* fermé) → **comportement indéfini**

### Avant (lignes 128–145, session 147)

**Fichier :** `src/debug/forensic_logger.c`

```c
/* ligne 133 */ pthread_mutex_lock(&fl001_log_file_mutex);
/* ligne 134 */ FILE* log_snapshot = forensic_log_file;   /* copie atomique sous verrou */
/* ligne 135 */ pthread_mutex_unlock(&fl001_log_file_mutex);
/* ligne 137 */ if (!log_snapshot) { ... return; }
/* ligne 144 */ fprintf(log_snapshot, "[%llu] [LUM_%u] %s: ...", ...);
/* ligne 145 */ fflush(log_snapshot);
```

### Après (rapport 149, Option A rapport 148 §4)

**Fichier :** `src/debug/forensic_logger.c`

```c
/* ligne 133 */ pthread_mutex_lock(&fl001_log_file_mutex);
/* ligne 134 */ if (!forensic_log_file) {
/* ligne 135 */     pthread_mutex_unlock(&fl001_log_file_mutex);
/* ligne 136 */     printf("[FORENSIC_ERROR] Log file not initialized for LUM_%u\n", lum_id);
/* ligne 137 */     return;
/* ligne 138 */ }
/* ligne 140 */ fprintf(forensic_log_file, "[%" PRIu64 "] [LUM_%u] %s: ...", ...);
/* ligne 141 */ fflush(forensic_log_file);
/* ligne 142 */ pthread_mutex_unlock(&fl001_log_file_mutex);  /* ← unlock APRÈS fflush */
```

### Propriétés garanties

- `fl001_log_file_mutex` couvre **toute** la durée pendant laquelle `forensic_log_file` est utilisé.
- `forensic_logger_destroy()` doit attendre la fin de chaque écriture avant de fermer.
- Ordre d'acquisition strict préservé : `fl001_log_file_mutex` TOUJOURS avant `fl001_individual_mutex`.
- Écriture console (`printf`) hors mutex — stdout ne dépend pas du FILE*.

### Critère de clôture FL-005

✅ Mécanisme corrigé — mutex maintenu pendant toute l'I/O  
✅ Chemin défectueux (snapshot + unlock + write) supprimé  
✅ Ordre d'acquisition strict documenté et préservé  
⚠️ Test concurrent ThreadSanitizer : **BUILD-THREAD-001 OPEN** — validation sous TSan recommandée (hors scope session 149, machine de dev macOS sans TSan pour clang)

---

## 4. BL-013 + BL-015 — Sérialisation canonique du header blockchain

### Problème BL-013 (rapport 148 §10)

`block_header_hash()` hashait les **80 premiers octets de la représentation mémoire C** :

```c
sha256_lumvorax((const uint8_t *)h, 80, mid);
```

Avec les offsets réels de `block_header_t` (vérifiés par offsetof) :

| Champ | Offset | Taille | Dans les 80 oct ? |
|-------|--------|--------|-------------------|
| version | 0 | 4 | ✅ |
| prev_hash | 4 | 32 | ✅ |
| merkle_root | 36 | 32 | ✅ (partiel — 36+32=68) |
| timestamp | 72 | 8 | ⚠️ 8 octets (68→76 = hors 80) |
| bits | 80 | 4 | ❌ hors fenêtre |
| nonce | 88 | 8 | ❌ hors fenêtre |

**Conséquence** : modifier le nonce ne changeait pas le digest — PoW impossible.

### Problème BL-015 (rapport 148 §11)

`lumvorax_genesis_compute_hash()` construisait un buffer 80 octets manuellement, mais avec des troncatures :

```c
uint32_t ts32    = (uint32_t)(hdr->timestamp & 0xFFFFFFFFU);  /* timestamp tronqué à 32 bits */
uint32_t nonce32 = (uint32_t)(hdr->nonce     & 0xFFFFFFFFU);  /* nonce tronqué à 32 bits */
memcpy(buf + 68, &ts32, 4);
memcpy(buf + 72, &hdr->bits, 4);
memcpy(buf + 76, &nonce32, 4);
```

→ **Deux fonctions calculant le "hash du header" avec deux représentations différentes**.

### Solution : block_header_serialize_canonical()

**Nouveau fichier :** `src/blockchain_lumvorax/block_header.c` (remplacé entièrement)  
**Déclaration :** `src/blockchain_lumvorax/blockchain_lumvorax.h` (ajout de la fonction + constante `LUMVORAX_HEADER_SERIAL_LEN = 88`)

Format canonique (88 octets, little-endian explicite, indépendant du padding/ABI) :

| Offset | Champ | Taille | Encodage |
|--------|-------|--------|----------|
| 0–3 | version | 4 | uint32_t LE |
| 4–35 | prev_hash | 32 | verbatim |
| 36–67 | merkle_root | 32 | verbatim |
| 68–75 | timestamp | 8 | uint64_t LE |
| 76–79 | bits | 4 | uint32_t LE |
| 80–87 | nonce | 8 | uint64_t LE |

Encodage LE explicite via macros `WRITE_LE32` / `WRITE_LE64` (pas de `memcpy` d'entiers — évite la dépendance à l'endianness du compilateur).

### Avant — block_header.c (lignes 15–21, session 147)

```c
void block_header_hash(const block_header_t *h, uint8_t out_hash[32]) {
    if (!h || !out_hash) return;
    uint8_t mid[32];
    sha256_lumvorax((const uint8_t *)h, 80, mid);  /* ← cast struct, nonce hors fenêtre */
    sha256_lumvorax(mid, 32, out_hash);
}
```

### Après — block_header.c (rapport 149)

```c
int block_header_serialize_canonical(const block_header_t *h,
                                     uint8_t out[LUMVORAX_HEADER_SERIAL_LEN]) {
    if (!h || !out) return -1;
    WRITE_LE32(out,  0, h->version);
    memcpy(out +  4, h->prev_hash,   32);
    memcpy(out + 36, h->merkle_root, 32);
    WRITE_LE64(out, 68, h->timestamp);
    WRITE_LE32(out, 76, h->bits);
    WRITE_LE64(out, 80, h->nonce);
    return LUMVORAX_HEADER_SERIAL_LEN;
}

void block_header_hash(const block_header_t *h, uint8_t out_hash[32]) {
    if (!h || !out_hash) return;
    uint8_t buf[LUMVORAX_HEADER_SERIAL_LEN];
    block_header_serialize_canonical(h, buf);  /* ← sérialisation canonique */
    uint8_t mid[32];
    sha256_lumvorax(buf, LUMVORAX_HEADER_SERIAL_LEN, mid);
    sha256_lumvorax(mid, 32, out_hash);
}
```

### Avant — genesis.c (lignes 39–52, session 147)

```c
uint8_t buf[80];
uint32_t ts32    = (uint32_t)(hdr->timestamp & 0xFFFFFFFFU);  /* troncature BUG */
uint32_t nonce32 = (uint32_t)(hdr->nonce     & 0xFFFFFFFFU);  /* troncature BUG */
memcpy(buf + 68, &ts32, 4);
memcpy(buf + 72, &hdr->bits, 4);
memcpy(buf + 76, &nonce32, 4);
sha256_lumvorax(buf, 80, mid);
```

### Après — genesis.c (rapport 149, lignes 60–67)

```c
uint8_t buf[LUMVORAX_HEADER_SERIAL_LEN];
block_header_serialize_canonical(hdr, buf);    /* ← délègue à la fonction canonique */
sha256_lumvorax(buf, LUMVORAX_HEADER_SERIAL_LEN, mid);
```

---

## 5. Mise à jour vecteur T04

L'ancien vecteur T04 était calculé sur les 80 premiers octets du cast struct — donc invalide avec BL-013 présent.

**Recalcul indépendant (Python hashlib) — sérialisation canonique 88 octets LE :**

```python
import hashlib, struct
buf = struct.pack('<I', 1)         # version=1, 4 LE
buf += b'\x00' * 32               # prev_hash
buf += b'\x00' * 32               # merkle_root
buf += struct.pack('<Q', 1727712000) # timestamp, 8 LE
buf += struct.pack('<I', 0)        # bits=0, 4 LE
buf += struct.pack('<Q', 42)       # nonce=42, 8 LE
assert len(buf) == 88
mid = hashlib.sha256(buf).digest()
result = hashlib.sha256(mid).hexdigest()
# → 27b8b9304208721bed3ba89297dc593d1d9c6840883bc62933042a9b7e921db0
```

| Ancien vecteur (BL-013) | Nouveau vecteur (rapport 149) |
|-------------------------|-------------------------------|
| `c9c8cd4d...6df0` | `27b8b930...1db0` |

---

## 6. Résultats des tests

```
make blockchain_test → 0 warning / 0 erreur
./bin/test_blockchain_sha256 → 11/11 PASS

[PASS] T01 — sha256("abc") == FIPS 180-4
[PASS] T02 — sha256("") == FIPS 180-4
[PASS] T03 — double-SHA256("abc") == vecteur Python hashlib indépendant
[PASS] T03b — double-SHA256 diffère du single-SHA256
[PASS] T04 — block_header_hash(canonical) == vecteur 27b8b930... (Python LE)
[PASS] T04b — digest non nul
[PASS] T05 — déterminisme (deux appels identiques → même hash)
[PASS] T06 — prev_hash différent → hash différent
[PASS] T06b — BL-013 CORRIGÉ : nonce=999 → hash différent de nonce=0
[PASS] T06c — BL-013 CORRIGÉ : bits=0x1d00ffff → hash différent de bits=0
[PASS] T07 — SHA-256(NIST 448-bit) == vecteur attendu
```

**make clean && make** : 0 erreur / 0 warning (build complet 39 modules)

---

## 7. Avant / Après résumé (lignes exactes)

| Fichier | Avant | Après |
|---------|-------|-------|
| `src/debug/forensic_logger.c` | L133: `FILE* log_snapshot = forensic_log_file; pthread_mutex_unlock(&fl001_log_file_mutex); … fprintf(log_snapshot, …);` | L133: `pthread_mutex_lock` → `if (!forensic_log_file)` → `fprintf(forensic_log_file, …)` → `fflush` → `pthread_mutex_unlock` |
| `src/blockchain_lumvorax/block_header.c` | L19: `sha256_lumvorax((const uint8_t *)h, 80, mid);` | L65: `block_header_serialize_canonical(h, buf); sha256_lumvorax(buf, 88, mid);` |
| `src/blockchain_lumvorax/genesis.c` | L39–52: sérialisation manuelle 80 oct, troncature nonce/ts à 32 bits | L60–67: `block_header_serialize_canonical(hdr, buf)` |
| `src/blockchain_lumvorax/blockchain_lumvorax.h` | L47: déclaration `block_header_hash` seulement | L47–53: + `#define LUMVORAX_HEADER_SERIAL_LEN 88` + `block_header_serialize_canonical()` |
| `src/tests/test_blockchain_sha256.c` | L88: vecteur `c9c8cd4d…` (80 oct struct), T06b PASS si nonce identique | L88: vecteur `27b8b930…` (88 oct LE), T06b PASS si nonce différent, T06c ajouté |

---

## 8. Registre persistant — état post-149

### Clôturés dans cette session

| ID | Description | Session clôture |
|----|-------------|-----------------|
| **FL-005** | use-after-close FILE* dans forensic_log_individual_lum | 149 |
| **BL-013** | nonce/bits hors fenêtre de hash | 149 |
| **BL-015** | divergence sérialisation genesis vs block_header_hash | 149 |

### Toujours ouverts (P1 et au-dessous)

- **FL-001** — thread-safety/cycle de vie global du logger : toujours OPEN (BUILD-THREAD-001 recommandé)
- **BUILD-THREAD-001** — validation TSan concurrent FL-005 : OPEN (nouveau, recommandé)
- **BL-003→BL-012** — registre historique à poursuivre
- **BUILD-PROOF-001** — CI C indépendante : OPEN
- **BUILD-PORT-002** — validation portable indépendante : OPEN
- **NQubit** — superposition simulée par RNG classique : OPEN scientifique
- **ART-CT003** — copies divergentes common_types.h : OPEN
- **NS-001→NS-012** — solveur Navier-Stokes : OPEN
- **LL-002 / LL-004 / LL-005** — logging : OPEN
- **IBM-001 / IBM-002** : OPEN
- **TT-001 / TT-002** : OPEN

---

## 9. Conclusion

Les trois anomalies P1 ciblées sont clôturées.

**FL-005** : le mutex est maintenant maintenu pendant toute la durée de l'écriture — la race use-after-close décrite dans le rapport 148 §4 est éliminée.

**BL-013 + BL-015** : `block_header_serialize_canonical()` est la source de vérité unique pour la sérialisation du header. Tous les champs PoW (version, prev_hash, merkle_root, timestamp, bits, nonce) sont inclus dans les 88 octets canoniques LE. `genesis.c` et `block_header.c` utilisent désormais la même sérialisation.

**État global : CERTIFIED_100=false.**
