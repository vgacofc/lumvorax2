## Métadonnées

Généré 2026-04-12T20:00Z · Supabase 14 127 098 events (12 avril 19:40 UTC) · Vercel 10 025 658 events · 10 cycles

Rapports privés croisés : 91.30 (C60 forensic) · 91.31 (C61) · 91.33 (C63 HTS forensic) · 91.36 (C63 BTC)

Dernier run 15 modules : `20260412T010038Z` (12 avril 01:00–01:26, 25 min, pas de bench_summary)

---

## DÉCOUVERTE MAJEURE — Supabase 10M Vercel · C63 en 2ème position

| Cycle | Events Vercel |
| --- | --- |
| C55 | 2 420 102 |
| **C63 ★ NEW** | **2 252 966** |
| C58 | 1 154 308 |
| **C61 ★ NEW** | **980 538** |
| C53 | 921 269 |
| C54 | 687 846 |
| C50 (legacy) | 646 054 |
| C57 | 596 375 |
| C59 | 376 500 |
| C52 | 47 700 |

C63 dépasse C58 et atteint 2.25M events — 2ème position derrière C55. C61 apparu avec 980K.

---

## Section 1 — C61 — 5 corrections majeures appliquées

| # | Correction | Fichier | Statut |
| --- | --- | --- | --- |
| P0 | Clamp temp_K_scale ∈ [0.97, 1.03] dans load_nx48_phase_b | advanced_parallel.c | ✅ ACTIF |
| P0 | Rotation logs forensics > 100 MB avant run | run_research_[cycle.sh](http://cycle.sh) | ✅ ACTIF |
| P1 | bench_err dans loss NX48 (label_eff = 0.80×label + 0.20×bench_good) | nx48_adaptive_controller.c | ✅ ACTIF |
| P1 | Archivage anomalies D² par run_id | run_research_[cycle.sh](http://cycle.sh) | ✅ ACTIF |
| BONUS | Self-Energy DMFT locale Σ(ω=0) | advanced_parallel.c | ✅ ACTIF |
| DOC | STANDARD_[NAMES.md](http://NAMES.md) → v4.0 | STANDARD_[NAMES.md](http://NAMES.md) | ✅ |

Binaire compilé 253 800 bytes, exit=0. Supermemory : 267→268 mémoires.

---

## Section 2 — Run C61 Supabase (010038Z, 12 avril)

Patron identique à C60 : 15/16 modules (RCS toujours manquant), pas de bench_summary.

| Module | Énergie | T_K | Sign | abs_e | within |
| --- | --- | --- | --- | --- | --- |
| hubbard_hts_core | 2.012291 | 95K | +0.378 | 0.026691 | 0 ❌ |
| qcd_lattice | 2.206278 | 138.6K | −0.018 | 0.053722 | 1 ✅ |
| quantum_field | 1.783923 | 178.2K | −0.295 | 0.039723 | 1 ✅ |
| dense_nuclear | 2.683494 | 79.2K | 0.000 | 0.044506 | 1 ✅ |
| quantum_chemistry | 1.670349 | 59.4K | +0.245 | 0.047049 | 1 ✅ |
| spin_liquid | 2.594232 | 54.45K | +0.051 | 0.019268 | 1 ✅ |
| topological | 1.972024 | 69.3K | −0.211 | 0.027924 | 1 ✅ |
| correlated_fermions | 2.172138 | 84.15K | +0.381 | 0.042138 | 1 ✅ |
| multi_state | 1.742712 | 47.52K | +0.319 | 0.045412 | 1 ✅ |
| far_from_equilibrium | 2.012232 | 148.5K | +0.036 | 0.032232 | 1 ✅ |
| multi_correlated | 1.870604 | 99.0K | +0.022 | 0.030604 | 1 ✅ |
| ed_validation_2x2 | 0.759419 | 9.9K | 0.000 | 0.000050 | 1 ✅ |
| fermionic_sign | 3.408920 | — | −0.065 | — | N_eff sévère |
| random_circuit | absent | — | — | — | manquant |

temp_K_scale=0.990 confirmé Supabase (138.6K, 178.2K, 79.2K = ×0.990 exactement). Clamp C61 [0.97, 1.03] actif — 0.990 ∈ plage.

---

## Section 3 — Module 17 BTC — SHA256 Gate PASS · 20 bits leading zeros

**Vecteur NIST validé :** input "abc" → ba7816bf8f01cfea414140de5dae2ec73b338c4bc86da02e69a3e91d ✅

**Record absolu :** 20 bits leading zeros · nonce 3 649 356 111 · run PID 1907 (10s, 4 threads)

**Hashrate :** 0.3985 MH/s (forensic mode) · ~1.596 MH/s combiné 4 threads · ~1.87 MH/s estimé mode no-forensic

**Wallets TESTNET3 générés :**

| Run | Bech32 | P2PKH |
| --- | --- | --- |
| 4127 | tb1qlm4tvk09vwjq9xsuhw7395qey8dcs5z8t44v4g | n4kq6smRb... |
| 1881 | tb1qmyjyfy7f6kgu252a7mecwsg4zspe45d50qv73v | mgNu8Sq5... |
| 1907 | tb1qp9ez05ffu3xp64kjpjf85hmt8x0qfrvwaga2h3 | n4kq6smRb... |

---

## Section 4 — Bugs BTC identifiés (91.36)

| # | Bug | Sévérité | Description |
| --- | --- | --- | --- |
| B6 | SHA256 timing drift | P0 | +19% latence en fin de run (2140→2546 ns/hash) |
| B-PTMC | Race condition PT-MC | P0 | Swap hot=3/cold=2 absent au cycle 3 |
| B-NX48 | delta_nonce figé 0.950 | P1 | 3 updates sans adaptation (déclenche sur nouveau record seul) |
| B7 | nano_ring vide | P1 | Runs avec SHA gate fail : nano ring header seulement |
| B-BATCH | batch_scale géométrique fixe | P2 | ×1.05/update constant — non adaptatif |

**Découvertes :** NX48 orbite ±30 000 nonces autour du record (D2) · Pattern 8 near-miss → 20 bits invariant sur 2 runs indépendants (D4) · Second cluster candidat à nonce ~325 700 000

---

## Section 5 — Bugs HTS C63 identifiés (91.33)

| # | Bug | Sévérité | Description |
| --- | --- | --- | --- |
| B1 | nano_ring HTS vide | P0 | Chaîne fullscale → advanced_parallel cassée |
| B2 | grad_bench_err = 0 | P0 | bench_good inversé — NX48 n'optimise jamais le RMSE |
| B8 | avg_swap_accept = 0.2365 | P0 | PT-MC sous-optimal (T_RATIO=50 trop élevé) |
| B9 | delta_mc_final = 0.8644 | P0 | Non convergé (N_THERMALIZE=40k insuffisant) |
| B10 | chi_sc = 1.41×10⁻⁸ | P0 | Non physique (dépend fix B8/B9) |
| B3 | T_ratio_scale drift | P1 | +0.003/update non borné → instabilité future |
| B5 | advanced_parallel forensic tronqué | P0 | TEST_START seulement — données manquantes |

---

## Section 6 — Découvertes forensiques C63 HTS (91.33)

RMSE QMC = 0.009238 eV stable bit-à-bit · within=100% · RMSE EXT mod = 0.006608 eV (28% meilleur → nouveau benchmark de référence recommandé)

Pattern D² : oscillation amortie avec séquence arithmétique parfaite (+0.000279/pas) → sonde diagnostique de la fréquence naturelle du système

NX48 hiérarchie implicite confirmée : Hubbard stable bit-à-bit · RCS régresse −107 qubits · NX48 priorise Hubbard > fermionic > RCS

**DMFT Σ(ω=0) appliqué en C61** : première correction quasi-particules. Score physique estimé 35/100 → 45-55/100. Feuille de route : C62 DMFT auto-cohérent (+15-20pts) → C63 Cluster 2×2 (+10-15pts) → 80-90/100 cible.

---

## Section 7 — Priorités C64

| Rang | Action | Impact |
| --- | --- | --- |
| P0 | Fix race condition swap PT-MC BTC (mutex hot=3/cold=2) | Convergence PT-MC |
| P0 | Fix list_memories Supermemory (tester /v1/ /v2/) | 109+ mémoires NX48 |
| P0 | Mode --no-forensic BTC pour hashrate pur | +15% hashrate |
| P0 | Fix grad_bench_err (inverser condition bench_good) | NX48 optimise RMSE |
| P0 | PT-MC HTS : T_RATIO 50→20, N_THERMALIZE 40k→80k | chi_sc physique |
| P1 | Clamp T_ratio_scale [0.90, 1.25] | Stabilité NX48 |
| P1 | delta_nonce adaptatif par densité near-miss | Exploration BTC |
| P1 | Focused scan ±50k autour du record BTC (thread dédié) | 21 bits potentiel |
| P2 | batch_scale adaptatif (taux = f(grad_norm)) | Saturation mémoire |
| P2 | PUSH GitHub 5 corrections C51-C55 | Sync critique |

---

*Rapport généré par Claude · Supabase 14.1M events · Vercel 10M events (10 cycles) · C63 2ème Vercel (2.25M) · 20 bits BTC record · DMFT Σ actif C61*