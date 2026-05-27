# PROTOCOLE VALIDATION FORENSIQUE ABSOLUE C241
## LumVorax Bitcoin Mining — Level Zero Native Gen9

**Date** : 2026-05-13 09:37 CEST  
**Cycle** : C241  
**Objectif** : Validation scientifique ABSOLUE des capacités réelles  
**Méthode** : Preuves techniques brutes, reproductibles, falsifiables  
**Dépendance OpenCL** : **0%** ✅

---

## 🎯 OBJECTIF PRINCIPAL

Valider ou invalider scientifiquement les capacités réelles du système LumVorax Native Gen9 :

- ✅ Hashrate RÉEL (pas comptage logique)
- ✅ Exécution SHA256 RÉELLE (double SHA256 Bitcoin complet)
- ✅ Stabilité DRM/i915 long terme
- ✅ Absence d'artefacts de mesure
- ✅ Gains RÉELS vs baseline
- ✅ Absence de faux positifs
- ✅ Absence de comptage invalide

**RÈGLE ABSOLUE** : Aucune conclusion qualitative sans données vérifiables.

---

## 📋 CONFIGURATION OBLIGATOIRE

### Hardware Documenté

```bash
# CPU
cat /proc/cpuinfo | grep "model name" | head -1
lscpu | grep -E "Model name|Architecture|CPU\(s\)|Thread|Core"

# GPU exact
lspci -nn | grep VGA
lspci -v -s $(lspci | grep VGA | cut -d' ' -f1)

# RAM
free -h
cat /proc/meminfo | grep -E "MemTotal|MemAvailable"

# Température ambiante
sensors | grep -E "temp|Core"

# Kernel
uname -a
cat /proc/version
```

### Software Documenté

```bash
# Distribution
cat /etc/os-release

# Kernel exact
uname -r
cat /proc/cmdline

# Mesa
glxinfo | grep -E "OpenGL version|Mesa"

# libdrm
dpkg -l | grep libdrm
rpm -qa | grep libdrm

# i915
modinfo i915 | grep -E "version|filename|description"
cat /sys/module/i915/version

# Microcode Intel
dmesg | grep microcode
cat /proc/cpuinfo | grep microcode
```

---

## 🔬 PARTIE 1 — VALIDATION SHA256 RÉELLE

### Objectif

Prouver que :

```
1 iteration LumVorax = 1 vrai double SHA256 Bitcoin complet
```

### TEST U1 — SHA256 Bloc Unique

**Entrée connue** :
```c
// Bitcoin block header (80 bytes)
uint8_t header[80] = {
    0x01, 0x00, 0x00, 0x00, // version
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // prev_block (32 bytes)
    // ... (merkle_root 32 bytes)
    // ... (timestamp 4 bytes)
    // ... (bits 4 bytes)
    0x00, 0x00, 0x00, 0x00  // nonce
};
```

**Validation** :
1. Calculer SHA256(SHA256(header)) avec OpenSSL
2. Calculer SHA256(SHA256(header)) avec LumVorax GPU
3. Comparer bit-à-bit

**Critère succès** : Divergence = 0 bits

### TEST U2 — DOUBLE SHA256 Complet

**Validation** :
- Hash final exact
- Endianness correct (little-endian Bitcoin)
- Padding SHA256 correct
- Midstate correct
- Injection nonce correcte

**Référence** : Bitcoin Core `sha256d()`

### TEST U3 — 1 Million Nonces

**Exécution** :
- 1,000,000 nonces consécutifs
- Comparer CPU baseline vs GPU LumVorax

**Vérifications** :
- Zéro divergence
- Zéro collision anormale
- Zéro hash invalide
- Distribution uniforme

---

## ⚡ PARTIE 2 — VALIDATION HASHRATE RÉEL

### Objectif

Déterminer le vrai throughput cryptographique.

### INTERDICTION ABSOLUE

**NE PAS compter** :
- Iterations internes kernel
- Boucles partielles
- Partial hashes
- Midstates seuls
- Work-items logiques

**COMPTER UNIQUEMENT** :
- Doubles SHA256 Bitcoin COMPLETS
- Validés cryptographiquement

### TEST H1 — Baseline CPU

**Mesurer** :
```bash
# OpenSSL SHA256
openssl speed sha256

# Bitcoin Core miner (si disponible)
bitcoin-cli getmininginfo

# cpuminer
cpuminer --benchmark
```

### TEST H2 — Baseline OpenCL

**Mesurer** :
- OpenCL SHA256 standard (cgminer, bfgminer)
- Kernel OpenCL référence

### TEST H3 — LumVorax Native

