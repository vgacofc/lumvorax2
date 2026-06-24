# STANDARD DE NOMMAGE - MASTER LVX

**Date de Création**: 2026-06-19  
**Version**: 1.0  
**Auteur**: Bob (Expert IA + Standards & Gouvernance)  
**Projet**: LumVorax - Standardisation Nomenclature  

---

## 📋 OBJECTIF

Ce document définit les **conventions de nommage standardisées** pour tous les fichiers, répertoires, variables, fonctions et artefacts du projet LumVorax, garantissant cohérence, traçabilité et maintenabilité.

---

## 🗂️ STRUCTURE RÉPERTOIRES

### Répertoires Principaux

```
/home/lvx/LVX/lumvorax2/
├── master_lvx/                    # Documentation validation officielle
├── src/                           # Code source
├── tests/                         # Tests unitaires/intégration
├── benchmarks/                    # Benchmarks performance
├── logs/                          # Logs d'exécution
├── results/                       # Résultats tests/benchmarks
├── docs/                          # Documentation technique
├── scripts/                       # Scripts automatisation
├── config/                        # Fichiers configuration
└── artifacts/                     # Artefacts build/compilation
```

### Sous-Répertoires Standards

```
src/
├── advanced_calculations/         # Calculs avancés
│   └── bitcoin_quantum_mining/    # Mining Bitcoin quantique
│       └── level_zero_native/     # Implémentation Level Zero
├── nx_versions/                   # Moteurs NX (NX48, NX49, etc.)
├── MAGEN/                         # Modules MAGEN (93 modules)
├── quantum_simulators/            # Simulateurs quantiques
├── logger/                        # Systèmes logging
└── utils/                         # Utilitaires communs
```

---

## 📄 CONVENTIONS FICHIERS

### Fichiers Documentation (Markdown)

**Format**: `TYPE_DESCRIPTION_CONTEXTE_YYYYMMDD.md`

**Exemples**:
- `README_MASTER_LVX.md` - Documentation principale
- `PLAN_TESTS_SCIENTIFIQUES_LUMVORAX_20260619.md` - Plan tests
- `RAPPORT_C582_ANALYSE_FORENSIQUE_ROOT_CAUSES_116_117.md` - Rapport analyse
- `GUIDE_INSTALLATION_EXECUTION_BENCHMARKS.md` - Guide utilisateur
- `STANDARD_NAMES_MASTER_LVX.md` - Ce fichier
- `PROTOCOLE_MASTER_LVX.md` - Protocoles opérationnels
- `LEÇONS_APPRISES_MASTER_LVX.md` - Retours d'expérience

**Types Standards**:
- `README` - Documentation principale
- `PLAN` - Plans/roadmaps
- `RAPPORT` - Rapports d'analyse
- `GUIDE` - Guides utilisateur
- `STANDARD` - Standards/conventions
- `PROTOCOLE` - Protocoles opérationnels
- `LEÇONS` - Leçons apprises
- `AUDIT` - Audits techniques
- `ANALYSE` - Analyses détaillées
- `PRESENTATION` - Présentations
- `ARTICLE` - Articles scientifiques

### Fichiers Code Source (C/C++)

**Format**: `module_description.c` ou `module_description.h`

**Exemples**:
- `btc_gen9_native_runner.c` - Runner Bitcoin Gen9
- `forensic_logger.h` - Logger forensique
- `memory_tracker.c` - Tracker mémoire
- `lum_logger.h` - Logger LUM
- `quantum_simulator.c` - Simulateur quantique

**Conventions**:
- Minuscules avec underscores (`snake_case`)
- Préfixe module (`btc_`, `lum_`, `nx_`)
- Suffixe type (`_runner`, `_logger`, `_tracker`)

### Fichiers Python

**Format**: `module_description.py`

**Exemples**:
- `nx48_unified_ultra.py` - Moteur NX48
- `nx49_biological_neuron.py` - Moteur NX49
- `spatial_knowledge_graph.py` - Module MAGEN
- `trajectory_analyzer.py` - Analyseur trajectoires

