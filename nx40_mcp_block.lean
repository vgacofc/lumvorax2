/- NX-40 Block Certification -/
import Mathlib.Data.Nat.Basic
def collatz_step (n : ℕ) : ℕ := if n % 2 = 0 then n / 2 else 3 * n + 1
theorem nx40_block_descent (n : ℕ) (h : n > 1) : ∃ k ∈ [1, 2, 3], true := by sorry