**Mesurer exactement** :
```c
uint64_t nonces_tested = dispatches * batch_size;
uint64_t hashes_complete = nonces_tested; // SI ET SEULEMENT SI validé U1-U3
double time_seconds = (end_ns - start_ns) / 1e9;
double hashrate_real = hashes_complete / time_seconds;
```

**Calcul gain réel** :
```
gain_vs_cpu = hashrate_lumvorax / hashrate_cpu_baseline
gain_vs_opencl = hashrate_lumvorax / hashrate_opencl_baseline
```

---

## 🔧 PARTIE 3 — VALIDATION DRM / I915

### Objectif

Déterminer si le système provoque :
- Hangs GPU
- Resets engine
- Fragmentation GEM
- Memory leaks
- Stalls mémoire
- Thermal throttling

### Logging Obligatoire

**Terminal 1** :
```bash
sudo dmesg -w | tee dmesg_c241.log
```

**Terminal 2** :
```bash
sudo journalctl -kf | tee journalctl_c241.log
```

**Terminal 3** :
```bash
./test_btc_mining_c241 | tee execution_c241.log
```

### TEST D1 — GEM Fragmentation

**Mesurer après chaque reopen** :
```bash
# GEM objects
cat /sys/kernel/debug/dri/0/i915_gem_objects

# Aperture usage
cat /sys/kernel/debug/dri/0/i915_gem_gtt

# mmap count
cat /proc/$(pidof test_btc_mining_c241)/maps | wc -l
```

### TEST D2 — Context Reset vs FD Reopen

**Tester 3 stratégies** :

**Stratégie A** : Reopen complet (actuel C240)
```c
close(drm_fd);
drm_fd = open("/dev/dri/renderD128", O_RDWR);
// Recreate contexts + buffers
```

**Stratégie B** : Reset context uniquement
```c
// Garder drm_fd ouvert
drm_ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_DESTROY, &ctx_id);
drm_ioctl(drm_fd, DRM_IOCTL_I915_GEM_CONTEXT_CREATE, &ctx_id);
// Garder buffers GEM
```

**Stratégie C** : GEM persistant
```c
// Garder drm_fd + contexts + buffers
// Reset uniquement batch buffer
memset(batch_buffer, 0, batch_size);
```

**Comparer** :
- Stabilité hashrate
- Overhead reopen
- Fragmentation GEM
- Hangs GPU

### TEST D3 — Long Run 1000 Dispatches

**Exécution** :
- 1000 dispatches minimum
- Batch size 268M
- Logging complet

**Mesurer** :
- Stabilité hashrate (écart-type)
- Temps dispatch (min/max/avg)
- Erreurs ioctl
- Hangs GPU
- Resets engine
- Fragmentation progressive

---

## 💾 PARTIE 4 — VALIDATION MÉMOIRE GPU

### TEST M1 — GEM_GET_APERTURE

**Implémentation** :
```c
struct drm_i915_gem_get_aperture aperture = {0};
int ret = ioctl(drm_fd, DRM_IOCTL_I915_GEM_GET_APERTURE, &aperture);

LOG_EVENT(ctx, "GPU_MEM: total=%lu available=%lu used=%lu",
         aperture.aper_size,
         aperture.aper_available_size,
         aperture.aper_size - aperture.aper_available_size);
```

**Logger** :
- Avant/après chaque reopen
- Avant/après chaque dispatch
- Détecter leaks progressifs

### TEST M2 — mmap Tracking

**Logger** :
```c
void* addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, drm_fd, offset);
LOG_EVENT(ctx, "MMAP: addr=%p size=%lu time=%lu ns", addr, size, time_ns);

munmap(addr, size);
LOG_EVENT(ctx, "MUNMAP: addr=%p size=%lu time=%lu ns", addr, size, time_ns);
```

### TEST M3 — Leak Detection

**Comparer** :
```
dispatch 1 : GEM objects = X
dispatch 1000 : GEM objects = Y

leak = (Y - X) / 1000
```

**Critère succès** : leak < 1 KB/dispatch

---

## 🌡️ PARTIE 5 — VALIDATION THERMIQUE

### Objectif

Confirmer ou invalider thermal throttling.

### Module Monitoring Natif

**Créer** : `src/lum_thermal_monitor_c241.c`

**Fonctionnalités** :
```c
typedef struct {
    double cpu_temp_celsius;
    double gpu_temp_celsius;
    uint32_t cpu_freq_mhz;
    uint32_t gpu_freq_mhz;
    uint32_t gpu_rc6_percent;
    double power_draw_watts;
    uint64_t timestamp_ns;
} lum_thermal_snapshot_t;

int lum_thermal_init(lum_thermal_monitor_t** monitor);
int lum_thermal_snapshot(lum_thermal_monitor_t* monitor, lum_thermal_snapshot_t* snapshot);
int lum_thermal_log(lum_thermal_monitor_t* monitor, const char* log_path);
void lum_thermal_cleanup(lum_thermal_monitor_t* monitor);
```

