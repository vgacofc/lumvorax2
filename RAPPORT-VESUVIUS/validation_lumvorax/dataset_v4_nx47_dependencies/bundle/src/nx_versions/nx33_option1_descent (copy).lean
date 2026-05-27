import Mathlib.Data.Nat.Basic
import Mathlib.Tactic
import Mathlib.Data.Nat.Iterate

def collatz_step (n : ℕ) : ℕ :=
  if n % 2 = 0 then n / 2 else 3 * n + 1

theorem collatz_even_lt (n : ℕ) (h : n % 2 = 0) (hn : n > 1) :
  collatz_step n < n := by
  simp [collatz_step, h]
  have : n / 2 < n := by
    have hn' : n ≥ 2 := Nat.succ_le_iff.mpr hn
    exact Nat.div_lt_self hn' (by norm_num)
  exact this

theorem collatz_odd_two_steps_lt (n : ℕ) (h : n % 2 = 1) (hn : n ≥ 5) :
  Nat.iterate collatz_step 2 n < n := by
  -- étape 1 : n impair → 3n+1 pair
  have h1 : collatz_step n = 3*n + 1 := by
    simp [collatz_step, h, Nat.mod_

    te proposer l’énoncé Lean minimal vrai (100% prouvable)

    te donner la version publication-proof exacte

    rédiger la réponse officielle aux reviewers expliquant pourquoi la dissipation n’est pas monotone
eq_of_lt (by decide)]
  -- étape 2 : division par 2
  have h2 : Nat.iterate collatz_step 2 n = (3*n + 1) / 2 := by
    simp [Nat.iterate, h1, collatz_step]
  -- inégalité clé : (3n+1)/2 < n pour n ≥ 5
  have : (3*n + 1)/2 < n := by
    have : 3*n + 1 < 2*n := by
      linarith
    exact Nat.div_lt_of_lt_mul (by norm_num) this
  simpa [h2]

theorem collatz_local_descent (n : ℕ) (hn : n > 1) :
  ∃ k : ℕ, k ≤ 2 ∧ Nat.iterate collatz_step k n < n := by
  by_cases hpar : n % 2 = 0
  · refine ⟨1, by decide, ?_⟩
    simpa [Nat.iterate] using collatz_even_lt n hpar hn
  · have hodd : n % 2 = 1 := by
      exact Nat.mod_two_ne_zero.mp hpar
    by_cases hsmall : n < 5
    · -- vérification finie des cas {2,3,4}
      decide
    · refine ⟨2, by decide, ?_⟩
      exact collatz_odd_two_steps_lt n hodd (Nat.le_of_not_lt hsmall)
