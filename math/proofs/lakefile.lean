import Lake
open Lake DSL

package ATOMiK where
  leanOptions := #[
    ⟨`autoImplicit, false⟩,
    ⟨`relaxedAutoImplicit, false⟩
  ]

@[default_target]
lean_lib ATOMiK where
  globs := #[.submodules `ATOMiK]