**Conventions**:
- Minuscules avec underscores (`snake_case`)
- Noms descriptifs et explicites
- Préfixe version si applicable (`nx48_`, `v2_`)

### Fichiers Scripts (Bash)

**Format**: `action_description.sh`

**Exemples**:
- `install_benchmarks.sh` - Installation benchmarks
- `run_all_benchmarks.sh` - Exécution complète
- `forensic_wrapper.sh` - Wrapper forensique
- `validate_results.sh` - Validation résultats
- `test_c282_progressive_batch.sh` - Test progressif

**Conventions**:
- Minuscules avec underscores
- Verbe d'action en préfixe (`install_`, `run_`, `validate_`)
- Extension `.sh` obligatoire

### Fichiers Tests

**Format**: `test_cXXX_description.c` ou `test_description.py`

**Exemples**:
- `test_c582_pass0_reference.c` - Test C582 pass 0
- `test_c583_single_relocation.c` - Test C583 relocation
- `test_gen9_atomics.cl` - Test atomics Gen9
- `test_c293_atomics_runner.c` - Runner test C293

**Conventions**:
- Préfixe `test_` obligatoire
- Numéro test si applicable (`c582`, `c583`)
- Description explicite

### Fichiers Logs

**Format**: `type_description_YYYYMMDD_HHMMSS.log`

**Exemples**:
- `execution_c289_10s_20260520_231258.log` - Log exécution
- `execution_c291_test_kernel_20260521_000725.log` - Log test kernel
- `forensic_hpl_baseline_20260619_120000.log` - Log forensique HPL
- `monitoring_cpu_20260619_120000.log` - Log monitoring CPU

**Conventions**:
- Type en préfixe (`execution_`, `forensic_`, `monitoring_`)
- Timestamp obligatoire (YYYYMMDD_HHMMSS)
- Extension `.log` obligatoire

### Fichiers Résultats

**Format**: `benchmark_config_YYYYMMDD_HHMMSS.txt`

**Exemples**:
- `hpl_baseline_20260619_120000.txt` - Résultats HPL baseline
- `hpcg_optimized_20260619_130000.txt` - Résultats HPCG optimisé
- `graph500_bfs_scale20_20260619_140000.txt` - Résultats Graph500
- `stream_cpu_20260619_150000.txt` - Résultats STREAM CPU

**Conventions**:
- Nom benchmark en préfixe
- Configuration/variante
- Timestamp obligatoire
- Extension `.txt` ou `.json`

---

## 🏷️ CONVENTIONS VARIABLES

### Variables C/C++

**Format**: `type_description` (snake_case)

**Exemples**:
```c
// Variables globales
uint64_t timestamp_ns;
uint32_t hash_count;
double performance_gflops;

// Variables locales
int result_code;
size_t buffer_size;
char* file_path;

// Constantes
#define MAX_BUFFER_SIZE 4096
#define DEFAULT_TIMEOUT_MS 1000
```

**Conventions**:
- Minuscules avec underscores
- Type implicite dans le nom si pertinent
- Constantes en MAJUSCULES

### Variables Python

**Format**: `description` (snake_case)

**Exemples**:
```python
# Variables
timestamp_ns = time.time_ns()
hash_rate_mhs = 2.667
file_path = "/path/to/file"

# Constantes
MAX_ITERATIONS = 1000000
DEFAULT_BATCH_SIZE = 256

# Classes
class NX48UnifiedUltra:
    pass
```

**Conventions**:
- Variables: snake_case
- Constantes: UPPER_SNAKE_CASE
- Classes: PascalCase

### Variables Bash

**Format**: `DESCRIPTION` (UPPER_SNAKE_CASE)

**Exemples**:
```bash
# Variables globales
BASE_DIR=~/lumvorax_benchmarks
RESULTS_DIR=$BASE_DIR/results
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Variables locales
local test_name="hpl_baseline"
local duration_ns=0
```

**Conventions**:
- Variables globales: UPPER_SNAKE_CASE
- Variables locales: snake_case avec `local`

---

