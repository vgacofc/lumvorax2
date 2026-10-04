# Rapport 191 — P1 Richardson-PROTOCOL-003 : diagnostic racine + correction set_couette_bc()

**Date :** 2026-10-04T01:05:52Z  
**Session :** S182  
**SHA Git HEAD :** 8aaabe5  
**CERTIFIED_100=false | unique_human_proven=false | Mode DEBUG actif**

---

## 1. Contexte

P1 du registre OPEN (rapport 189/190) : vérifier si Richardson-PROTOCOL-003 peut PASS avec la solution analytique Couette.  
Binaire `bin/ns_richardson_manufactured` exécuté. Résultats proto A et proto B partiels obtenus.

---

## 2. Résultats d'exécution (observés sur HEAD 8aaabe5)

### 2.1 Protocole A (dt=const, diagnostique)

| Grille | Statut | Steps | t_phys | L1 | L2 | Linf | Wall |
|--------|--------|-------|--------|-----|-----|------|------|
| 32×32  | LIMIT  | 40000 | 4.00 s | 0.515625 | 0.579399 | 1.012032 | 7.1 s |
| 64×64  | LIMIT  | 40000 | 4.00 s | 0.507813 | 0.573427 | 1.031294 | 31.7 s |
| 128×128 | (exclue) | — | — | — | — | — | — |

### 2.2 Protocole B (dt∝dx, partiel — interrompu à 128×128)

| Grille | Statut | Steps | t_phys | L1 | L2 | Linf | Wall |
|--------|--------|-------|--------|-----|-----|------|------|
| 32×32  | CONV   | 96600  | 9.66 s | 0.515625 | 0.576743 | 1.012439 | 12.2 s |
| 64×64  | CONV   | 164200 | 8.21 s | 0.507813 | 0.570930 | 1.031616 | 86.5 s |
| 128×128 | (en cours, interrompu) | — | — | — | — | — | — |

---

## 3. Diagnostic de la cause racine — AVANT (état actuel)

### 3.1 L2 ≈ 0.57 indépendamment de la grille

**Observation :** L2 = 0.579 (32×32), 0.573 (64×64 proto A) — **quasi-identiques malgré le raffinement**. Cela signifie que la solution numérique ne converge **pas** vers `u_exact = y`. L'erreur est structurelle, pas de discrétisation.

**Linf > 1.0** confirme des zones où `|u_num - u_exact| > 1` — impossible pour Couette si la solution convergeait.

### 3.2 Cause racine identifiée : `set_couette_bc()` est une copie de Lid-Driven

**Fichier :** `src/validation/ns_richardson_manufactured.c` lignes 176–181  
**AVANT (état actuel) :**

```c
// src/validation/ns_richardson_manufactured.c L176-181
static void set_couette_bc(NSSolver2D *s)
{
    /* ns_solver_set_lid_bc() impose u=1 sur le couvercle Nord et
     * u=v=0 sur les autres parois — identique à Couette plan. */
    ns_solver_set_lid_bc(s);   // ← ERRONÉ
}
```

**Analyse :** `ns_solver_set_lid_bc()` (`ns_solver_2d.c` L106–157) impose :
- `u=0` sur Ouest (`i=0`) et Est (`i=nx`) — **parois solides**
- `u=0` miroir sur Sud, `u=1` miroir sur Nord (couvercle)
- `v=0` sur tous les bords

Cela produit une **cavité à 4 parois** (problème de la cavité Lid-Driven classique), dont la solution est un profil tourbillonnaire 2D. La solution exacte de ce problème est **inconnue analytiquement** (seuls des tableaux Ghia 1982 existent).

**Couette plan ≠ Lid-Driven :**

| Propriété | Lid-Driven | Couette plan |
|-----------|-----------|--------------|
| Bords O/E | `u=0` (parois solides) | Périodiques |
| Solution stationnaire | Tourbillon 2D (Ghia) | `u(y) = y` linéaire |
| `u·∇u` | Non-nul | Nul |
| Solution analytique | Inconnue (17 pts Ghia) | Exacte partout |

**Conséquence :** L2 ≈ 0.57 est la norme L2 de la différence entre le profil tourbillonnaire Lid-Driven et le profil linéaire `y` — valeur structurelle indépendante du raffinement, jamais nulle. T01/T02/T03/T04 ne peuvent pas PASS avec la CL actuelle.

---

## 4. Correction requise (APRÈS)

### 4.1 Approche : CL périodiques en x pour Couette plan

Pour que le solveur produise `u(y) = y`, il faut que les bords O/E n'aient pas d'influence sur la solution — soit en les rendant périodiques, soit en initialisant avec le profil exact et en ne les touchant pas.

**Stratégie minimale** (sans modifier `ns_solver_2d.c`) : initier le champ `u` avec le profil Couette exact, imposer une CL de type "copy" sur O/E (Neumann en x, `∂u/∂x = 0`), et vérifier que la solution reste linéaire.

**Fichier à modifier :** `src/validation/ns_richardson_manufactured.c`

**APRÈS :**

