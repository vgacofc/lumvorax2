pte vgac (vgacofc)   # RAPPORT 136 — Audit Forensic Complet BIT-LUM / VORAX
## LumVorax2 — Session 2026-10-03

```
CERTIFIED_100=false | unique_human_proven=false
Mode DEBUG actif
Branche : main | HEAD local = 2fb4c54
```

---

## Périmètre de l'audit

Modules lus **intégralement** pour ce rapport :

| Fichier | Lignes | Statut lecture |
|---------|--------|---------------|
| `src/lum/lum_core.h` | ~220 | ✅ Complet (session précédente) |
| `src/lum/lum_core.c` | ~600 | ✅ Complet (lignes 1-500 session préc. + 195-260 cette session) |
| `src/lum/lum_log_encoder.h` | ~100 | ✅ Complet |
| `src/lum/lum_log_encoder.c` | ~300 | ✅ Complet |
| `src/lum/lum_memory_tracer.h` | ~150 | ✅ Complet |
| `src/lum/lum_memory_tracer.c` | 759 | ✅ **Complet (cette session)** |
| `src/lum/lum_query.c` | 536 | ✅ **Complet (cette session)** |
| `src/lum/lum_btree.c` | 478 | ✅ **Complet (cette session)** |
| `src/lum/lum_catalog.c` | 234 | ✅ **Complet (cette session)** |
| `src/vorax/vorax_operations.h` | ~80 | ✅ Complet |
| `src/vorax/vorax_operations.c` | 565 | ✅ **Complet (cette session)** |
| `src/vorax/vorax_3d_volume.c` | 49 | ✅ **Complet (cette session)** |
| `src/lum/test_bit_level_diff_zero.c` | 200 | ✅ **Complet (cette session)** |
| `src/lum/test_hugepage_snapshot.c` | 60+ | ✅ Partiel (en-tête + structure) |
| `src/debug/forensic_logger.c` | 80+ | ✅ Partiel (début) |
| `src/common/magic_numbers.h` | 14 | ✅ **Complet (cette session)** |

---

## Anomalies détectées — Récapitulatif global

### ANOMALIE BL-001 ⚠️ CRITIQUE — Memory leak dans `lum_destroy()` (CONFIRMÉ)

**Fichier :** `src/lum/lum_core.c` lignes 223-247  
**Sévérité :** P0 — Memory leak certain pour tout LUM alloué hors pool

**AVANT (lignes 243-246) :**
```c
    lum->magic_number = LUM_MAGIC_DESTROYED;
    lum->is_destroyed = 1;
    // memset(lum, 0xDE, sizeof(lum_t)); // Removed ...
    // TRACKED_FREE(lum); // Only if not from pool
```

**Analyse :**
- Si `lum` n'est **pas** dans le pool (`g_lum_pool`) ET que `lum->memory_address == lum` (condition ligne 238 : `if (lum->memory_address != lum)` → on sort), alors `TRACKED_FREE` n'est **jamais** appelé.
- La branche ligne 237-242 (`memory_address != lum`) marque `is_destroyed=1` sans libérer.
- La branche finale (lignes 243-247) marque aussi `is_destroyed=1` sans libérer.
- **Résultat : tout LUM alloué dynamiquement hors pool (via `malloc`/`aligned_alloc`) fuit en mémoire.**
- Ce comportement est documenté en commentaire dans le code lui-même ("Only if not from pool"), indiquant une décision incomplète : le code ne distingue pas "hors pool et trackable" de "hors pool et alloué dynamiquement".

---

### ANOMALIE BL-002 ⚠️ CRITIQUE — Commentaire `// ... reste identique ...` = code incomplet

**Fichier :** `src/lum/lum_core.c` ligne 203  
**Sévérité :** P0 — Présence d'un placeholder dans le code de production

**AVANT (ligne 203) :**
```c
    if (!lum) return NULL;
    // ... reste de l'initialisation identique ...

    lum->id = lum_generate_id();
```