**Sources de données** :
```bash
# Température CPU
/sys/class/thermal/thermal_zone*/temp

# Température GPU (si disponible)
/sys/class/drm/card0/device/hwmon/hwmon*/temp*_input

# Fréquence GT
/sys/class/drm/card0/gt_cur_freq_mhz
/sys/class/drm/card0/gt_max_freq_mhz

# RC6 (power saving)
/sys/class/drm/card0/power/rc6_residency_ms

# Power draw (si disponible)
/sys/class/hwmon/hwmon*/power*_input
```

### TEST T1 — Corrélation

**Analyser** :
```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('thermal_c241.csv')

# Corrélation temps dispatch vs température
correlation_temp = df['dispatch_time_ms'].corr(df['gpu_temp_celsius'])

# Corrélation temps dispatch vs fréquence
correlation_freq = df['dispatch_time_ms'].corr(df['gpu_freq_mhz'])

print(f"Corrélation dispatch/temp: {correlation_temp}")
print(f"Corrélation dispatch/freq: {correlation_freq}")
```

**Critère throttling** :
- Corrélation > 0.7 ET
- Fréquence GPU diminue > 10% ET
- Température > 80°C

### TEST T2 — Cooling Test

**Protocole** :
1. Run baseline (température ambiante)
2. Run avec ventilateur externe
3. Run avec capot ouvert
4. Run avec température réduite (climatisation)

**Comparer** :
- Hashrate moyen
- Stabilité
- Temps dispatch
- Fréquence GPU

**Si throttling confirmé** :
- Hashrate augmente avec refroidissement
- Fréquence GPU stable avec refroidissement

---

## 🔬 PARTIE 6 — VALIDATION SCIENTIFIQUE

### TEST S1 — Randomization

**Randomiser** :
- Ordre dispatches
- Batch size (128M, 256M, 268M)
- Contexts utilisés
- Pauses entre dispatches

**Vérifier** :
- Hashrate reste proportionnel au batch size
- Pas de pattern suspect

### TEST S2 — Blind Run

**Exécuter sans connaître** :
- Batch courant
- Contexte courant
- Timestamp

**Analyser** :
- Distribution hashrate
- Outliers
- Patterns anormaux

### TEST S3 — Shuffle

**Injecter données aléatoires** :
```c
// Remplacer kernel SHA256 par kernel random
for (int i = 0; i < batch_size; i++) {
    output[i] = rand();
}
```

**Vérifier** :
- Disparition du signal SHA256
- Hashrate devient invalide
- Détection immédiate

---

## 🔐 PARTIE 7 — VALIDATION CRYPTOGRAPHIQUE

### Module Validation SHA256

**Créer** : `src/lum_sha256_validator_c241.c`

**Fonctionnalités** :
```c
typedef struct {
    uint8_t header[80];
    uint32_t nonce;
    uint8_t hash_expected[32];
    uint8_t hash_lumvorax[32];
    uint8_t hash_openssl[32];
    bool match_openssl;
    bool match_expected;
} lum_sha256_test_vector_t;

int lum_sha256_validate_single(const uint8_t* header, uint32_t nonce, uint8_t* hash_out);
int lum_sha256_validate_batch(lum_sha256_test_vector_t* vectors, size_t count);
int lum_sha256_compare_openssl(const uint8_t* data, size_t len, const uint8_t* hash_lumvorax);
```

### TEST C1 — 10,000 Nonces Aléatoires

**Exécution** :
```c
for (int i = 0; i < 10000; i++) {
    uint32_t nonce = rand();
    
    // Hash OpenSSL
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, header, 80);
    SHA256_Final(hash1, &ctx);
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, hash1, 32);
    SHA256_Final(hash_openssl, &ctx);
    
    // Hash LumVorax
    lum_sha256_compute(header, nonce, hash_lumvorax);
    
    // Comparer
    assert(memcmp(hash_openssl, hash_lumvorax, 32) == 0);
}
```

**Critère succès** : 10,000/10,000 matchs exacts

---

## 📊 PARTIE 8 — PREUVES OBLIGATOIRES

### Logs Complets

**Format** :
```
[timestamp_utc_ns] EVENT: details
```

**Contenu** :
- stderr complet
- stdout complet
- dmesg complet
- journalctl complet
- thermal logs
- validation SHA256

**Checksums** :
```bash
sha256sum execution_c241.log > checksums.txt
sha256sum dmesg_c241.log >> checksums.txt
sha256sum thermal_c241.csv >> checksums.txt
```

