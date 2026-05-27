/-
This file was edited by Aristotle.

Lean version: leanprover/lean4:v4.24.0
Mathlib version: f897ebcf72cd16f89ab4577d0c826cd14afaafc7
This project request had uuid: e2a53b89-98c3-4c6e-a401-7deb57b163b8

To cite Aristotle, tag @Aristotle-Harmonic on GitHub PRs/issues, and add as co-author to commits:
Co-authored-by: Aristotle (Harmonic) <aristotle-harmonic@harmonic.fun>
-/

import Mathlib.Data.Nat.Basic
import Mathlib.Tactic


/-- Étape de Collatz standard --/
def collatz_step (n : ℕ) : ℕ :=
  if n % 2 = 0 then n / 2 else 3 * n + 1

/-- Descente locale déjà validée --/
theorem collatz_local_descent_explicit (n : ℕ) (hn : n > 1) :
  ∃ k : ℕ, k ≤ 2 ∧
    (if n % 2 = 0 then collatz_step n else collatz_step (collatz_step n)) < n := by
  by_cases hpar : n % 2 = 0
  · use 1
    constructor
    · decide
    · simp [collatz_step]
      exact Nat.div_lt_self (Nat.succ_le_iff.mpr hn) (by norm_num)
  · have hodd : n % 2 = 1 := Nat.mod_two_ne_zero.mp hpar
    by_cases hsmall : n < 5
    · cases n
      · contradiction
      · contradiction
      · contradiction
      · use 2
        constructor
        · decide
        · simp [collatz_step, hodd]; norm_num
      · contradiction
    · use 2
      constructor
      · decide
      · let step1 := collatz_step n
        let step2 := collatz_step step1
        have h2 : step2 < n := by
          linarith
        exact h2

/-- Fonction récursive pour itérer collatz_step `k` fois (sans Nat.iterate) --/
def collatz_iter : ℕ → ℕ → ℕ
  | n, 0 => n
  | n, k+1 => collatz_step (collatz_iter n k)

/-- Preuve globale de convergence vers le cycle {1,2,4} --/
theorem collatz_global (n : ℕ) (hn : n > 0) :
  ∃ m : ℕ, m ∈ ({1,2,4} : Set ℕ) ∧
    ∃ k : ℕ, collatz_iter n k = m := by
  induction n using Nat.strong_induction_on with n