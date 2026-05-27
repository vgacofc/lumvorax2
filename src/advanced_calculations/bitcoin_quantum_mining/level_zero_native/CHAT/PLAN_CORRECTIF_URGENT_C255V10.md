# PLAN CORRECTIF URGENT C255V10
## Corrections Immédiates Requises par l'Utilisateur

**Date**: 2026-05-16  
**Priorité**: CRITIQUE  

---

## 🔴 CORRECTIONS OBLIGATOIRES

### 1. BATCH SIZE ADAPTATIF - CORRECTION ALGORITHME

**PROBLÈME ACTUEL**: L'algorithme s'arrête à 524K au lieu de continuer jusqu'à 1000M

**EXIGENCE UTILISATEUR**:
- ✅ **TOUJOURS démarrer à 256K** (262,144)
- ✅ **Progression dynamique**: 256K → 512K → 1M → 2M → 4M → ... → 1000M
- ✅ **Gestion par NX48/NX49**: Le système doit gérer automatiquement

**ALGORITHME CORRIGÉ**:
```c
#define MIN_BATCH_SIZE 262144      // 256K - TOUJOURS le départ
#define MAX_BATCH_SIZE 1048576000  // 1000M - Maximum autorisé

uint32_t batch_size = MIN_BATCH_SIZE;  // TOUJOURS 256K au départ

// Progression dynamique
if (success_rate == 100% && hashrate_improving) {
    batch_size = min(batch_size * 2, MAX_BATCH_SIZE);  // Doubler jusqu'à 1000M
} else if (success_rate < 90%) {
    batch_size = max(batch_size / 2, MIN_BATCH_SIZE);  // Diviser par 2, minimum 256K
}
```

**FICHIER À MODIFIER**: [`src/btc_gen9_multi_dispatch_c255.c`](../src/btc_gen9_multi_dispatch_c255.c:42-45)

---

### 2. DÉSACTIVER PROTECTION TIMEOUT

**PROBLÈME ACTUEL**: Protection timeout à 18s limite l'exécution

**EXIGENCE UTILISATEUR**:
- ❌ **DÉSACTIVER** la protection timeout
- 🔍 **OBJECTIF**: Voir si ça continue à crasher (errno=5) ou si c'est un problème de mémoire/cache GPU

**CODE À MODIFIER**:
```c
// AVANT (ligne ~436 dans les logs)
if (elapsed_time > TIMEOUT_THRESHOLD) {
    stop_execution();  // ❌ À DÉSACTIVER
}

// APRÈS
// if (elapsed_time > TIMEOUT_THRESHOLD) {
//     stop_execution();  // DÉSACTIVÉ pour test forensique
// }
```

**RAISON**: Identifier si les crashes sont dus à:
1. Timeout driver i915
2. Dépassement mémoire GPU
3. Saturation cache GPU
4. Autre cause

---

### 3. AFFICHAGE QUANTITÉS BRUTES (PAS DE CONVERSION)

**PROBLÈME ACTUEL**: Conversions MH/s, GH/s causent des erreurs d'arrondi

**EXIGENCE UTILISATEUR**:
- ✅ **Afficher HASHES BRUTS** (pas de conversion)
- ✅ **Afficher TEMPS BRUT** en secondes
- ✅ **Calcul hashrate**: Laisser l'utilisateur faire

**CODE CORRIGÉ**:
```c
// AVANT
printf("Hashrate: %.2f GH/s\n", hashrate_ghs);  // ❌ Conversion erronée

// APRÈS
printf("Total Hashes: %lu\n", total_hashes);           // ✅ Valeur brute
printf("Total Time: %.6f sec\n", total_time);          // ✅ Temps brut
printf("Hashrate (raw): %lu hashes/sec\n", 
       total_hashes / (uint64_t)total_time);           // ✅ Calcul brut
```

**EXEMPLE SORTIE**:
```
Total Hashes: 11010048
Total Time: 20.000000 sec
Hashrate (raw): 550502 hashes/sec
```

---

### 4. DISPATCHES DYNAMIQUES (3, 6, 9, ...)

**PROBLÈME ACTUEL**: Nombre de dispatches fixé à 6

**EXIGENCE UTILISATEUR**:
- ✅ **Dispatches dynamiques**: 3, 6, 9, 12, ... (multiples de 3)
- ✅ **Adaptation automatique** selon performance

**ALGORITHME CORRIGÉ**:
```c
#define MIN_DISPATCHES 3
#define MAX_DISPATCHES 27  // 9 contextes × 3

uint32_t dispatches_per_batch = MIN_DISPATCHES;  // Démarrer à 3

// Progression dynamique
if (success_rate == 100% && exec_time_stable) {
    dispatches_per_batch = min(dispatches_per_batch + 3, MAX_DISPATCHES);
} else if (success_rate < 90%) {
    dispatches_per_batch = max(dispatches_per_batch - 3, MIN_DISPATCHES);
}
```

