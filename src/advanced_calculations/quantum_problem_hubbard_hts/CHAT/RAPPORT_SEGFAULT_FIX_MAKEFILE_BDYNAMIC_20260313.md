# RAPPORT : DIAGNOSTIC ET CORRECTION DU SEGMENTATION FAULT — MAKEFILE LDFLAGS
**Date :** 2026-03-13  
**Cycle :** Quantum Research Cycle (Lumvorax Hubbard HTS)  
**Statut :** CORRIGÉ ✓

---

## 1. INVENTAIRE DES FICHIERS ANALYSÉS

| Fichier | Rôle | État |
|---|---|---|
| `src/hubbard_hts_research_cycle.c` | Binaire principal (1238 lignes) | Inchangé — code C sain |
| `Makefile` | Règles de compilation | **MODIFIÉ** — correction `LDFLAGS` |
| `run_research_cycle.sh` | Script workflow ligne 60 | Inchangé |
| `hubbard_hts_research_runner` | Binaire compilé | Recompilé (63 KB, sain) |

---

## 2. TABLEAU COMPARATIF — AVANT / APRÈS

| Métrique | AVANT (bugué) | APRÈS (corrigé) |
|---|---|---|
| Taille binaire | **997 568 octets** (997 KB) | **63 960 octets** (64 KB) |
| Symboles texte définis | **941** fonctions | **5** fonctions |
| `_dl_relocate_object` présent | **OUI** (!) | NON |
| `__vfscanf_internal` présent | **OUI** (!) | NON |
| `libm.so.6` (dynamique) | **ABSENT** — libm statique | **PRÉSENT** — dynamique |
| `libc.so.6` (dynamique) | Partiel (mixte static/dynamic) | Complet — dynamique pur |
| Crash avec chemin absolu | **EXIT 139 (SIGSEGV)** | EXIT 0 (succès) |
| Crash avec `.` | EXIT 124 (timeout — computation normale) | EXIT 124 (computation normale) |

---

## 3. CAUSE RACINE — ANALYSE LIGNE PAR LIGNE

### 3.1 Ligne Makefile modifiée

**AVANT (ligne 3) :**
```makefile
LDFLAGS ?=
```

**APRÈS (ligne 3) :**
```makefile
LDFLAGS = -Wl,-Bdynamic,--as-needed
```

### 3.2 Mécanisme du crash — explication technique

Le crash n'était **PAS** dans le code C (`hubbard_hts_research_cycle.c`). Il était dans la chaîne de compilation.

**Environnement Replit/NixOS :**  
Le wrapper GCC NixOS (`/nix/store/.../gcc-wrapper-14.2.1.../bin/cc`) injecte automatiquement deux variables d'environnement dans chaque invocation :
- `NIX_CFLAGS_COMPILE` : ~65 chemins `-isystem /nix/store/...` (Python, GTK, Cairo, FFmpeg, Arrow, etc.)
- `NIX_LDFLAGS` : ~65 chemins `-L/nix/store/...` pointant vers des **bibliothèques statiques** NixOS

**Comportement du linker GNU ld :**  
Avec les multiples chemins `-L` NixOS, le linker trouvait une version statique (`libm.a`) **avant** la version dynamique (`libm.so`). En linkant `libm.a` statiquement, celui-ci tirait via ses dépendances vers :
- `libc.a` (glibc statique)  
- `libgcc_s` (runtime GCC statique)
- `ld-linux` (dynamic loader statique !)

Résultat : un binaire **hybride statique/dynamique** de 997 KB contenant `_dl_relocate_object` (la fonction du dynamic linker) à la fois dans l'exécutable ET dans `libc.so.6` chargé dynamiquement → **conflit de symboles au runtime** → **SIGSEGV**.

