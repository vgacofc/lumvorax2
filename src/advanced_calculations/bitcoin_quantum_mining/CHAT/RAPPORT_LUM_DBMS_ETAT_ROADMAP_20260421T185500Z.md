# Format LUM — État actuel et roadmap vers DBMS complet

## Verdict honnête de Claude/ChatGPT (synthétisé)

> ❌ « DBMS LuM remplace PostgreSQL »
>
> Techniquement faux dans l'état actuel.
>
> Ce qui existe :
> - ✅ **log append-only + hash-chain** (forensic intégrité)
> - ✅ format binaire 64 bytes par enregistrement (`btc_nx48_last.lum`)
> - ✅ CRC32 par bloc, sérialisation rapide (5× plus rapide que CSV)
>
> Ce qui MANQUE pour être un vrai DBMS :
> - ❌ pas de requêtage complexe (pas de SELECT/JOIN/WHERE)
> - ❌ pas de transactions concurrentes réelles (pas de MVCC)
> - ❌ pas de planificateur de requête (pas de cost-based optimizer)
> - ❌ pas d'index multi-dimensionnel (pas de B-Tree, pas de R-Tree)

## État actuel des modules `src/file_formats/` et `src/persistence/`

| Module | Lignes | Capacité actuelle | Manque pour DBMS |
|---|---|---|---|
| `lum_secure_serialization.c` | ~800 | Sérialisation binaire 64 bytes/record + CRC32 | Pas d'index |
| `lum_native_file_handler.c` | ~1100 | I/O fichier .lum + gzip + SHA-256 | Pas de pages, pas de buffer pool |
| `lum_native_universal_format.c` | ~830 | Format LUMQ v2 (header + records + footer) | Pas de schema, pas de catalog |
| `data_persistence.c` | ~600 | Append-only log avec hash-chain | Pas de WAL, pas de checkpoint |
| `transaction_wal_extension.c` | ~900 | WAL minimal (write-ahead log) | Pas de 2PC, pas d'isolation |
| `recovery_manager_extension.c` | ~780 | Recovery basique après crash | Pas de point-in-time recovery |

**Ce qu'on a déjà** : un système de fichiers append-only avec intégrité forensique. C'est l'équivalent d'un **fichier de log structuré** (LSM-tree très basique sans le merge), pas un SGBD.

## Pourquoi ce n'est PAS un DBMS au sens PostgreSQL

| Composant DBMS standard | LUM actuel | PostgreSQL |
|---|---|---|
| **Storage engine** | Append-only flat file | Heap + B-Tree + GiST + GIN |
| **Query parser** | ❌ aucun | SQL → AST |
| **Query planner** | ❌ aucun | Cost-based optimizer + statistics |
| **Query executor** | ❌ scan séquentiel uniquement | Volcano model + parallel workers |
| **Transaction manager** | WAL minimal | MVCC + snapshot isolation |
| **Concurrency control** | ❌ mutex global lecture | Row-level locks + 2PL |
| **Index types** | ❌ aucun | B-Tree, Hash, GiST, SP-GiST, GIN, BRIN |
| **Catalog (schema metadata)** | ❌ aucun | pg_catalog complet |
| **Network protocol** | ❌ aucun | libpq frontend/backend protocol |
| **Authentication** | N/A | pg_hba.conf + SCRAM/MD5 |
| **Replication** | ❌ aucune | Streaming + logical replication |

## Roadmap honnête — passer du log forensique au DBMS

### Phase 1 (0-15% — fait)
- ✅ Format binaire compact (64 bytes/record)
- ✅ Hash-chain + CRC32 (intégrité)
- ✅ Compression gzip post-écriture
- ✅ Sérialisation 5× plus rapide que CSV

### Phase 2 (15-35% — partiellement fait)
- ✅ WAL minimal (`transaction_wal_extension.c`)
- ✅ Recovery basique après crash
- ⏳ Snapshot isolation (manque)
- ⏳ Catalog/schema metadata (manque)

### Phase 3 (35-60% — à faire)
- ❌ Index B-Tree pour recherche par clé
- ❌ Index R-Tree pour requêtes spatiales (utile pour LUMs 4D)
- ❌ Buffer pool avec LRU
- ❌ Page-based storage (pages 8KB comme PostgreSQL)
- ❌ Catalog système (pg_class équivalent)

### Phase 4 (60-85% — à faire)
- ❌ Mini-langage de requête (AST + interpreter)
- ❌ Query planner basique (push-down filter)
- ❌ Concurrency control row-level (pas seulement global)
- ❌ MVCC complet (snapshot timestamp)

### Phase 5 (85-100% — à faire)
- ❌ Protocole client-serveur
- ❌ Authentication
- ❌ Replication (streaming WAL)
- ❌ Backup/restore point-in-time

### État actuel global : **~25%** d'un DBMS complet

## Recommandation honnête

**NE PAS prétendre que LUM remplace PostgreSQL aujourd'hui.** La phrase correcte serait :

> « LUM est un **format de stockage forensique append-only avec hash-chain**, optimisé pour la traçabilité d'expériences quantiques.
> Il complète PostgreSQL plutôt que de le remplacer : PostgreSQL gère les requêtes structurées et les transactions concurrentes, LUM garantit l'intégrité bit-par-bit et la reproductibilité scientifique des runs IBM Quantum. »

Pour atteindre un vrai DBMS, l'effort restant est **~15 000-30 000 lignes de C** (B-Tree, parser SQL minimal, query planner, MVCC). C'est faisable mais ce serait un projet à part entière de ~6-12 mois.

## Alternative pragmatique recommandée

Au lieu de réinventer un DBMS complet, intégrer LUM avec un **moteur SQL embarqué léger** :

| Option | Lignes effort | Avantage |
|---|---|---|
| **DuckDB** (analytique colonnaire C++) | ~500 lignes wrapper | OLAP rapide, supporte parquet/csv, déjà mature |
| **SQLite VFS custom** | ~1500 lignes | Parser SQL gratuit, transactions ACID, mature |
| **PostgreSQL Foreign Data Wrapper LUM** | ~800 lignes | Intégration directe avec PostgreSQL, requêtage natif |

Cela donnerait **80% des fonctionnalités DBMS pour 5% de l'effort**, tout en gardant LUM pour ce qu'il fait bien : la traçabilité forensique.
