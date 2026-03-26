import Mathlib.Data.Nat.Basic
import Mathlib.Tactic

/- 
NX-43 v2 : FORMALISATION DE LA DESCENTE CONDITIONNELLE
Cette version abandonne la prétention de preuve universelle pour se concentrer 
sur la vérité mathématique vérifiable.
-/

def collatz_step (n : ℕ) : ℕ :=
  if n % 2 = 0 then n / 2 else 3 * n + 1

/-- 
Démonstration de la descente pour les cas pairs.
C'est la seule partie 100% prouvable sans obstruction majeure.
--/
theorem nx43_even_descent (n : ℕ) (h_even : n % 2 = 0) (h_pos : n > 0) :
  collatz_step n < n := by
  simp [collatz_step, h_even]
  exact Nat.div_lt_self h_pos (by norm_num)

/--
Identification formelle de l'obstruction pour les cas impairs.
On prouve ici que la descente directe en 1 étape est impossible pour n > 1 impair.
--/
theorem nx43_odd_obstruction (n : ℕ) (h_odd : n % 2 = 1) (h_gt1 : n > 1) :
  collatz_step n > n := by
  simp [collatz_step, h_odd]
  linarith