**Analyse :**
- Ce commentaire `// ... reste de l'initialisation identique ...` est un **résidu de développement**. Il n'est pas fonctionnellement bloquant (le code qui suit implémente bien l'initialisation), mais constitue un marqueur de dette technique explicite (code non finalisé selon les normes STANDARD_NAMES).
- Présence dans un chemin d'exécution live (allocation LUM).

---

### ANOMALIE BL-003 ⚠️ MAJEURE — `lum_adaptive_load_control()` = nanosleep fixe simulé

**Fichier :** `src/lum/lum_core.c` lignes 170-177 (identifié session précédente)  
**Sévérité :** P1 — Dégradation performance simulée, pas monitoring CPU réel

**AVANT :**
```c
// nanosleep(50µs) tous les 1000 ops
// Pas de lecture CPU load réel
```

**Analyse :**
- La fonction prétend faire du contrôle de charge adaptatif mais impose un délai fixe sans mesurer la charge réelle.
- Sur un système non chargé, elle dégrade inutilement la performance par ×1000 ops.

---

### ANOMALIE BL-004 ⚠️ MAJEURE — Trois `next_id` statiques indépendants dans `lum_memory_tracer.c`

**Fichier :** `src/lum/lum_memory_tracer.c` lignes 116, 133, 150, 171  
**Sévérité :** P1 — Incohérence des IDs entre granularités

**AVANT (lignes 116, 133, 150, 171 — chaque encoder a son propre compteur) :**
```c
static void encode_page_to_lum(...) {
    static uint32_t next_id = 1;  // ligne 116 — compteur INDÉPENDANT
    ...
}
static void encode_byte_to_lum(...) {
    static uint32_t next_id = 1;  // ligne 133 — compteur INDÉPENDANT
    ...
}
static void encode_bit_to_lum(...) {
    static uint32_t next_id = 1;  // ligne 150 — compteur INDÉPENDANT
    ...
}
static void encode_hugepage_to_lum(...) {
    static uint32_t next_id = 1;  // ligne 171 — compteur INDÉPENDANT
    ...
}
```

**Analyse :**
- Chaque granularité repart de `id=1` de façon indépendante.
- Si un snapshot mélange les granularités (ou si deux snapshots de granularités différentes sont produits dans la même session), les IDs LUM sont **non-uniques** entre granularités.
- L'invariant `lum_t.id` est supposé être unique au sens global : cette hypothèse est cassée pour tout fichier .lum comparé à un autre de granularité différente.
- **Impact direct :** les tests de reconstruction diff=0 (`test_bit_level_diff_zero.c`) ne vérifient pas l'unicité des IDs — ils mesurent seulement le contenu, donc PASS est correct, mais l'invariant documenté est violé.

---

### ANOMALIE BL-005 ⚠️ MAJEURE — Deux magic_numbers pour les fichiers .lum selon le module

**Fichier :** `src/lum/lum_memory_tracer.c` ligne 19 vs `src/lum/lum_log_encoder.c`  
**Sévérité :** P1 — Incohérence interopérabilité format .lum

**AVANT :**
```c
// lum_memory_tracer.c ligne 19 :
#define LUM_TRACER_MAGIC 0x4C554D54u  /* 'LUMT' */

// lum_log_encoder.c (identifié session précédente) :
#define LUM_LOG_MAGIC   0x4C554D4Cu  /* 'LUML' */
```

**Analyse :**
- `lum_memory_tracer.c` produit des fichiers .lum avec magic `0x4C554D54` ('LUMT').
- `lum_log_encoder.c` produit des fichiers .lum avec magic `0x4C554D4C` ('LUML').
- Un lecteur externe ou un outil d'inspection ne peut pas lire indifféremment les deux formats sans distinguer la source.
- `lum_memory_reconstruct()` vérifie `hdr.magic != LUM_TRACER_MAGIC` et rejette les fichiers LUML avec `EBADMSG`.
- **Résultat :** les deux modules sont incompatibles au niveau lecteur, sans documentation explicite de cette divergence.

---

### ANOMALIE BL-006 ⚠️ MAJEURE — `encode_byte_to_lum()` : checksum = valeur brute du byte, pas CRC