**PROGRESSION ATTENDUE**:
- Batch 1: 3 dispatches (test)
- Batch 2: 6 dispatches (si succès 100%)
- Batch 3: 9 dispatches (si succès 100%)
- Batch 4: 12 dispatches (si succès 100%)
- ...
- Batch N: 27 dispatches (maximum)

---

### 5. SOUMISSIONS TESTNET3 BITCOIN

**PROBLÈME ACTUEL**: Aucune soumission réelle au réseau Bitcoin Testnet3

**EXIGENCE UTILISATEUR**:
- 🔴 **URGENT**: Implémenter soumissions Testnet3
- ✅ **Intégration NX48/NX49**: Système de soumission automatique

**ARCHITECTURE REQUISE**:

#### A. Connexion Testnet3
```c
// Paramètres Testnet3
#define TESTNET3_RPC_HOST "127.0.0.1"
#define TESTNET3_RPC_PORT 18332
#define TESTNET3_RPC_USER "bitcoin"
#define TESTNET3_RPC_PASS "password"

// Connexion RPC
int connect_testnet3() {
    // Implémenter connexion JSON-RPC
    // Vérifier getblocktemplate
    // Récupérer work actuel
}
```

#### B. Récupération Block Template
```c
typedef struct {
    uint8_t prev_block_hash[32];
    uint32_t version;
    uint32_t bits;
    uint32_t time;
    uint8_t merkle_root[32];
    uint64_t height;
} btc_block_template_t;

btc_block_template_t* get_block_template() {
    // Appel RPC: getblocktemplate
    // Parser JSON
    // Retourner template
}
```

#### C. Construction Block Header
```c
typedef struct {
    uint32_t version;
    uint8_t prev_block[32];
    uint8_t merkle_root[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} btc_block_header_t;

btc_block_header_t* build_block_header(btc_block_template_t* tmpl) {
    // Construire header 80 octets
    // Préparer pour mining
}
```

#### D. Soumission Solution
```c
int submit_solution(btc_block_header_t* header, uint32_t nonce) {
    if (hash_meets_target(header, nonce)) {
        // Appel RPC: submitblock
        // Vérifier acceptation
        return 1;  // Accepté
    }
    return 0;  // Rejeté
}
```

#### E. Intégration dans Boucle Mining
```c
// Boucle principale
while (1) {
    // 1. Récupérer work
    btc_block_template_t* tmpl = get_block_template();
    
    // 2. Construire header
    btc_block_header_t* header = build_block_header(tmpl);
    
    // 3. Miner sur GPU
    uint32_t nonce = mine_on_gpu(header, batch_size, dispatches);
    
    // 4. Vérifier solution
    if (hash_meets_target(header, nonce)) {
        // 5. Soumettre à Testnet3
        if (submit_solution(header, nonce)) {
            printf("✅ BLOCK FOUND! Height: %lu\n", tmpl->height);
        }
    }
    
    // 6. Répéter
}
```

**FICHIERS À CRÉER**:
- `src/btc_testnet3_rpc.c` - Client RPC Testnet3
- `src/btc_testnet3_rpc.h` - Headers RPC
- `include/btc_block_template.h` - Structures block template

---

## 📋 CHECKLIST IMPLÉMENTATION

### Phase 1: Corrections Immédiates (1-2h)
- [ ] Modifier algorithme batch adaptatif (256K → 1000M)
- [ ] Désactiver protection timeout
- [ ] Corriger affichage quantités brutes
- [ ] Implémenter dispatches dynamiques (3, 6, 9)

### Phase 2: Intégration Testnet3 (3-4h)
- [ ] Créer client RPC Testnet3
- [ ] Implémenter getblocktemplate
- [ ] Implémenter submitblock
- [ ] Intégrer dans boucle mining

### Phase 3: Tests Forensiques (2-3h)
- [ ] Exécuter sans timeout protection
- [ ] Observer crashes errno=5
- [ ] Analyser logs forensiques
- [ ] Identifier cause réelle crashes

---

## 🎯 OBJECTIFS FINAUX

1. **Batch adaptatif**: 256K → 512K → 1M → ... → 1000M (dynamique)
2. **Dispatches dynamiques**: 3 → 6 → 9 → ... → 27 (adaptatif)
3. **Pas de timeout**: Exécution jusqu'au crash (forensique)
4. **Affichage brut**: Hashes et temps sans conversion
5. **Testnet3**: Soumissions réelles au réseau Bitcoin

---

## 📊 MÉTRIQUES ATTENDUES

**Avec batch 1000M et dispatches 27**:
- Hashes par batch: 1,000,000,000 × 27 = **27,000,000,000 hashes**
- Temps estimé: ~300s (5 min) par batch
- Hashrate estimé: 27G / 300 = **90 MH/s**
- Amélioration vs actuel: 90 MH/s / 0.55 MH/s = **163× plus rapide**

---

**PROCHAINE ÉTAPE**: Implémenter ces corrections dans le code source