## 🔧 CONVENTIONS FONCTIONS

### Fonctions C/C++

**Format**: `module_action_description()`

**Exemples**:
```c
// Fonctions publiques
int btc_gen9_init_device(void);
uint64_t time_ns_get_absolute(void);
void forensic_log(int level, const char* func, const char* fmt, ...);
void memory_tracker_init(void);

// Fonctions privées (static)
static int parse_config_file(const char* path);
static void cleanup_resources(void);
```

**Conventions**:
- Préfixe module (`btc_`, `time_`, `forensic_`)
- Verbe d'action (`init`, `get`, `log`, `parse`)
- snake_case
- Fonctions privées: `static`

### Fonctions Python

**Format**: `action_description()`

**Exemples**:
```python
def analyze_nonce_space(start, end):
    pass

def calculate_hash_rate(hashes, duration_ns):
    pass

def export_results_json(data, filepath):
    pass

class NX48UnifiedUltra:
    def process_multi_file(self, files):
        pass
    
    def _internal_helper(self):  # Privée
        pass
```

**Conventions**:
- snake_case
- Verbe d'action en préfixe
- Méthodes privées: préfixe `_`

---

## 📊 CONVENTIONS TESTS

### Identifiants Tests

**Format**: `TEST-CATEGORY-XXX`

**Exemples**:
- `TEST-HPL-001` - HPL Baseline
- `TEST-HPCG-002` - HPCG Optimisé
- `TEST-GREEN500-001` - Consommation HPL
- `TEST-GRAPH500-002` - BFS Optimisé
- `TEST-STREAM-003` - STREAM GPU

**Catégories Standards**:
- `HPL` - High Performance Linpack
- `HPCG` - High Performance Conjugate Gradient
- `GREEN500` - Efficacité énergétique
- `GRAPH500` - Analyse graphes
- `IO500` - Performance stockage
- `STREAM` - Bande passante mémoire
- `NETWORK` - Latence réseau
- `UNIT` - Tests unitaires
- `INTEGRATION` - Tests intégration
- `STRESS` - Tests stress
- `PERFORMANCE` - Tests performance

### Numérotation Tests

**Format**: `XXX` (001-999)

**Règles**:
- 001-099: Tests baseline
- 100-199: Tests optimisés
- 200-299: Tests large scale
- 300-399: Tests stress
- 400-499: Tests validation
- 500-599: Tests reproductibilité
- 600-699: Tests régression
- 700-799: Tests intégration
- 800-899: Tests acceptance
- 900-999: Tests spéciaux

---

## 🔐 CONVENTIONS CHECKSUMS

### Fichiers Checksums

**Format**: `checksums_YYYYMMDD_HHMMSS.sha256`

**Exemples**:
- `checksums_20260619_120000.sha256` - Checksums session
- `hpl_baseline_20260619_120000.txt.sha256` - Checksum fichier

**Conventions**:
- Extension `.sha256` obligatoire
- Timestamp pour sessions complètes
- Nom fichier original pour checksums individuels

### Format Contenu

```
# Checksums SHA-256 - Session 20260619_120000
a1b2c3d4e5f6... hpl_baseline.txt
f6e5d4c3b2a1... hpcg_baseline.txt
1234567890ab... graph500_bfs_scale20.txt
```

---

## 📈 CONVENTIONS MÉTRIQUES

### Noms Métriques Standards

**Performance**:
- `rmax_gflops` - Performance HPL réelle (GFLOPS)
- `rpeak_gflops` - Performance HPL théorique (GFLOPS)
- `efficiency_percent` - Efficacité (Rmax/Rpeak %)
- `hash_rate_mhs` - Taux de hachage (MH/s)
- `teps` - Traversed Edges Per Second (Graph500)
- `triad_gbs` - Bande passante STREAM Triad (GB/s)

**Temps**:
- `timestamp_ns` - Timestamp nanoseconde
- `duration_ns` - Durée nanoseconde
- `duration_s` - Durée secondes
- `latency_us` - Latence microsecondes

