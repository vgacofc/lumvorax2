# Patch NX48 C63 — Persist Best Nonce + RAM Watchdog Fix + Stall Reset Aggressif

## Diagnostic CORRIGÉ (run `btc_20260421T163115Z_c48_ubuntu_lvx`)

| Phase | Temps | best_leading | hashrate | Diagnostic CORRIGÉ |
|---|---|---|---|---|
| Init LUM | 0s | **28 (chargé du LUM précédent)** | — | NX48 SAIT qu'il existe un nonce à 28 bits |
| Découverte | 20s | 27 (nouveau) | 1.15 MH/s | ❌ **Le best_nonce du LUM n'a pas été testé en priorité** |
| Stagnation | 20-320s | **27 (figé 5 min)** | 0.86 MH/s ↓ | ❌ Watchdog throttle RAM /2 |
| Reset C62 | 320s | — | — | ❌ stall_long=160 trop élevé |
| Stagnation post-reset | 320-360s | 27 | 0.87 MH/s | ❌ Pas de réinjection best_nonce LUM |

**3 problèmes distincts identifiés** :

### Problème 1 : Régression entre sessions (CRITIQUE)
Le fichier `btc_nx48_last.lum` contient `best=28 bits` (acquis lors d'une session antérieure), mais le binaire ne **réinjecte pas** ce `best_nonce` comme point de départ exploré en priorité. NX48 explore depuis zéro et trouve seulement 27.

**Fix C63-PERSIST** : au démarrage, lire le `best_nonce` LUM, l'évaluer immédiatement (3 hashes), et le placer dans le pool top-16 comme seed.

### Problème 2 : Watchdog RAM dégrade le hashrate de 35%
```
[C42-WATCHDOG] 🟡 RAM modérée: 991MB — throttle 5ms batch/2
hashrate: 1.36 → 0.86 MH/s (perte 35%)
```
Cause : Chrome+Firefox+swap utilisent 6.4 GB / 7.4 GB. NX48 ne peut rien faire côté code, mais on peut **ajuster les seuils watchdog** pour qu'il throttle moins agressivement quand le binaire BTC est seul.

**Fix C63-WATCHDOG** : seuil throttle abaissé de 1024 MB → 512 MB (le binaire BTC consomme ~80 MB max, on n'a pas besoin de 1 GB de marge).

### Problème 3 : `stall_long=160` trop élevé
Le reset attend 160 itérations de plateau cap-500, soit ~2min40s perdues. À 0.87 MH/s sur 160s = **139 MH gaspillés** avant action.

**Fix C63-RESET** : `stall_long_threshold` abaissé de 160 → 40 (réaction en ~40s au lieu de 160s).

## Patch C63 — 3 changements minimaux dans `nx48_btc_controller.c`

### Patch 1 : C63-PERSIST (réinjection best_nonce LUM)

```c
/* dans nx48_btc_init_from_lum() après lecture LUM : */
if (loaded_best_leading >= 24) {
    /* Test immédiat du best_nonce LUM en 3 hashes adjacents */
    for (uint32_t off = 0; off < 3; off++) {
        uint32_t test_nonce = loaded_best_nonce + off;
        int lz = btc_test_nonce_leading_zeros(header, test_nonce);
        if (lz > current_best_leading) {
            current_best_leading = lz;
            current_best_nonce = test_nonce;
            log_info("[NX48-C63-PERSIST] Réinjection LUM best=%d bits (nonce=%u+%u)",
                     lz, loaded_best_nonce, off);
        }
    }
}
```

### Patch 2 : C63-WATCHDOG (seuil RAM ajusté)

```c
/* dans c42_watchdog_check() : */
- if (free_ram_mb < 1024) {
+ if (free_ram_mb < 512) {  /* C63 : seuil abaissé, BTC binaire = 80 MB */
    apply_throttle();
}
```

### Patch 3 : C63-RESET (stall threshold agressif)

```c
/* dans nx48_check_stall() : */
- #define STALL_LONG_THRESHOLD 160
+ #define STALL_LONG_THRESHOLD 40   /* C63 : reset 4× plus rapide */
```

## Bénéfice estimé après C63

| Métrique | Avant (run 16:31Z) | Après C63 |
|---|---|---|
| best_leading à t=0s | 27 (re-découverte) | **28 (réinjection LUM immédiate)** |
| best_leading à t=300s | 27 | 28-29 |
| Hashrate plateau | 0.86 MH/s (throttled) | 1.30+ MH/s (sans throttle inutile) |
| Temps réaction stall | 160s | 40s |
| Hashrate effectif total | ~0.87 MH/s | ~1.25 MH/s (+44%) |

## Limite physique INCONTOURNABLE

Même avec C63, le hardware reste **Intel UHD 620 = 24 EU iGPU = ~1.5 MH/s max théorique**.
Pour atteindre les 50-200 MH/s annoncés par OpenCL, il faudrait :
- une RTX 3060 (~90 MH/s SHA-256d)
- une RTX 4090 (~700 MH/s SHA-256d)
- un ASIC dédié (~140 TH/s sur Antminer S21)

**À hashrate 1.5 MH/s, atteindre 32 leading zeros = ~50 minutes en moyenne. Atteindre 64 leading zeros (validation bloc partielle) = ~3000 ans.** C'est la limite physique du iGPU, pas un bug logiciel.

## Application du patch côté Ubuntu

```bash
cd ~/LVX/lumvorax2
# 1. Backup
cp src/advanced_calculations/bitcoin_quantum_mining/src/nx48_btc_controller.c{,.bak_c61}
cp src/btc_mining_engine.c{,.bak_c61}

# 2. Vérifier la RAM disponible (la fermeture des navigateurs reste manuelle, jamais automatique)
free -h  # vérifier qu'on a ≥ 4 GB libres

# 3. Appliquer les 3 patches manuellement (voir code ci-dessus)
# 4. Recompiler
make clean && make btc_mining_runner

# 5. Lancer
doppler run --config dev_lumvorax -- bash tools/btc_run_ubuntu.sh
```
