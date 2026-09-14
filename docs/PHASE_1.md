# Phase 1 — EXGINE Language Front End

## Objective

Phase 1 turns EXGINE source text into validated EXGINE IR. It is the first complete code-to-world path in the engine.

```text
EXGINE source
    -> SourceText
    -> Lexer
    -> Tokens
    -> Parser
    -> AST
    -> SemanticValidator
    -> EXGINE IR
```

## Implemented

- Source text abstraction with stable source offsets.
- Deterministic lexer with identifiers, strings, integers, numbers, booleans, keywords, braces, brackets, commas and `=`.
- `//` comments and common string escapes.
- Source line/column diagnostics for lexical and parser failures.
- Recursive AST for games, worlds and world objects.
- Parser recovery so malformed input can produce multiple diagnostics.
- Semantic validation for required game/world structure, duplicate properties, terrain placement and invalid terrain heights.
- Explicit AST-to-IR lowering with no stringly-typed runtime handoff.
- One public `Compiler::compile(SourceText)` entry point connecting all stages.
- End-to-end tests covering valid programs, tokenization and semantic rejection.
- Demo updated to compile actual EXGINE source instead of constructing IR directly.

## Phase 1 language

A minimal valid program is:

```text
game "Green World" {
    world {
        terrain = procedural
        terrain_height = 20

        building "house" {
            floors = 2
        }

        vehicle "car" {
            type = "sports_car"
        }
    }
}
```

Block-style terrain is also supported:

```text
terrain {
    type = procedural
    height = 20
}
```

## Design boundary

The lexer owns character-to-token conversion. The parser owns syntax. Semantic validation owns structural correctness. Lowering owns conversion into the existing IR. Runtime systems do not parse source text.

Phase 1 intentionally does not implement code generation, physics, rendering, procedural terrain algorithms, or a general-purpose scripting language. Those systems consume the stable IR contract in later phases.

## Checkpoint

Phase 1 passes when the complete source-to-IR path builds through the normal CMake target, tests cover the major contracts, malformed programs fail through diagnostics rather than undefined behavior, and the example exercises the same compiler path exposed to users.
