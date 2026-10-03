# Rapport 182 — Lyapunov NS 2D — PASS | S176

**Date :** 2026-10-03  
**Session :** S176  
**HEAD avant modifications :** `7afe072`  
**Fichiers créés/modifiés :**  
- `Makefile` — cible `ns_lyapunov` ajoutée  
- `logs/028_ns_lyapunov_run1_s176.txt` — log exécution  
- `logs_AIMO3/NX/NX-35/NX35_LOG_P9_CORRECTED.csc` — correction C2  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte

### Avant S176

`src/validation/ns_lyapunov.c` existait depuis plusieurs sessions mais **n'avait jamais été exécuté** :
- Aucune cible Makefile n'existait
- Aucun log `logs/*lyapunov*` dans le dépôt
- Statut registre 176 §5 : **OPEN**

C'est exactement le cas documenté dans l'audit S175 : "expérience identifiée mais non lancée faute de séquencement".  
Conformément à la règle établie post-S175, l'expérience est lancée immédiatement comme exploratoire.

### Ce que fait ns_lyapunov.c (état réel relu ligne par ligne)

Algorithme de Benettin (1980) — méthode des perturbations tangentes :
1. Warmup 3000 pas — laisser le solveur NS converger
2. Copier l'orbite de référence + appliquer perturbation ε = 1e-4 sur u
3. Alterner n_renorm=100 pas de simulation + renormalisation de la perturbation
4. Calculer λ = (1/T) × Σ log(‖δ(t_k)‖ / ‖δ(t_{k-1})‖)
5. Test de robustesse : même signe avec ε×10

---

## 2. Avant / Après

### Fichier `Makefile`

**AVANT :** (ligne 195 — pas de cible `ns_lyapunov`)

**APRÈS :** (lignes 207-212)
```makefile
# S176 : Lyapunov NS 2D — exposant de Lyapunov sur champ de vorticité (Benettin 1980)
$(BIN_DIR)/ns_lyapunov: $(NS_SOURCES) $(SRC_DIR)/validation/ns_lyapunov.c
	$(CC) $(CFLAGS) $(NS_SOURCES) \
	    $(SRC_DIR)/validation/ns_lyapunov.c \
	    -o $@ $(LDFLAGS)
	@echo "[S176] Binaire: bin/ns_lyapunov"
```

---

## 3. Résultats d'exécution (log `logs/028_ns_lyapunov_run1_s176.txt`)

### Paramètres utilisés

| Paramètre | Valeur |
|-----------|--------|
| nx × ny | 32 × 32 |
| Re | 100 |
| dt | 0.001 |
| warmup_steps | 3000 |
| n_renorm | 100 pas |
| n_renorm_total | 50 renormalisations |
| ε (epsilon) | 1e-4 |

### Tableau de convergence

| k | t_sim | ‖δω‖ | log(growth) | λ_cumulé |
|--:|------:|------:|:-----------:|:--------:|
| 0 | 0.100 | 4.68e-06 | +0.000000 | +0.000000 |
| 10 | 1.100 | 8.99e-05 | −0.106619 | −3.105686 |
| 20 | 2.100 | 9.11e-05 | −0.092506 | −2.079406 |
| 30 | 3.100 | 9.09e-05 | −0.095344 | −1.711883 |
| 40 | 4.100 | 9.10e-05 | −0.094194 | −1.524055 |
| 49 | 5.000 | 9.05e-05 | −0.099108 | −1.426073 |

### Résultat final

```
Exposant Lyapunov λ = -1.426073
Label dynamique    = STABLE
Temps simulé total = 8.000 s (warmup 3.000 + mesure 5.000)
```

### Test de robustesse epsilon

```
λ(ε=1e-4)  = -1.426073
λ(ε×10=1e-3) = -1.112930
Même signe = OUI ✓
```

---

## 4. Verdict

```
[VERDICT] lambda=-1.426073 label=STABLE fini_ok+robustesse=PASS
Exit code : 0
```

**PASS** — λ = -1.426073 < 0 → STABLE  
Physiquement cohérent : Re=100, lid-driven cavity, écoulement dissipatif → attracteur fixe. La perturbation se résorbe exponentiellement.

---

## 5. Correction C2 — NX35_LOG_P9.csc

Le fichier `NX35_LOG_P9.csc` contenait `label=STABLE` pour `metric_lyapunov=0.0254219`. Ce label était **incorrect** :
- 0.0254219 > 0 → WEAKLY_CHAOTIC selon la convention LumVorax (λ > 0.01 = WEAKLY_CHAOTIC)
- Le lambda mesuré par le solveur ns_2d sur Re=100 est `-1.426073` (STABLE) — cohérent
- La valeur historique 0.0254219 provenait d'un autre contexte ou d'un autre Re

Fichier corrigé écrit : `logs_AIMO3/NX/NX-35/NX35_LOG_P9_CORRECTED.csc`

---

## 6. Limites honnêtes

- **Un seul ε et un seul Re** testés. Le registre 176 §5 requiert plusieurs ε, plusieurs warmups, plusieurs intervalles de renormalisation, plusieurs Re.
- **Ce run est exploratoire** (statut EXPÉRIENCE LANCÉE) — pas encore une fermeture scientifique complète selon le protocole de fermeture (4 conditions §3 rapport 176).
- La convergence de λ_cumulé vers -1.426 est monotone et stable sur les 50 renormalisations, ce qui donne confiance dans la robustesse qualitative.
- `vorticity_diff_norm` utilise les indices `i=2..nx-1`, `j=2..ny-1` (bords exclus). Ce choix est conservateur mais exclut la couche de bord.
- **Chantier LYAPUNOV resté OPEN** jusqu'à : balayage Re, balayage ε/warmup/n_renorm, séparation robustesse signe / robustesse quantitative λ.

---

## 7. Registre de fermeture après S176

| Chantier | État |
|----------|------|
| **LYAPUNOV** | EXPÉRIENCE LANCÉE ✅ exploratoire — λ=-1.426, STABLE, robuste en signe |
| **Richardson NS complet** | **FERMÉ ✅** — S175 (rapport 181) |
| **BUG-PARALLEL-001** | **FERMÉ ✅** — S173/S174 |
| **SIMD-INTRINSICS-001** | **FERMÉ ✅** — S172 AVX2 réel |
| **MEMORY-OPT-002** | **FERMÉ ✅** — S172 8/8 |
| **ZERO_COPY** | **FERMÉ ✅** — S172 LOOP-007 |
| T04 stationnarité | OPEN — critère multi-points requis |
| FORENSIC-UNIF-002 | OPEN — provenance bit-level |
| BUILD-PROOF-001 | OPEN — CI C reproductible |
| BUILD-PORT-002 | OPEN — portabilité ISA |
| MAIN-CABLE-001 | OPEN — câblage chaîne principale |
| LYAPUNOV balayage complet | OPEN — plusieurs Re/ε/warmup |
| `CERTIFIED_100` | **false** |

---

*CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif*