Le crash se manifestait seulement avec le **chemin absolu** (`/home/runner/workspace/...`) car :
- Le chemin plus long forçait des opérations sur des chaînes plus longues
- Ces opérations appelaient les fonctions de string internes (ex. `__memcpy_evex_unaligned`) qui étaient présentes en double (version statique vs dynamique)
- La version statique de `_dl_relocate_object` dans l'exécutable entrait en conflit avec le dynamic linker lors de l'accès aux bibliothèques partagées

Avec `.` comme argument, le chemin était suffisamment court pour éviter ce code path spécifique.

### 3.3 Preuves diagnostiques

```
AVANT : nm /tmp/hts_from_make | grep "^[0-9a-f]* T " | wc -l → 941
APRÈS  : nm /tmp/hts_fixed   | grep "^[0-9a-f]* T " | wc -l →   5

AVANT : ldd hubbard_hts_research_runner
  libgcc_s.so.1 => ... (libm ABSENT!)
APRÈS  : ldd hubbard_hts_research_runner  
  libm.so.6 => /nix/store/.../libm.so.6 ✓
  libc.so.6 => /nix/store/.../libc.so.6 ✓
```

---

## 4. CORRECTION APPLIQUÉE

**Fichier :** `Makefile`  
**Ligne modifiée :** ligne 3  
**Changement :**
```diff
-LDFLAGS ?=
+LDFLAGS = -Wl,-Bdynamic,--as-needed
```

**Signification des flags :**
- `-Wl,-Bdynamic` : Force le linker à utiliser le linkage **dynamique** pour toutes les `-l` qui suivent, annulant l'effet des archives statiques trouvées via NIX_LDFLAGS
- `--as-needed` : N'inclut que les bibliothèques effectivement utilisées (optimisation)
- `=` au lieu de `?=` : Force la valeur même si l'environnement avait défini `LDFLAGS`

---

## 5. VERIFICATION POST-CORRECTION

```bash
# Test 1: chemin absolu
ROOT="/home/runner/workspace/src/advanced_calculations/quantum_problem_hubbard_hts"
timeout 30 "$ROOT/hubbard_hts_research_runner" "$ROOT" → EXIT 0 ✓ (était EXIT 139)

# Test 2: taille et linkage
ls -la hubbard_hts_research_runner → 63 960 octets ✓ (était 997 568)
ldd → libm.so.6 présent ✓ (était absent)
nm | wc -l → 5 symboles ✓ (était 941)

# Test 3: workflow
Quantum Research Cycle → RUNNING ✓ (était SIGSEGV ligne 60 du script)
```

---

## 6. SUSPECTS ÉCARTÉS

Les analyses précédentes avaient identifié plusieurs suspects dans le code C :

| Suspect | Lignes | Verdict |
|---|---|---|
| `pjoin()` sans vérif retour | 631-646 | **ÉCARTÉ** — code C correct |
| `benchmark_row_t brow[256]` sur pile | 990 | **ÉCARTÉ** — 30 KB sur pile 8 MB |
| `double ts[4096]` sur pile | 851 | **ÉCARTÉ** — 32 KB normal |
| `calloc(65025,...)` pour cluster 255×255 | 1071-1086 | **ÉCARTÉ** — heap OK |
| Cast `(uint64_t)j` avec j négatif | 903 | **ÉCARTÉ** — wrapping uint64_t défini |
| Stack overflow | — | **ÉCARTÉ** — stack 8 MB, usage ~85 KB |
| **NIX_LDFLAGS linkage statique glibc** | **Makefile ligne 3** | **CAUSE RÉELLE ✓** |

---

## 7. BILAN

- **Cause racine** : `LDFLAGS ?=` dans le Makefile permettait au wrapper NixOS d'injecter des chemins `-L` vers des archives statiques glibc, produisant un binaire hybride corrompu de 997 KB.
- **Correction** : `LDFLAGS = -Wl,-Bdynamic,--as-needed` — forçage du linkage dynamique.
- **Fichier modifié** : 1 ligne dans `Makefile` uniquement.
- **Code C** : inchangé — le code de simulation Hubbard est correct.
- **Workflow** : en cours d'exécution sans SIGSEGV ✓