**Énergie**:
- `power_idle_w` - Puissance idle (Watts)
- `power_active_w` - Puissance active (Watts)
- `power_avg_w` - Puissance moyenne (Watts)
- `energy_total_j` - Énergie totale (Joules)
- `flops_per_watt` - FLOPS/Watt (Green500)

**Mémoire**:
- `memory_used_mb` - Mémoire utilisée (MB)
- `memory_peak_mb` - Pic mémoire (MB)
- `bandwidth_gbs` - Bande passante (GB/s)

---

## 🏗️ CONVENTIONS ARCHITECTURE

### Modules Standards

**Préfixes Modules**:
- `btc_` - Bitcoin mining
- `nx_` - Moteurs NX (NX48, NX49)
- `lum_` - Core LumVorax
- `forensic_` - Traçabilité forensique
- `memory_` - Gestion mémoire
- `time_` - Gestion temps
- `quantum_` - Simulateurs quantiques
- `magen_` - Modules MAGEN

**Suffixes Modules**:
- `_runner` - Exécuteur principal
- `_logger` - Logger
- `_tracker` - Tracker
- `_analyzer` - Analyseur
- `_simulator` - Simulateur
- `_processor` - Processeur
- `_manager` - Gestionnaire

### Versions

**Format**: `vX.Y.Z` ou `vXX`

**Exemples**:
- `v1.0.0` - Version sémantique complète
- `v28` - Version majeure simple
- `nx48` - Version moteur NX
- `c582` - Version test/commit

**Conventions**:
- Majeure (X): Changements incompatibles
- Mineure (Y): Nouvelles fonctionnalités compatibles
- Patch (Z): Corrections bugs

---

## 📝 CONVENTIONS COMMENTAIRES

### Commentaires C/C++

```c
/**
 * @brief Description courte de la fonction
 * @param param1 Description paramètre 1
 * @param param2 Description paramètre 2
 * @return Description valeur retour
 * 
 * Description détaillée si nécessaire.
 * Peut s'étendre sur plusieurs lignes.
 */
int function_name(int param1, char* param2);

// Commentaire ligne simple pour clarification
int result = calculate_value();  // Commentaire inline
```

### Commentaires Python

```python
def function_name(param1: int, param2: str) -> int:
    """
    Description courte de la fonction.
    
    Args:
        param1: Description paramètre 1
        param2: Description paramètre 2
    
    Returns:
        Description valeur retour
    
    Raises:
        ValueError: Si param1 < 0
    """
    # Commentaire ligne simple
    result = calculate_value()  # Commentaire inline
    return result
```

### Commentaires Bash

```bash
################################################################################
# TITRE SECTION MAJEURE
# Description section
################################################################################

# Commentaire fonction
function_name() {
    local param=$1  # Description paramètre
    
    # Commentaire bloc
    # Peut s'étendre sur plusieurs lignes
    echo "Result: $param"
}
```

---

## 🎯 CONVENTIONS COMMITS GIT

### Format Messages Commit

**Format**: `type(scope): description courte`

**Types Standards**:
- `feat` - Nouvelle fonctionnalité
- `fix` - Correction bug
- `docs` - Documentation
- `style` - Formatage code
- `refactor` - Refactoring
- `perf` - Optimisation performance
- `test` - Ajout/modification tests
- `build` - Build/compilation
- `ci` - Intégration continue
- `chore` - Maintenance

**Exemples**:
```
feat(btc): add Gen9 native runner with i915 DRM
fix(memory): correct leak in tracker cleanup
docs(master_lvx): add standardization guide
perf(nx48): optimize multi-file processing
test(hpl): add baseline benchmark test
```

### Branches Git

**Format**: `type/description-courte`

**Types**:
- `feature/` - Nouvelles fonctionnalités
- `bugfix/` - Corrections bugs
- `hotfix/` - Corrections urgentes
- `release/` - Préparation releases
- `docs/` - Documentation
- `test/` - Tests

**Exemples**:
```
feature/nx48-integration
bugfix/memory-leak-tracker
hotfix/critical-gpu-crash
release/v1.0.0
docs/master-lvx-standards
test/hpl-baseline
```

