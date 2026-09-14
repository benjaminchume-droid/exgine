# EXGINE

**EXGINE** is an experimental lightweight game engine and code-to-game runtime focused on declarative game concepts, procedural worlds, and efficient mobile rendering.

## Vision

Describe *what* a game should contain instead of manually implementing every low-level detail. EXGINE will translate high-level game concepts into an intermediate representation (IR), optimize them, and execute them through a lightweight native runtime.

Example:

```text
world {
    terrain = procedural
    terrain_height = 20
    trees = procedural
    exploration = infinite
}
```

## Initial architecture

```text
Game source / EXGINE DSL / supported languages
                    |
              Language adapters
                    |
                 EXGINE IR
                    |
                Optimizer
                    |
             Native runtime
             /      |       \
        Renderer  Physics   World
                    |
          Android / Desktop
```

## Current status

Early prototype. The first milestone is a small C++ core with a typed intermediate representation and procedural-world foundation.

## Design goals

- C++ native core
- Small memory footprint
- Mobile-first performance
- Chunked world streaming
- Procedural terrain and object generation
- Automatic level-of-detail opportunities
- Declarative game descriptions
- Language adapters through a shared IR
- Android and desktop targets

## Non-goals for v0.1

EXGINE will not initially attempt to support every programming language, photorealistic rendering, or a full AAA editor. The architecture should make those possible later without making the first prototype unmanageable.

## License

TBD during the prototype phase.
