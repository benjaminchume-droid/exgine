# EXGINE real 3D art resource test

This directory is an actual art-resource fixture, not a rendered placeholder.

## Asset pipeline

`exworld_test_car.exart` is the authoring resource. It describes real mesh primitives,
transforms, and material parameters. EXGINE's `exgart` tool constructs the mesh using
the engine geometry library and emits a standard Wavefront OBJ/MTL resource.

```text
exworld_test_car.exart
        |
      exgart
        |
  +-----+------+
  |            |
OBJ mesh    MTL materials
```

The normal desktop CMake build also regenerates a copy at:

`build/generated_art/exworld_test_car.obj`
`build/generated_art/exworld_test_car.mtl`

## Command line

```bash
exgart tests/art_resource/exworld_test_car.exart \
      build/generated_art/exworld_test_car.obj \
      build/generated_art/exworld_test_car.mtl
```

The generated OBJ can be opened in Blender, MeshLab, Windows 3D Viewer, and other
Wavefront-compatible viewers. Keep the `.obj` and `.mtl` files together.

The important design point is that the resource description is data, while EXGINE owns
the reusable geometry/material construction and export path. New art resources can be
authored by changing the resource data rather than adding a one-off hard-coded exporter.