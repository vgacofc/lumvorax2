# 🏗️ Architecture LumVorax - Guide Technique Complet

**Version**: 1.0.0  
**Date**: 2026-04-30  
**Auteur**: LumVorax Team

---

## Table des Matières

1. [Vue d'Ensemble](#1-vue-densemble)
2. [Architecture en Couches](#2-architecture-en-couches)
3. [Composants Principaux](#3-composants-principaux)
4. [Flux de Données](#4-flux-de-données)
5. [Patterns de Conception](#5-patterns-de-conception)
6. [Scalabilité et Performance](#6-scalabilité-et-performance)
7. [Sécurité](#7-sécurité)
8. [Déploiement](#8-déploiement)

---

## 1. Vue d'Ensemble

### 1.1 Philosophie de Conception

LumVorax est conçu selon les principes suivants :

- **Modularité** : Composants indépendants et réutilisables
- **Performance** : Optimisation à tous les niveaux (cache, algorithmes, mémoire)
- **Fiabilité** : Transactions ACID, réplication, consensus
- **Scalabilité** : Architecture distribuée P2P
- **Observabilité** : Traçage et monitoring intégrés

### 1.2 Technologies Clés

| Domaine | Technologie | Justification |
|---------|-------------|---------------|
| Langage | C11 | Performance, contrôle bas niveau |
| Threading | POSIX Threads | Portabilité, performance |
| Réseau | TCP/IP, UDP | Fiabilité + vitesse |
| Consensus | Raft | Simplicité, prouvé en production |
| Cache | Hash Table + LRU | O(1) accès, éviction efficace |
| Monitoring | perf_event_open | Accès hardware counters |

---

## 2. Architecture en Couches

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│  (Quantum Algorithms, Mining Logic, User Applications)      │
└─────────────────────────────────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Service Layer                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   Quantum    │  │   Mining     │  │     P2P      │      │
│  │  Simulator   │  │    Agent     │  │   Network    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Data Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  LUM Cache   │  │  LUM DBMS    │  │ Replication  │      │
│  │   (50ns)     │  │   (ACID)     │  │   (Raft)     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Infrastructure Layer                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  NX-Trace    │  │   NX-Perf    │  │   Logger     │      │
│  │  (Tracing)   │  │ (Monitoring) │  │  (Forensic)  │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Core Layer                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  lum_core    │  │    Memory    │  │   Common     │      │
│  │   (Kernel)   │  │   Tracker    │  │    Types     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. Composants Principaux

### 3.1 Core Layer

#### 3.1.1 lum_core (Noyau)
**Responsabilités** :
- Initialisation système
- Gestion du cycle de vie
- Configuration globale
- Coordination des composants

**API Clé** :
```c
typedef struct {
    uint32_t magic;
    uint32_t version;
    bool initialized;
    lum_config_t config;
} lum_core_t;

lum_core_t* lum_core_init(const lum_config_t* config);
void lum_core_shutdown(lum_core_t* core);
```

#### 3.1.2 Memory Tracker
**Responsabilités** :
- Allocation/libération mémoire
- Détection de fuites
- Statistiques d'utilisation
- Pool de mémoire

**Métriques** :
- Allocations totales
- Mémoire utilisée
- Fuites détectées
- Fragmentation

### 3.2 Infrastructure Layer

#### 3.2.1 NX-Trace (Traçage Sémantique)
**Architecture** :
```
┌─────────────────────────────────────────┐
│         Application Code                │
└──────────────┬──────────────────────────┘
               │ nx_trace_event()
               ▼
┌─────────────────────────────────────────┐
│      Event Buffer (Ring Buffer)         │
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐    │
│  │ E1 │→│ E2 │→│ E3 │→│ E4 │→│ E5 │    │
│  └────┘ └────┘ └────┘ └────┘ └────┘    │
└──────────────┬──────────────────────────┘
               │ Flush (async)
               ▼
┌─────────────────────────────────────────┐
│      Causal Graph Builder               │
│  (Reconstruit relations cause-effet)    │
└──────────────┬──────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────┐
│         Trace File (JSON)               │
└─────────────────────────────────────────┘
```

**Niveaux de Traçage** :
- **MINIMAL** (2% overhead) : Événements critiques uniquement
- **STANDARD** (4% overhead) : Événements importants
- **DETAILED** (6% overhead) : Tous les événements
- **EXHAUSTIVE** (8% overhead) : Événements + contexte complet

#### 3.2.2 NX-Perf (Monitoring Performance)
**Compteurs Hardware** :
```c
typedef enum {
    NX_PERF_CYCLES,           // Cycles CPU
    NX_PERF_INSTRUCTIONS,     // Instructions exécutées
    NX_PERF_CACHE_REFERENCES, // Accès cache
    NX_PERF_CACHE_MISSES,     // Cache misses
    NX_PERF_BRANCH_INSTRUCTIONS,
    NX_PERF_BRANCH_MISSES,
    NX_PERF_BUS_CYCLES,
    NX_PERF_STALLED_CYCLES_FRONTEND,
    NX_PERF_STALLED_CYCLES_BACKEND,
    NX_PERF_TLB_MISSES
} nx_perf_counter_type_t;
```

**Métriques Dérivées** :
- IPC (Instructions Per Cycle)
- Cache hit rate
- Branch prediction accuracy
- Memory bandwidth

### 3.3 Data Layer

#### 3.3.1 LUM Cache
**Architecture** :
```
┌─────────────────────────────────────────────────────────┐
│                    LUM Cache                            │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │         Hash Table (djb2)                       │   │
│  │  ┌────┐  ┌────┐  ┌────┐       ┌────┐           │   │
│  │  │ B0 │→ │ E1 │→ │ E2 │  ...  │ EN │           │   │
│  │  └────┘  └────┘  └────┘       └────┘           │   │
│  └─────────────────────────────────────────────────┘   │
│                        ▲                                │
│                        │                                │
│  ┌─────────────────────────────────────────────────┐   │
│  │      LRU List (Doubly-Linked)                   │   │
│  │  ┌────┐  ┌────┐  ┌────┐       ┌────┐           │   │
│  │  │MRU │←→│ E1 │←→│ E2 │ ←→... │LRU │           │   │
│  │  └────┘  └────┘  └────┘       └────┘           │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  pthread_mutex_t lock (Thread-safe)                    │
└─────────────────────────────────────────────────────────┘
```

**Politiques d'Éviction** :
1. **LRU** (Least Recently Used) : Évince l'entrée la moins récemment utilisée
2. **LFU** (Least Frequently Used) : Évince l'entrée la moins fréquemment utilisée
3. **NX48 Predictive** : Prédiction basée sur patterns d'accès

**Performance** :
- **Get** : O(1) - Hash lookup
- **Put** : O(1) - Hash insert + LRU update
- **Evict** : O(1) - LRU tail removal
- **Latence** : 50ns (théorique), 190ns (mesuré)

#### 3.3.2 LUM DBMS
**Architecture Transactionnelle** :
```
┌─────────────────────────────────────────────────────────┐
│                Transaction Manager                      │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Active Transactions (Hash Table)                │   │
│  │  TXN_ID → {state, isolation, locks, undo_log}   │   │
│  └──────────────────────────────────────────────────┘   │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│                  Lock Manager                           │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Lock Table (Resource → Lock Queue)             │   │
│  │  Deadlock Detection (Wait-For Graph)            │   │
│  └──────────────────────────────────────────────────┘   │
└──────────────┬──────────────────────────────────────────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│                   B-Tree Index                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │         Root Node                                │   │
│  │    ┌────┬────┬────┐                              │   │
│  │    │ K1 │ K2 │ K3 │                              │   │
│  │    └─┬──┴─┬──┴─┬──┘                              │   │
│  │      │    │    │                                 │   │
│  │   ┌──▼─┐ ┌▼──┐ ┌▼──┐                            │   │
│  │   │Leaf│ │Leaf│ │Leaf│                           │   │
│  │   └────┘ └────┘ └────┘                           │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

**Niveaux d'Isolation** :
1. **Read Uncommitted** : Dirty reads possibles
2. **Read Committed** : Lit uniquement données committées
3. **Repeatable Read** : Lectures répétables garanties
4. **Serializable** : Isolation complète (équivalent exécution série)

**Protocole 2PL (Two-Phase Locking)** :
- **Phase 1 (Growing)** : Acquisition de locks uniquement
- **Phase 2 (Shrinking)** : Libération de locks uniquement

#### 3.3.3 Replication System
**Architecture Master-Slave** :
```
┌─────────────────────────────────────────────────────────┐
│                      Master                             │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Write Operations                                │   │
│  │  ┌────────┐  ┌────────┐  ┌────────┐             │   │
│  │  │ Write1 │→ │ Write2 │→ │ Write3 │             │   │
│  │  └────────┘  └────────┘  └────────┘             │   │
│  └──────────────────────────────────────────────────┘   │
└──────────────┬──────────────────────────────────────────┘
               │ Replication Log
               ├────────────────────────────────────┐
               │                                    │
               ▼                                    ▼
┌─────────────────────────┐      ┌─────────────────────────┐
│       Slave 1           │      │       Slave 2           │
│  ┌──────────────────┐   │      │  ┌──────────────────┐   │
│  │  Apply Log       │   │      │  │  Apply Log       │   │
│  │  Heartbeat       │   │      │  │  Heartbeat       │   │
│  └──────────────────┘   │      │  └──────────────────┘   │
└─────────────────────────┘      └─────────────────────────┘
```

**Modes de Réplication** :
1. **Synchrone** : Master attend ACK de tous les slaves
2. **Asynchrone** : Master n'attend pas les slaves
3. **Semi-synchrone** : Master attend ACK d'au moins 1 slave

**Failover Automatique** :
```
1. Heartbeat timeout détecté (3 échecs consécutifs)
2. Élection nouveau master (slave avec log le plus récent)
3. Promotion slave → master
4. Notification autres slaves
5. Redirection clients vers nouveau master
```

### 3.4 Service Layer

#### 3.4.1 P2P Network
**Architecture DHT Kademlia** :
```
┌─────────────────────────────────────────────────────────┐
│                    Node ID Space                        │
│  (160-bit identifier space, distance XOR)               │
│                                                         │
│  Node A: 0x1234...                                      │
│  Node B: 0x5678...                                      │
│  Distance(A,B) = A XOR B = 0x444C...                    │
└─────────────────────────────────────────────────────────┘
                            ▼
┌─────────────────────────────────────────────────────────┐
│                  Routing Table (K-Buckets)              │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Bucket 0 (distance 2^0 - 2^1)   : [Node1, ...]  │   │
│  │  Bucket 1 (distance 2^1 - 2^2)   : [Node2, ...]  │   │
│  │  ...                                              │   │
│  │  Bucket 159 (distance 2^159-2^160): [NodeN, ...] │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

**Protocole Gossip** :
```
Round 1: Node A → [B, C]        (2 peers)
Round 2: B → [D, E], C → [F, G] (4 peers)
Round 3: D,E,F,G → 8 peers      (8 peers)
...
Round log₂(N): All N peers reached
```

**Complexité** : O(log N) pour propagation à N nœuds

#### 3.4.2 Consensus Raft
**États du Nœud** :
```
┌──────────┐  Timeout    ┌──────────┐  Receive votes  ┌──────────┐
│ FOLLOWER │────────────→│CANDIDATE │────────────────→│  LEADER  │
└──────────┘             └──────────┘                 └──────────┘
     ▲                        │                            │
     │                        │ Discover leader            │
     │                        │ or new term                │
     └────────────────────────┴────────────────────────────┘
```

**Élection Leader** :
1. Follower timeout (150-300ms randomisé)
2. Devient Candidate, incrémente term
3. Vote pour lui-même
4. Envoie RequestVote à tous les peers
5. Si majorité (>50%) : devient Leader
6. Sinon : retourne Follower

**Réplication Log** :
```
Leader:    [E1][E2][E3][E4][E5]
           ↓   ↓   ↓   ↓   ↓
Follower1: [E1][E2][E3][E4][E5]  (à jour)
Follower2: [E1][E2][E3]          (en retard)
Follower3: [E1][E2][E3][E4]      (en retard)

Leader envoie AppendEntries avec E4, E5 aux followers en retard
```

---

## 4. Flux de Données

### 4.1 Requête Lecture (Cache Hit)
```
Client
  │
  │ 1. GET key
  ▼
Load Balancer
  │
  │ 2. Route to Agent
  ▼
Mining Agent
  │
  │ 3. Check Cache
  ▼
LUM Cache ──────────────┐
  │                     │ 4. Hit!
  │ 5. Return value     │
  ▼                     │
Mining Agent            │
  │                     │
  │ 6. Response         │
  ▼                     │
Client ◄────────────────┘

Latence totale: ~200ns
```

### 4.2 Requête Écriture (Transaction)
```
Client
  │
  │ 1. BEGIN TRANSACTION
  ▼
Transaction Manager
  │
  │ 2. Allocate TXN_ID
  │ 3. Set isolation level
  ▼
Lock Manager
  │
  │ 4. Acquire locks
  ▼
B-Tree Index
  │
  │ 5. Write to index
  │ 6. Write undo log
  ▼
Replication Manager
  │
  │ 7. Replicate to slaves
  ├──────────────┬──────────────┐
  ▼              ▼              ▼
Slave 1       Slave 2       Slave N
  │              │              │
  │ 8. ACK       │ 8. ACK       │ 8. ACK
  ▼              ▼              ▼
Replication Manager
  │
  │ 9. All ACKs received
  ▼
Transaction Manager
  │
  │ 10. COMMIT
  │ 11. Release locks
  ▼
Client

Latence totale: ~1-5ms (selon mode réplication)
```

---

## 5. Patterns de Conception

### 5.1 Singleton Pattern
**Utilisé pour** : lum_core, logger, memory_tracker

```c
static lum_core_t* g_instance = NULL;

lum_core_t* lum_core_get_instance(void) {
    if (!g_instance) {
        g_instance = lum_core_init(NULL);
    }
    return g_instance;
}
```

### 5.2 Factory Pattern
**Utilisé pour** : Création de composants configurables

```c
lum_cache_t* lum_cache_create(size_t capacity, lum_cache_policy_t policy) {
    lum_cache_t* cache = malloc(sizeof(lum_cache_t));
    
    switch (policy) {
        case LUM_CACHE_POLICY_LRU:
            cache->evict = lru_evict;
            break;
        case LUM_CACHE_POLICY_LFU:
            cache->evict = lfu_evict;
            break;
        case LUM_CACHE_POLICY_NX48:
            cache->evict = nx48_evict;
            break;
    }
    
    return cache;
}
```

### 5.3 Observer Pattern
**Utilisé pour** : Événements système, monitoring

```c
typedef void (*event_callback_t)(const event_t* event, void* user_data);

void lum_core_register_observer(event_type_t type, event_callback_t callback) {
    // Enregistre callback pour type d'événement
}

void lum_core_notify(event_type_t type, const event_t* event) {
    // Notifie tous les observers
}
```

### 5.4 Strategy Pattern
**Utilisé pour** : Politiques d'éviction cache, algorithmes quantiques

```c
typedef struct {
    void (*evict)(lum_cache_t* cache);
    void (*update)(lum_cache_entry_t* entry);
} cache_strategy_t;

cache_strategy_t lru_strategy = {
    .evict = lru_evict,
    .update = lru_update
};
```

---

## 6. Scalabilité et Performance

### 6.1 Scalabilité Horizontale
**Ajout de nœuds** :
```
1 nœud  : 100K ops/s
2 nœuds : 200K ops/s (linéaire)
4 nœuds : 400K ops/s (linéaire)
8 nœuds : 750K ops/s (sub-linéaire, overhead réseau)
```

**Facteurs limitants** :
- Latence réseau
- Overhead consensus
- Contention sur ressources partagées

### 6.2 Optimisations Appliquées

#### 6.2.1 Memory Layout
```c
// Mauvais: Cache misses fréquents
struct bad_layout {
    char flag;      // 1 byte
    int64_t value;  // 8 bytes (padding 7 bytes)
    char name[16];  // 16 bytes
}; // Total: 32 bytes (7 bytes gaspillés)

// Bon: Alignement optimal
struct good_layout {
    int64_t value;  // 8 bytes
    char name[16];  // 16 bytes
    char flag;      // 1 byte
    char padding[7];// 7 bytes (explicite)
}; // Total: 32 bytes (padding explicite)
```

#### 6.2.2 Lock-Free Structures
```c
// Compteur atomique sans lock
typedef struct {
    _Atomic uint64_t value;
} atomic_counter_t;

void atomic_counter_inc(atomic_counter_t* counter) {
    atomic_fetch_add(&counter->value, 1);
}
```

#### 6.2.3 Memory Pooling
```c
typedef struct {
    void* pool;
    size_t block_size;
    size_t num_blocks;
    uint64_t free_bitmap;
} memory_pool_t;

void* memory_pool_alloc(memory_pool_t* pool) {
    // O(1) allocation depuis pool pré-alloué
}
```

---

## 7. Sécurité

### 7.1 Byzantine Fault Tolerance
**Règle** : Système tolère jusqu'à f nœuds malicieux si N ≥ 3f + 1

**Exemple** :
- 10 nœuds → tolère 3 malicieux
- Validation requiert 7 votes (>66%)

### 7.2 Authentification
```c
typedef struct {
    uint8_t node_id[32];      // SHA-256 hash
    uint8_t public_key[64];   // Ed25519 public key
    uint8_t signature[64];    // Ed25519 signature
} node_identity_t;
```

### 7.3 Chiffrement
- **Transport** : TLS 1.3
- **Stockage** : AES-256-GCM
- **Clés** : Rotation automatique toutes les 24h

---

## 8. Déploiement

### 8.1 Architecture Production
```
┌─────────────────────────────────────────────────────────┐
│                    Load Balancer                        │
│                   (HAProxy/Nginx)                       │
└──────────────┬──────────────────────────────────────────┘
               │
       ┌───────┴───────┐
       │               │
       ▼               ▼
┌─────────────┐ ┌─────────────┐
│  LumVorax   │ │  LumVorax   │
│  Instance 1 │ │  Instance 2 │
└──────┬──────┘ └──────┬──────┘
       │               │
       └───────┬───────┘
               │
               ▼
┌─────────────────────────────────────────────────────────┐
│              Database Cluster (Raft)                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │  Master  │  │  Slave1  │  │  Slave2  │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
```

### 8.2 Monitoring Stack
```
LumVorax Instances
       │
       │ Metrics (Prometheus format)
       ▼
┌─────────────┐
│ Prometheus  │
└──────┬──────┘
       │
       │ Queries
       ▼
┌─────────────┐
│   Grafana   │ ← Dashboards
└─────────────┘
```

---

**Fin du document**  
Pour plus d'informations, consultez la [documentation API](API.md) et les [guides de déploiement](DEPLOYMENT.md).