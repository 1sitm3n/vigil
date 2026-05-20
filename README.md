<img width="1427" height="706" alt="Screenshot 2026-05-20 at 23 32 02" src="https://github.com/user-attachments/assets/1d15b1a7-fcb0-4137-980c-f971896a7d70" />

# Vigil

> A lone knight holds an ancient ruin against the rising dark.
> Seven waves, one boss, one night.

A single-arena dark-fantasy combat gauntlet built on C++20 + Vulkan 1.2 (MoltenVK on macOS).

## Project documents

- `docs/vigil_gdd_v1.1.md` — Game Design Document
- `docs/vigil_roadmap_v1.md` — 8-week production roadmap
- `docs/vigil_tripo_mixamo_pipeline.md` — asset pipeline reference

## Build

Prerequisites: Vulkan SDK 1.3+, CMake 3.25+, Ninja, C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build
./build/vigil
```

## License

All rights reserved. See `LICENSE`.