### Dumps Techniques

**Capturer** :
```bash
# Avant run
lspci -vvv > lspci_before.txt
cat /sys/kernel/debug/dri/0/i915_gem_objects > gem_before.txt
cat /proc/meminfo > meminfo_before.txt

# Pendant run (toutes les 10s)
while true; do
    date +%s >> intel_gpu_top.log
    intel_gpu_top -l 1 >> intel_gpu_top.log
    sleep 10
done

# Après run
lspci -vvv > lspci_after.txt
cat /sys/kernel/debug/dri/0/i915_gem_objects > gem_after.txt
cat /proc/meminfo > meminfo_after.txt
```

### Hashes Validés

**Inclure dans rapport** :
```
nonce=0x12345678
header=0x01000000...
hash_lumvorax=0xabcdef...
hash_openssl=0xabcdef...
match=true
```

### Reproductibilité

**Refaire sur** :
1. Machine 1 (actuelle)
2. Machine 2 (si disponible)
3. Après reboot complet
4. Après recompilation

---

## ✅ CRITÈRES DE VALIDATION

### CAS A — VALIDATION FORTE ✅

- SHA256 exact confirmé (10,000/10,000)
- Throughput confirmé reproductible
- Stabilité >1000 dispatches
- Aucun hang critique
- Gains reproductibles
- Corrélation thermique < 0.3

**Conclusion** : LumVorax représente un breakthrough réel DRM/i915 natif

### CAS B — VALIDATION PARTIELLE ⚠️

- SHA256 valide
- Gains réels
- Mais instabilité i915 (hangs, resets)
- Ou throttling thermique confirmé

**Conclusion** : LumVorax fonctionne mais limité par hardware/driver

### CAS C — INVALIDATION PARTIELLE ❌

- Throughput exagéré (comptage incorrect)
- Partial hashing détecté
- Artefacts de mesure

**Conclusion** : Hashrate annoncé invalide, revoir comptage

### CAS D — INVALIDATION TOTALE ❌

- Hashes invalides (divergence SHA256)
- Faux throughput
- Pipeline non équivalent Bitcoin

**Conclusion** : Système ne calcule pas de vrais SHA256 Bitcoin

---

## 📝 CONCLUSION EXIGÉE

Le rapport final C241 doit répondre explicitement :

1. ✅/❌ Le système exécute-t-il un vrai double SHA256 Bitcoin ?
2. ✅/❌ Le hashrate annoncé est-il physiquement réel ?
3. ✅/❌ Le gain vs OpenCL est-il réel ?
4. ✅/❌ Les gains sont-ils reproductibles ?
5. ✅/❌ Le système est-il stable long terme ?
6. Le système est-il limité par :
   - ❓ Mémoire (fragmentation GEM)
   - ❓ i915 (hangs, resets)
   - ❓ Thermique (throttling)
   - ❓ Architecture Gen9 (EU count, fréquence)
7. LumVorax représente-t-il :
   - ❓ Une optimisation réelle
   - ❓ Un artefact de mesure
   - ❓ Un pipeline invalide
   - ❓ Un breakthrough natif DRM/i915

**RÈGLE ABSOLUE** : Aucune affirmation non prouvée ne doit apparaître.

---

## 🚀 PLAN D'EXÉCUTION C241

### Phase 1 : Modules Monitoring (2h)
1. `lum_thermal_monitor_c241.c` (monitoring thermique natif)
2. `lum_sha256_validator_c241.c` (validation cryptographique)
3. `lum_dmesg_correlator_c241.c` (corrélation dmesg temps réel)

### Phase 2 : Tests Unitaires (1h)
1. TEST U1-U3 (validation SHA256)
2. TEST H1-H3 (hashrate réel)
3. TEST M1-M3 (mémoire GPU)

### Phase 3 : Tests Stratégies (2h)
1. TEST D2 Stratégie A (reopen complet)
2. TEST D2 Stratégie B (reset context)
3. TEST D2 Stratégie C (GEM persistant)

### Phase 4 : Long Run (1h)
1. TEST D3 (1000 dispatches)
2. Monitoring thermique continu
3. Corrélation dmesg

### Phase 5 : Analyse & Rapport (1h)
1. Analyse statistique complète
2. Corrélations thermiques
3. Validation cryptographique
4. Rapport final scientifique

**Durée totale estimée** : 7 heures

---

**Protocole créé par** : Bob (LumVorax Core Team)  
**Date** : 2026-05-13  
**Version** : 1.0  
**Statut** : PRÊT POUR EXÉCUTION

**Mot de passe root** : `emmaus` (requis pour dmesg, debugfs, sensors)