```c
// src/validation/ns_richardson_manufactured.c — set_couette_bc() corrigée
// CL Couette plan : u=0 bas, u=1 haut, Neumann en x (∂u/∂x=0)
static void set_couette_bc_corrected(NSSolver2D *s)
{
    int nx = s->params.nx;
    int ny = s->params.ny;

    /* Bords Sud et Nord pour u (identique à Lid-Driven) */
    for (int i = 0; i <= nx; i++) {
        U(s, i, 0)      = -U(s, i, 1);          /* no-slip Sud : u=0 */
        U(s, i, ny + 1) = 2.0 - U(s, i, ny);    /* couvercle Nord : u=1 */
    }

    /* Bords O/E : Neumann (∂u/∂x = 0) — copie de la colonne voisine */
    for (int j = 0; j <= ny + 1; j++) {
        U(s, 0,  j) = U(s, 1,  j);    /* Neumann Ouest : u_O = u_1 */
        U(s, nx, j) = U(s, nx-1, j);  /* Neumann Est   : u_E = u_{n-1} */
    }

    /* v = 0 partout (Couette : pas de composante transverse) */
    for (int i = 0; i <= nx + 1; i++) {
        V(s, i, 0)  = 0.0;
        V(s, i, ny) = 0.0;
    }
    for (int j = 0; j <= ny; j++) {
        V(s, 0,      j) = 0.0;   /* Neumann : pas de flux transverse */
        V(s, nx + 1, j) = 0.0;
    }

    /* p : Neumann homogène dp/dn=0 */
    for (int j = 0; j <= ny + 1; j++) {
        P(s, 0,      j) = P(s, 1,   j);
        P(s, nx + 1, j) = P(s, nx,  j);
    }
    for (int i = 0; i <= nx + 1; i++) {
        P(s, i, 0)      = P(s, i, 1);
        P(s, i, ny + 1) = P(s, i, ny);
    }
}
```

**Et initialiser le champ `u` avec le profil linéaire avant la boucle :**

```c
// Dans run_to_steady(), après ns_solver_create() et set_couette_bc() :
// Initialiser u avec profil linéaire Couette exact
for (int i = 0; i <= nx; i++) {
    for (int j = 0; j <= ny + 1; j++) {
        double y = j * s->dy;
        s->u[i * (ny + 2) + j] = y;  // u_exact = y
    }
}
```

### 4.2 État de la correction dans ce rapport

**Ce rapport documente le diagnostic et la correction nécessaire. La correction n'est pas commitée dans ce rapport** (chantier ouvert P1). L'objectif de ce rapport est :
1. Prouver que T01–T04 ne peuvent pas PASS avec la CL actuelle (raison démontrée)
2. Spécifier précisément la correction à apporter
3. Documenter avant/après pour audit

---

## 5. Limite honnête supplémentaire identifiée

Même avec la CL Neumann en x, la solution peut ne pas être exactement linéaire pour des grilles grossières car l'advection `u·∇u` n'est pas exactement nulle numériquement pour les CL Neumann (copie) — contrairement aux CL périodiques vraies où `u·∇u = 0` exactement. Ce point doit être évalué expérimentalement après la correction.

Pour une preuve rigoureuse, il faudrait :
- CL périodiques vraies en x (modification de `ns_solver_step()`) — changement plus profond
- ou confirmation expérimentale que les erreurs diminuent avec O(2) après correction

---

## 6. Bilan P1 Richardson-PROTOCOL-003

| Critère | État |
|---------|------|
| Code PROTOCOL-003 existe (`ns_richardson_manufactured.c`) | ✅ |
| Binaire compilé | ✅ |
| Exécution proto A et B partielle | ✅ |
| L2 ≈ 0.57 (non convergence vers Couette) | ❌ Bug confirmé |
| Cause racine : `set_couette_bc()` = Lid-Driven | ✅ Identifiée |
| Correction spécifiée (CL Neumann en x + init profil) | ✅ Documentée |
| Correction commitée | ❌ En attente |

**P1 Richardson-PROTOCOL-003 : OPEN — diagnostic racine posé, correction spécifiée. Implémentation = prochaine action.**

---

## 7. Registre OPEN (mis à jour S182)

| Priorité | Chantier | État |
|----------|----------|------|
| ~~P0~~ | ~~FU002 provenance bit-level~~ | ✅ CLOSED (rapport 190) |
| **P1** | Richardson-PROTOCOL-003 — corriger `set_couette_bc()` + CL Neumann en x + init profil linéaire | **OPEN — correction spécifiée** |
| P2 | Ordre temporel Chorin complet | OPEN |
| P3 | T04 renforcé fenêtre finale | OPEN |
| P4 | Lyapunov robuste | OPEN |
| P5–P10 | … | OPEN |

---

*Diagnostic basé sur lecture source `ns_richardson_manufactured.c` + `ns_solver_2d.c` (HEAD 8aaabe5) + exécution directe du binaire. L2 ≈ 0.57 = preuve structurelle, pas de hasard. CERTIFIED_100=false.*
