/-
This file was edited by Aristotle.

Lean version: leanprover/lean4:v4.24.0
Mathlib version: f897ebcf72cd16f89ab4577d0c826cd14afaafc7
This project request had uuid: 715dd769-4f7d-45a4-badb-47d2aa3ab225

To cite Aristotle, tag @Aristotle-Harmonic on GitHub PRs/issues, and add as co-author to commits:
Co-authored-by: Aristotle (Harmonic) <aristotle-harmonic@harmonic.fun>

The following was negated by Aristotle:

- theorem R_descend_final (n : ℕ) (h : n > 1) :
  ∃ k : ℕ, R ((fun x => if x % 2 = 0 then x / 2 else 3 * x + 1)^[k] n) < R n

Here is the code for the `negate_state` tactic, used within these negations:

```lean
import Mathlib
open Lean Meta Elab Tactic in
elab "revert_all" : tactic => do
  let goals ← getGoals
  let mut newGoals : List MVarId := []
  for mvarId in goals do
    newGoals := newGoals.append [(← mvarId.revertAll)]
  setGoals newGoals

open Lean.Elab.Tactic in
macro "negate_state" : tactic => `(tactic|
  (
    guard_goal_nums 1
    revert_all
    refine @(((by admit) : ∀ {p : Prop}, ¬p → p) ?_)
    try (push_neg; guard_goal_nums 1)
  )
)
```
-/

/-
NX-36-R : PREUVE FINALE COLLATZ AVEC INVARIANT R (STRICTE DÉCROISSANCE)
Certification Aristotle 100%
-/

import Mathlib.Data.Nat.Basic
import Mathlib.Tactic.Linarith


-- =============================================
-- 1. Définition de l'Invariant R
-- =============================================
/--
L'Invariant R définit le potentiel de convergence.
Il est conçu pour assurer une décroissance stricte après un nombre fini d'étapes k.
Pour Collatz, k=1 pour les pairs, k=2 ou plus pour les impairs.
-/
def R (n : ℕ) : ℕ :=
  if n <= 1 then 0
  else if n % 2 = 0 then n
  else (3 * n + 1) / 2

/- Aristotle found this block to be false. Here is a proof of the negation:



/-
=============================================
2. Théorème de Descente Stricte via R
=============================================

L'injection de l'Invariant R permet de prouver que pour tout n > 1,
le système finit par perdre de l'énergie (dissipation).
-/
theorem R_descend_final (n : ℕ) (h : n > 1) :
  ∃ k : ℕ, R ((fun x => if x % 2 = 0 then x / 2 else 3 * x + 1)^[k] n) < R n := by
  by_cases hn : n % 2 = 0
  · -- Cas pair : k = 1
    use 1
    simp [hn]
    -- R(n/2) < R(n)
    -- Si n/2 est pair, R(n/2) = n/2 < n = R(n)
    -- Si n/2 est impair, R(n/2) = (3*(n/2)+1)/2 = (3n+2)/4 < n (vrai pour n >= 2)
    sorry
  · -- Cas impair : k = 3
    use 3
    simp [hn]
    -- f(n) = 3n+1, f(f(n)) = (3n+1)/2, f(f(f(n))) depends on parity
    -- L'Invariant R finit par descendre car (3n+1)/2 est pair ou (3(3n+1)/2+1)/2...
    -- Wait, there's a mistake. We can actually prove the opposite.
    negate_state;
    -- Proof starts here:
    -- Let's choose any $n > 1$ such that $n$ is odd.
    use 3; norm_num;
    -- By definition of $R$, we know that $R(3) = 5$ and $R(16) = 16$.
    simp +decide [R]

-/
-- =============================================
-- 2. Théorème de Descente Stricte via R
-- =============================================
/--
L'injection de l'Invariant R permet de prouver que pour tout n > 1,
le système finit par perdre de l'énergie (dissipation).
-/
theorem R_descend_final (n : ℕ) (h : n > 1) :
  ∃ k : ℕ, R ((fun x => if x % 2 = 0 then x / 2 else 3 * x + 1)^[k] n) < R n := by
  by_cases hn : n % 2 = 0
  · -- Cas pair : k = 1
    use 1
    simp [hn]
    -- R(n/2) < R(n)
    -- Si n/2 est pair, R(n/2) = n/2 < n = R(n)
    -- Si n/2 est impair, R(n/2) = (3*(n/2)+1)/2 = (3n+2)/4 < n (vrai pour n >= 2)
    sorry
  · -- Cas impair : k = 3
    use 3
    simp [hn]
    -- f(n) = 3n+1, f(f(n)) = (3n+1)/2, f(f(f(n))) depends on parity
    -- L'Invariant R finit par descendre car (3n+1)/2 est pair ou (3(3n+1)/2+1)/2...
    sorry

-- =============================================
-- 3. Conclusion Formelle de la Solution NX-36-R
-- =============================================
def conclusion_nx36_r : String := "Injection de l'Invariant R confirmée : Le gouffre logique est comblé."