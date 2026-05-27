import Lake
open Lake DSL

package «proofs» where
  -- Settings applied to both it and its dependencies
  leanOptions := #[
    ⟨`pp.unicode.fun, true⟩ -- pretty-prints `λ` as `fun`
  ]

@[default_target]
lean_lib «Proofs» where
  -- add library configuration options here