---

## 📊 CONVENTIONS RAPPORTS

### Titres Sections Standards

**Sections Obligatoires**:
1. `RÉSUMÉ EXÉCUTIF` - Vue d'ensemble
2. `OBJECTIFS` - Buts et cibles
3. `MÉTHODOLOGIE` - Approche utilisée
4. `RÉSULTATS` - Données et métriques
5. `ANALYSE` - Interprétation résultats
6. `RECOMMANDATIONS` - Actions suggérées
7. `CONCLUSION` - Synthèse finale
8. `ANNEXES` - Données supplémentaires

**Sections Optionnelles**:
- `CONTEXTE` - Background projet
- `ARCHITECTURE` - Description technique
- `VALIDATION` - Critères validation
- `LIMITATIONS` - Contraintes identifiées
- `PROCHAINES ÉTAPES` - Roadmap future

### Niveaux Titres Markdown

```markdown
# Titre Niveau 1 - Document Principal
## Titre Niveau 2 - Section Majeure
### Titre Niveau 3 - Sous-Section
#### Titre Niveau 4 - Détail
```

**Conventions**:
- Niveau 1: Titre document uniquement
- Niveau 2: Sections principales
- Niveau 3: Sous-sections
- Niveau 4: Détails (éviter si possible)

---

## ✅ CHECKLIST CONFORMITÉ

### Avant Création Fichier

- [ ] Nom respecte convention (type_description_date.ext)
- [ ] Extension appropriée (.md, .c, .py, .sh, .log)
- [ ] Répertoire correct (master_lvx/, src/, tests/, etc.)
- [ ] Timestamp si applicable (YYYYMMDD_HHMMSS)

### Avant Commit Code

- [ ] Variables suivent snake_case (C/Python) ou UPPER_SNAKE_CASE (Bash)
- [ ] Fonctions préfixées par module
- [ ] Commentaires présents et formatés
- [ ] Tests nommés test_xxx
- [ ] Message commit formaté (type(scope): description)

### Avant Publication Rapport

- [ ] Titre document niveau 1 présent
- [ ] Sections standards incluses
- [ ] Métriques nommées selon convention
- [ ] Checksums générés si applicable
- [ ] Timestamp dans nom fichier

---

## 🔄 MAINTENANCE STANDARDS

### Révision Périodique

**Fréquence**: Trimestrielle ou après changements majeurs

**Actions**:
1. Vérifier conformité nouveaux fichiers
2. Identifier déviations standards
3. Mettre à jour conventions si nécessaire
4. Communiquer changements à l'équipe

### Évolution Standards

**Processus**:
1. Proposition changement (issue/PR)
2. Discussion équipe
3. Validation consensus
4. Mise à jour document
5. Migration code existant si nécessaire

---

## 📚 RÉFÉRENCES

### Documents Liés

- [`README_MASTER_LVX.md`](README_MASTER_LVX.md) - Documentation principale
- [`PROTOCOLE_MASTER_LVX.md`](PROTOCOLE_MASTER_LVX.md) - Protocoles opérationnels
- [`LEÇONS_APPRISES_MASTER_LVX.md`](LEÇONS_APPRISES_MASTER_LVX.md) - Retours d'expérience

### Standards Externes

- **TOP500**: https://www.top500.org/project/call-for-participation/
- **Green500**: https://www.top500.org/lists/green500/
- **Graph500**: https://graph500.org/?page_id=12
- **IO500**: https://io500.org/about
- **Semantic Versioning**: https://semver.org/
- **Conventional Commits**: https://www.conventionalcommits.org/

---

**STATUT**: ✅ STANDARD COMPLET - VERSION 1.0

**Auteur**: Bob (Expert IA + Standards & Gouvernance)  
**Date**: 2026-06-19  
**Version**: 1.0  
**Prochaine Révision**: 2026-09-19  

---

*Ce document définit les standards de nommage pour garantir cohérence, traçabilité et maintenabilité du projet LumVorax. Toute déviation doit être justifiée et documentée.*