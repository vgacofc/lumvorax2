# Rapport 192 — P1 Richardson-PROTOCOL-003 : PASS T01=1.546 T02=1.543 (S182)

**Date :** 2026-10-04T01:05:52Z  
**Session :** S182  
**SHA Git HEAD :** (avant commit)  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte

Suite du rapport 191 (diagnostic de la cause racine de `set_couette_bc()`) et du registre OPEN P1. Deux corrections appliquées dans ce rapport :

1. **`set_couette_bc()`** : remplace l'appel à `ns_solver_set_lid_bc()` (parois solides O/E) par des CL Neumann (∂u/∂x=0) — supprime la cavité à 4 parois.  
2. **`run_to_steady()`** : remplace `ns_solver_step(s)` par `ns_solver_step_with_bc(s, set_couette_bc)` — la CL Couette est appliquée à chaque pas, pas seulement à l'initialisation.  
3. **`init_couette_profile()`** : initialise le champ `u` avec le profil linéaire exact `u=y` avant la boucle — convergence rapide (~5000 pas).

---

## 2. AVANT / APRÈS

### AVANT (`src/validation/ns_richardson_manufactured.c`, HEAD 8aaabe5)

```c
// Ligne 176-181 (AVANT)
static void set_couette_bc(NSSolver2D *s)
{
    /* utilise Lid-Driven → parois solides O/E → problème = cavité, pas Couette */
    ns_solver_set_lid_bc(s);
}

// Dans run_to_steady() ligne 390 (AVANT)
poisson_res = ns_solver_step(s);  /* CL hard-codée Lid-Driven écrase tout */
```

**Résultat AVANT :** L2 ≈ 0.57 pour toutes les grilles — erreur structurelle irréductible.

### APRÈS (ce commit)

```c
// set_couette_bc() — CL Neumann en x, no-slip N/S
static void set_couette_bc(NSSolver2D *s) {
    int nx = s->params.nx; int ny = s->params.ny;
    // Bords Sud/Nord : no-slip / couvercle (identique Lid-Driven)
    for (int i = 0; i <= nx; i++) {
        s->u[i * (ny+2) + 0]      = -s->u[i * (ny+2) + 1];
        s->u[i * (ny+2) + ny + 1] = 2.0 - s->u[i * (ny+2) + ny];
    }
    // Bords O/E : Neumann (∂u/∂x=0) — supprime les parois solides
    for (int j = 0; j <= ny + 1; j++) {
        s->u[0  * (ny+2) + j] = s->u[1       * (ny+2) + j];
        s->u[nx * (ny+2) + j] = s->u[(nx-1)  * (ny+2) + j];
    }
    // v=0, p Neumann…
}

// Dans run_to_steady() (APRÈS)
poisson_res = ns_solver_step_with_bc(s, set_couette_bc);  /* CL Couette à chaque pas */
```

---

## 3. Résultats d'exécution (HEAD post-correction)

### Protocole A (dt=const, diagnostique)

| Grille | Statut | Steps | L1 | L2 | Linf | Wall |
|--------|--------|-------|-----|-----|------|------|
| 32×32  | CONV   | 7950  | 0.004150 | 0.005177 | 0.013467 | 0.9 s |
| 64×64  | CONV   | 5550  | 0.001852 | 0.002401 | 0.007204 | 3.7 s |

### Protocole B (dt∝dx)

| Grille | Statut | Steps | L1 | L2 | Linf | Wall |
|--------|--------|-------|-----|-----|------|------|
| 32×32  | CONV   | 7950  | 0.004150 | 0.005177 | 0.013467 | 0.7 s |
| 64×64  | CONV   | 5950  | 0.001471 | 0.002066 | 0.007039 | 2.9 s |
| 128×128| CONV   | 5200  | 0.000529 | 0.000846 | 0.003633 | 13.5 s |

### Protocole C (dt∝dx², référence T01–T04)

