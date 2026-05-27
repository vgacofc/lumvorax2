theorem hello_world (p q : Prop) : p ∧ q → q ∧ p :=
begin
  intro h,
  cases h with hp hq,
  split,
  exact hq,
  exact hp,
end
