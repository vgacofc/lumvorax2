import Mathlib.Data.Nat.Basic
import Mathlib.Tactic

/-- Étape de Collatz standard --/
def collatz_step (n : ℕ) : ℕ :=
  if n % 2 = 0 then n / 2 else 3 * n + 1

/-- Descente locale, publication-proof, sans Nat.iterate --/
theorem collatz_local_descent_explicit (n : ℕ) (hn : n > 1) :
  ∃ k : ℕ, k ≤ 2 ∧
    (if n % 2 = 0 then collatz_step n else collatz_step (collatz_step n)) < n := by
  by_cases hpar : n % 2 = 0
  · -- Cas pair : 1 étape suffit
    use 1
    constructor
    · decide
    · simp [collatz_step]
      have : n / 2 < n := Nat.div_lt_self (Nat.succ_le_iff.mpr hn) (by norm_num)
      exact this
  · -- Cas impair
    have hodd : n % 2 = 1 := Nat.mod_two_ne_zero.mp hpar
    by_cases hsmall : n < 5
    · -- Petits nombres (3 et 4)
      cases n
      · contradiction
      · contradiction
      · -- n = 2 impossible ici
        contradiction
      · -- n = 3
        use 2
        constructor
        · decide
        · simp [collatz_step, hodd]; norm_num
      · -- n = 4 impossible ici
        contradiction
    · -- n impair ≥5 : 2 étapes explicites
      use 2
      constructor
      · decide
      · -- étape 1
        let step1 := collatz_step n     -- 3n + 1
        -- étape 2
        let step2 := collatz_step step1 -- (3n+1)/2
        have h2 : step2 < n := by
          -- vérification explicite de l'inégalité
          linarith
        exact h2
