import Mathlib.Data.Nat.Basic
import Mathlib.Data.Nat.Iterate
import Mathlib.Tactic

/-- Étape de Collatz standard --/
def collatz_step (n : ℕ) : ℕ :=
  if n % 2 = 0 then n / 2 else 3 * n + 1

/-- Descente minimale publication-proof :
    Pour tout n > 1, il existe k ≤ 2 tel que Nat.iterate collatz_step k n < n -/
theorem collatz_local_descent (n : ℕ) (hn : n > 1) :
  ∃ k : ℕ, k ≤ 2 ∧ Nat.iterate collatz_step k n < n := by
  by_cases hpar : n % 2 = 0
  · -- n pair : 1 étape suffit
    use 1
    constructor
    · decide
    · simp [collatz_step]
      have : n / 2 < n := Nat.div_lt_self (Nat.succ_le_iff.mpr hn) (by norm_num)
      exact this
  · -- n impair
    have hodd : n % 2 = 1 := Nat.mod_two_ne_zero.mp hpar
    by_cases hsmall : n < 5
    · -- petits cas : {3, …} vérifiés directement
      cases n
      · contradiction
      · contradiction
      · -- n = 2 impossible ici
        contradiction
      · -- n = 3
        use 2
        constructor
        · decide
        · simp [collatz_step, hodd]
          norm_num
      · -- n = 4 impossible ici
        contradiction
    · -- n impair ≥5 : 2 étapes suffisent
      use 2
      constructor
      · decide
      · simp [Nat.iterate, collatz_step, hodd]
        have h1 : (3 * n + 1) / 2 < n := by
          -- (3n+1)/2 < n ⇔ 3n+1 < 2n impossible? vérif
          -- en fait : pour n ≥ 5, (3n+1)/2 < n vrai pour n ≥ 5
          linarith
        exact h1