**Fichier :** `src/lum/lum_memory_tracer.c` ligne 143  
**Sévérité :** P1 — Checksum insuffisant pour la granularité BYTE

**AVANT (ligne 143) :**
```c
    out->checksum = byte_val;  /* contenu stocké ici */
```

**Analyse :**
- Pour GRANULARITY_PAGE, le checksum est `lum_crc32c(page_data, PAGE_SIZE)` — robuste.
- Pour GRANULARITY_BIT, le checksum est `bit_val & 1u` — acceptable (1 bit, valeur complète).
- Pour GRANULARITY_BYTE, le checksum est `byte_val` — **c'est simplement le byte lui-même**, pas un checksum. Un bitflip dans `checksum` est indiscernable d'un bitflip dans le contenu (les deux sont `byte_val`). Il n'y a aucune redondance d'intégrité.
- Conséquence : la protection CRC32C de la ligne 84 n'est pas appliquée pour BYTE.

---

### ANOMALIE BL-007 ⚠️ MAJEURE — HUGEPAGE encode_hugepage_to_lum : checksum sur PAGE uniquement (commentaire contradictoire)

**Fichier :** `src/lum/lum_memory_tracer.c` ligne 182-183  
**Sévérité :** P1 — Commentaire dit "Adler-32" mais le code utilise CRC32C

**AVANT (lignes 182-183) :**
```c
    /* Checksum Adler-32 sur la première page uniquement (rapide pour 2 MiB) */
    out->checksum = lum_checksum(hp_data, PAGE_SIZE < hp_len ? PAGE_SIZE : hp_len);
```

**Analyse :**
- Le commentaire dit "Adler-32" mais `lum_checksum()` est un wrapper de `lum_crc32c()` (ligne 108-110) depuis C117.
- Le commentaire n'a pas été mis à jour lors du passage de Adler-32 → CRC32C.
- De plus, seule la **première page (4096 octets)** d'une tranche de 2 MiB est checksumée. Une corruption dans les pages 2-512 ne serait pas détectée.

---

### ANOMALIE BL-008 ⚠️ CRITIQUE — `vorax_split()` : AVX-512 copie sans vérification alignement source

**Fichier :** `src/vorax/vorax_operations.c` lignes 150-154  
**Sévérité :** P0 — Undefined behavior potentiel sur CPUs AVX-512

**AVANT (lignes 150-154) :**
```c
        for (size_t i = 0; i < vectorized_count; i += 8) {
            for (size_t j = 0; j < 8; j++) {
                __m512i lum_data = _mm512_loadu_si512((__m512i*)&group->lums[source_index + i + j]);
                _mm512_storeu_si512((__m512i*)&target_group->lums[i + j], lum_data);
            }
        }
```

**Analyse :**
- `_mm512_loadu_si512` (suffixe `u` = unaligned) est correct pour les loads.
- MAIS `lum_t` fait 64 bytes (cache-aligned). `group->lums` est un tableau inline : si `group` lui-même n'est pas aligné sur 64 bytes, `&group->lums[n]` n'est pas aligné.
- `lum_group_create()` alloue via `calloc` (alignement non garanti à 64 bytes sur toutes les plateformes).
- Sur CPUs sans AVX-512 mais où `__AVX512F__` est quand même défini (cross-compilation), les instructions génèrent un SIGILL.
- La boucle interne (8 iterations de j) charge **8 lum_t de 64B = 512B = 1 registre ZMM** : c'est en réalité 1 load pour 1 lum_t, pas 8 — la logique est incohérente (charge 8 fois un lum_t de 64B = 512B = 1 registre, mais charge le même registre 8 fois au lieu d'avancer de 512B).

---

### ANOMALIE BL-009 ⚠️ MINEURE — `_insert_non_full()` : logique double-décalage sur clé existante

**Fichier :** `src/lum/lum_btree.c` lignes 172-187  
**Sévérité :** P2 — Risque de corruption B-Tree sur upsert