| Grille | Statut | Steps | L1 | L2 | Linf | Wall |
|--------|--------|-------|-----|-----|------|------|
| 32×32  | CONV   | 7950  | 0.004150 | 0.005177 | 0.013467 | 1.0 s |
| 64×64  | CONV   | 6500  | 0.001152 | 0.001773 | 0.006816 | 3.5 s |
| 128×128| CONV   | 5700  | 0.000297 | 0.000608 | 0.003413 | 13.5 s |

### Ordre de convergence (protocole C)

| Raffinement | L2 coarse | L2 fine | Ordre observé |
|-------------|-----------|---------|---------------|
| 32→64       | 0.005177  | 0.001773 | **1.546** |
| 64→128      | 0.001773  | 0.000608 | **1.543** |

---

## 4. Tests T01–T04

| Test | Critère | Résultat | Verdict |
|------|---------|----------|---------|
| T01 | Ordre spatial 32→64 ≥ 1.5 | **1.546** | ✅ PASS |
| T02 | Ordre spatial 64→128 ≥ 1.5 | **1.543** | ✅ PASS |
| T03 | L2 strictement décroissant (protocole C) | 0.005177 > 0.001773 > 0.000608 | ✅ PASS |
| T04 | Linf_128 < 0.05 | **0.003413** | ✅ PASS |

**[VERDICT] RICHARDSON-PROTOCOL-003 : PASS — 4/4 tests T01→T04 — ordre spatial ≈ 1.54 sur solution exacte Couette.**

---

## 5. Log forensic (artefact)

**Fichier :** `logs/forensic/ns_richardson_manufactured.log`  
**Total events :** 8 (1 SESSION_START + 3×3=9 LUM checkpoints + 1 SESSION_END... total 8 observés)

```
seq=0: PROTO003:n=32:proto=0:L2=0.005177:conv=1
seq=1: PROTO003:n=64:proto=0:L2=0.002401:conv=1
seq=2: PROTO003:n=32:proto=1:L2=0.005177:conv=1
seq=3: PROTO003:n=64:proto=1:L2=0.002066:conv=1
seq=4: PROTO003:n=128:proto=1:L2=0.000846:conv=1
seq=5: PROTO003:n=32:proto=2:L2=0.005177:conv=1
seq=6: PROTO003:n=64:proto=2:L2=0.001773:conv=1
seq=7: PROTO003:n=128:proto=2:L2=0.000608:conv=1
```

Tous `conv=1` — aucune grille n'a atteint la limite de sécurité.

---

## 6. Limites honnêtes post-PASS

| Limite | Statut |
|--------|--------|
| Solution Couette = stationnaire : `u·∇u=0` → ordre mesuré = ordre **diffusion** uniquement | ⚠️ Documenté |
| Advection non-linéaire non validée (MMS avec terme source requis) | ⚠️ Ouverte |
| Grille 256×256 non testée | ⚠️ Non évalué |
| Protocole A : grille 128×128 exclue (dt=1e-4 → ~120s wall) | ℹ️ Diagnostique tronqué |
| Bords O/E Neumann ≠ périodiques vrais (légère asymétrie possible) | ⚠️ Acceptable pour Couette plan |
| CERTIFIED_100=false | Invariant maintenu |

---

## 7. Registre OPEN (mis à jour S182)

| Priorité | Chantier | État |
|----------|----------|------|
| ~~P0~~ | ~~FU002 provenance bit-level~~ | ✅ CLOSED (rapport 190) |
| ~~P1~~ | ~~Richardson-PROTOCOL-003~~ | ✅ **CLOSED — T01=1.546 T02=1.543 PASS (rapport 192)** |
| **P2** | Ordre temporel Chorin complet (advection+diffusion+Poisson+projection+BCs) | OPEN |
| P3 | T04 renforcé fenêtre finale | OPEN |
| P4 | Lyapunov robuste | OPEN |
| P5–P10 | … | OPEN |

---

*Rapport produit sur exécution directe du binaire post-correction. Résultats copiés depuis stdout. Log forensic vérifié (8 événements, tous conv=1). CERTIFIED_100=false.*