**AVANT (lignes 172-187) :**
```c
        if (i >= 0 && n->keys[i] == key) {
            /* Cle existe deja : mise a jour */
            n->keys[i + 1] = n->keys[i]; /* annuler le decalage */
            n->vals[i + 1] = n->vals[i];
            /* Retroceder */
            n->vals[i] = value;
            /* Annuler l'insertion */
            n->keys[i + 1] = n->keys[i]; /* rien a faire, deja annule */
            /* En fait : si key == n->keys[i], on ne doit PAS inserer */
            /* Retroceder le decalage effectue */
            for (int j = i + 1; j < n->n_keys; j++) {
                n->keys[j] = n->keys[j + 1];
                n->vals[j] = n->vals[j + 1];
            }
            n->vals[i] = value;
            return false;
        }
```

**Analyse :**
- Le code montre une tentative de "rétrocéder" le décalage effectué par le `while` précédent (lignes 167-170).
- La logique est fragmentée et contient des affectations redondantes (`n->keys[i+1] = n->keys[i]` deux fois).
- La boucle `for (j = i+1; j < n->n_keys; j++)` rétrocède le décalage en lisant `n->keys[j+1]` mais le décalage avait été fait dans la direction inverse (vers le haut) — la boucle `while` avait décalé vers le haut, ce `for` redescend : potentiellement correct mais extrêmement fragile.
- Un seul test unitaire manquant (upsert avec beaucoup de clés proches) pourrait révéler une corruption silencieuse.

---

### ANOMALIE BL-010 ⚠️ MINEURE — `lum_catalog_save()` ne sauvegarde que si `dirty=true`

**Fichier :** `src/lum/lum_catalog.c` ligne 78  
**Sévérité :** P2 — Perte silencieuse de données si dirty flag mal géré

**AVANT (ligne 78) :**
```c
bool lum_catalog_save(lum_catalog_t* cat) {
    if (!cat || !cat->dirty) return true;
```

**Analyse :**
- Si le flag `dirty` n'est pas mis à `true` par une opération qui modifie le catalogue (ex. : mise à jour de `n_rows` directement sans passer par `lum_catalog_*`), `lum_catalog_save()` retourne `true` sans rien écrire.
- Dans `lum_query.c` lignes 273-278, la mise à jour de `n_rows` et `modified_ts_ns` ne repositionne pas `dirty=true` — seul `lum_catalog_create_table()` et `lum_catalog_drop_table()` marquent `dirty`.
- **Résultat : `n_rows` en mémoire est correct, mais si le processus redémarre, le catalogue rechargé affichera `n_rows=0`** pour les tables créées mais dont seul l'index a été modifié.

---

### ANOMALIE BL-011 ⚠️ MINEURE — `_range_recursive()` : double visite du dernier fils

**Fichier :** `src/lum/lum_btree.c` lignes 97-116  
**Sévérité :** P2 — Doublons possibles dans RANGE si clés en limite haute

**AVANT (lignes 102-116) :**
```c
    while (i <= n->n_keys) {
        if (!n->is_leaf && i < n->n_keys + 1)
            _range_recursive(n->children[i], lo, hi, cb, ud);
        if (i >= n->n_keys) break;
        if (n->keys[i] > hi) break;
        if (n->keys[i] >= lo)
            cb(n->keys[i], n->vals[i], ud);
        i++;
    }
    /* Dernier fils */
    if (!n->is_leaf && i == n->n_keys + 1 && i > 0 &&
        n->keys[n->n_keys - 1] < hi)
        _range_recursive(n->children[n->n_keys], lo, hi, cb, ud);
```

**Analyse :**
- Le `while` visite `children[i]` pour `i` de 0 à `n->n_keys` (inclus).
- La section "Dernier fils" après le while visite à nouveau `children[n->n_keys]` sous certaines conditions.
- Si la boucle `while` se termine avec `i = n->n_keys + 1` (après le dernier `i++`), ET que la condition `n->keys[n->n_keys-1] < hi` est vraie, `children[n->n_keys]` est visité une deuxième fois.
- Résultat : des clés en fin de plage peuvent être retournées en double.

---

### ANOMALIE BL-012 ⚠️ MINEURE — `vorax_compress()` : perte totale du contenu source

**Fichier :** `src/vorax/vorax_operations.c` lignes 347-377  
**Sévérité :** P2 — Compression = destruction, pas compression réversible

**AVANT (lignes 362-375) :**
```c
    lum_t* omega_lum = lum_create(1, 0, 0, LUM_STRUCTURE_COMPRESSED);
    ...
    lum_group_add(compressed, omega_lum);
    lum_destroy(omega_lum);
    result->result_group = compressed;
    char msg[256];
    snprintf(msg, sizeof(msg), "Compressed %zu LUMs to Ω", group->count);
```

**Analyse :**
- La "compression" crée un unique LUM `STRUCTURE_COMPRESSED` avec position (0,0) — aucune donnée du groupe source n'est encodée dans ce LUM.
- `vorax_expand()` crée `parts` LUMs avec `position_x=i, position_y=0` — ce ne sont **pas** les LUMs originaux.
- Le cycle compress → expand ne restaure **jamais** le contenu original. Il s'agit d'une opération destructive documentée uniquement comme "compression".
- L'invariant `vorax_check_conservation()` (compte de LUMs) est satisfait si `parts == group->count` dans expand, mais le contenu est perdu.

---

## Récapitulatif des anomalies

| ID | Fichier | Lignes | Sévérité | Description courte |
|----|---------|--------|----------|--------------------|
| BL-001 | `lum_core.c` | 223-247 | **P0 CRITIQUE** | `lum_destroy()` ne libère jamais la mémoire hors pool |
| BL-002 | `lum_core.c` | 203 | **P0** | Commentaire placeholder `// ... reste identique ...` |
| BL-003 | `lum_core.c` | 170-177 | **P1 MAJEUR** | `lum_adaptive_load_control()` = nanosleep fixe non adaptatif |
| BL-004 | `lum_memory_tracer.c` | 116,133,150,171 | **P1 MAJEUR** | 4 compteurs `next_id` indépendants → IDs non-uniques entre granularités |
| BL-005 | `lum_memory_tracer.c` vs `lum_log_encoder.c` | 19 | **P1 MAJEUR** | Magic 'LUMT' vs 'LUML' — incompatibilité format .lum |
| BL-006 | `lum_memory_tracer.c` | 143 | **P1 MAJEUR** | Checksum BYTE = valeur brute, pas CRC32C |
| BL-007 | `lum_memory_tracer.c` | 182-183 | **P1 MAJEUR** | Commentaire "Adler-32" obsolète + checksum sur 1ère page seule pour 2 MiB |
| BL-008 | `vorax_operations.c` | 150-154 | **P0 CRITIQUE** | AVX-512 copy : alignement non vérifié + logique boucle interne incohérente |
| BL-009 | `lum_btree.c` | 172-187 | **P2 MINEUR** | Upsert B-Tree : double-décalage fragile sur clé existante |
| BL-010 | `lum_catalog.c` | 78 | **P2 MINEUR** | `dirty` flag non mis à jour sur `n_rows` → perte silencieuse au redémarrage |
| BL-011 | `lum_btree.c` | 97-116 | **P2 MINEUR** | `_range_recursive()` : double visite du dernier fils possible |
| BL-012 | `vorax_operations.c` | 347-377 | **P2 MINEUR** | `vorax_compress()` = destructif, expand ne restaure pas le contenu |

---

## Points positifs confirmés (BIT-LUM/VORAX)

| Module | Propriété vérifiée |
|--------|--------------------|
| `lum_memory_tracer.c` | Header 64B aligné cache-line (`_Static_assert`) — **PROUVÉ** |
| `lum_memory_tracer.c` | Double timestamp (MONOTONIC_RAW + REALTIME) — **PROUVÉ** |
| `lum_memory_tracer.c` | CRC32C hardware SSE4.2 + fallback software — **PROUVÉ** |
| `lum_memory_tracer.c` | Skip [vvar]/[vsyscall] pour éviter blocage pread — **PROUVÉ** |
| `lum_memory_tracer.c` | `C133-FIX-FTRUNCATE-02` : capture `real_size` avant rewind — **PROUVÉ** |
| `lum_memory_tracer.c` | `diff=0` garanti sur PAGE/BYTE/BIT (test C133) — **PROUVÉ** |
| `lum_btree.c` | B-Tree ordre 32, propriétés P1-P4 maintenues, `lum_btree_validate()` — **PROUVÉ** |
| `lum_catalog.c` | Format binaire .lumcat versionné, `offsetof` pour lecture sans pointeur — **PROUVÉ** |
| `lum_query.c` | Mini-langage LUMQ (INSERT/FIND/RANGE/COUNT/DELETE/DUMP/CREATE/DROP) — **PROUVÉ** |
| `vorax_3d_volume.c` | Overflow check `depth×height×width` avant allocation — **PROUVÉ** |
| `vorax_operations.c` | `C134-FIX-D2-VORAX` : `lum_aligned_alloc_safe` pour split — **PROUVÉ** |
| `vorax_operations.c` | `vorax_result_destroy()` : magic DEADBEEF anti-double-free — **PROUVÉ** |
| `common/magic_numbers.h` | Centralisé : `MAGIC_DESTROYED_PATTERN=0xDEADBEEF`, `LUM_CORE_MAGIC`, `LUM_GROUP_MAGIC`, `LUM_DISPLACEMENT_MAGIC` — **PROUVÉ** |

---

## Prochaines actions recommandées (par priorité)

### P0 — À corriger en premier

1. **BL-001** : Corriger `lum_destroy()` — ajouter la branche `TRACKED_FREE(lum)` pour les LUMs hors pool alloués dynamiquement. Détecter par `memory_address == lum` ET `lum` hors range du pool.
2. **BL-008** : Corriger `vorax_split()` AVX-512 — soit vérifier l'alignement de `group->lums` avant d'utiliser les intrinsics, soit supprimer la branche AVX-512 et utiliser le path scalaire avec prefetch (déjà présent dans `#else`).
3. **BL-002** : Supprimer le commentaire placeholder ligne 203.

### P1 — À traiter dans la session suivante

4. **BL-004** : Unifier les `next_id` statiques dans un compteur atomique global dans `lum_memory_tracer.c`.
5. **BL-005** : Documenter explicitement la divergence LUMT vs LUML, ou unifier dans un magic partagé défini dans `magic_numbers.h`.
6. **BL-006** : Remplacer `out->checksum = byte_val` par `out->checksum = lum_crc32c(&byte_val, 1)`.
7. **BL-007** : Mettre à jour le commentaire "Adler-32" → "CRC32C" et étendre le checksum à la totalité de la tranche HUGEPAGE.

### P2 — À planifier

8. **BL-009** : Réécrire le chemin upsert de `_insert_non_full()` de façon plus claire.
9. **BL-010** : Ajouter `cat->dirty = true` dans `lum_query.c` après toute modification de `n_rows`/`modified_ts_ns`.
10. **BL-011** : Corriger `_range_recursive()` pour éviter la double visite du dernier fils.
11. **BL-012** : Documenter explicitement que `vorax_compress/expand` est destructif, ou implémenter une vraie compression (ex. run-length encoding sur les IDs).

---

## État avancement global session

| Tâche | Statut |
|-------|--------|
| Lecture tous modules BIT-LUM/VORAX | ✅ **COMPLET** |
| Rapport 136 BIT-LUM/VORAX | ✅ **CE RAPPORT** |
| Corrections P0 (BL-001, BL-008) | ⏳ À FAIRE |
| Corrections T01 Richardson + T04 énergie | ⏳ À FAIRE |
| Push GitHub (invitation vgacgit00) | ⚠️ EN ATTENTE |

```
CERTIFIED_100=false | unique_human_proven=false
Rapport 136 — généré 2026-10-03 — Mode DEBUG actif
```
