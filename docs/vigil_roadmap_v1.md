# VIGIL — Production Roadmap

**Version:** 1.0
**Duration:** 8 weeks (40 working days), Mon 25 May 2026 → Fri 17 July 2026
**Format:** Day-by-day tasks, daily deliverables, phase checkpoints, curated free resources
**Companion doc:** `vigil_gdd_v1.md` (locks scope and design)
**Output platform:** macOS (M3, primary), Windows (secondary)
**Launch target:** Steam + itch.io, end of Week 8

---

## How to use this roadmap

**Daily discipline.** Every day has a single ⚓ **deliverable**. The deliverable is binary: either it runs by end-of-day or it doesn't. If it doesn't, you stay on it the next morning *before* starting that day's work. Slipping a deliverable once is fine; slipping two in a row triggers a scope-cut review.

**Phase checkpoints.** Each Friday is a checkpoint, not a regular work day. You demo the week's deliverables to yourself in one continuous session, record a 30-second clip of the game state, write one paragraph in `devlog.md`, and decide whether the next phase opens or whether you cut scope. The clip becomes trailer material.

**Resources philosophy.** Resources listed are *the* resource for that task — not "here are five options, pick one." Don't shop for tutorials. Build, don't research.

**Tool consistency.** Editor: VS Code with C/C++ + CMake Tools + clangd. Debugger: lldb on macOS, Visual Studio on Windows. Profiler: RenderDoc for GPU, macOS Instruments for CPU. Source control: git, one commit per day minimum, branching only for spikes.

**Working hours.** Roadmap assumes ~6 productive hours per working day. Morrisons shifts go on weekends; if you have to skip a weekday, push that day's work to Saturday rather than compressing the next day.

---

## Asset delivery schedule

When each Tripo-generated asset needs to be in your `assets/` folder. Generate them in the week *before* they're needed — never on the day of integration.

| Week needed | Asset | Source | Notes |
|---|---|---|---|
| 1 | Knight character (FBX already uploaded) | Tripo + Mixamo auto-rig | Already have the mesh; week 1 runs the rig pipeline on it |
| 2 | Knight animations (Mixamo) | Mixamo | Pulled after rig step |
| 3 | — | — | Combat week, uses knight only |
| 4 | Arena pieces (walls, altar, pillars, braziers) | Tripo | ~10 static meshes |
| 4 | Cultist character + animations | Tripo + Mixamo | |
| 4 | Wraith character + animations | Tripo + Mixamo | |
| 5 | Pale King character + animations | Tripo + Mixamo | |
| 6 | VFX texture pack (sparks, embers, smoke) | Kenney / itch.io free | |
| 6 | Dead trees, rocks, rubble (perimeter dressing) | Tripo | |
| 7 | Music tracks (3) + SFX library | Pixabay, FreePD, Sonniss | |
| 7 | HDRIs (dusk, night) | Polyhaven | |

---

# PHASE 1 — Vulkan Foundation (Week 1)

**Goal:** Get a textured cube rotating in a Vulkan window on macOS via MoltenVK, with the build system, dependencies, and shader pipeline all working end-to-end.

**Pillar served:** Polish as a deliverable. None of the polish matters if the foundation leaks abstractions later. Spend the week getting CMake, dependencies, and tooling right.

**Phase resources:**
- *vkguide.dev* — Chapters 0 through 3. The single best free Vulkan learning resource. Read in order, don't skip.
- *github.com/charles-lunarg/vk-bootstrap* — Eliminates ~300 lines of Vulkan instance/device boilerplate. Single header + cpp, vendored.
- *github.com/KhronosGroup/MoltenVK* — Read the README's macOS setup section. You'll add MoltenVK as a system framework via the Vulkan SDK installer.
- *vulkan.lunarg.com/sdk/home* — LunarG Vulkan SDK for macOS. Installer puts headers, validation layers, and MoltenVK in `/usr/local`.
- *github.com/jkuhlmann/cgltf* — Single-header glTF loader. Vendor it.

## Day 1 (Mon) — Repo and build system

- Create `vigil/` repo on GitHub, MIT or proprietary licence header, `.gitignore` covering `build/`, `assets/` (large binary), `.DS_Store`, IDE folders.
- Install Vulkan SDK 1.3.x for macOS. Verify with `vulkaninfo` in terminal — should print device info for your M3.
- Create `CMakeLists.txt` with C++20, MoltenVK linkage, `FetchContent` for SDL3 and glm, vendored cgltf and vk-bootstrap in `third_party/`.
- Build "hello world" `main.cpp` that prints SDL3 + Vulkan SDK versions. Verify it compiles and runs.
- Set up VS Code workspace: `c_cpp_properties.json`, `tasks.json` for CMake build, `launch.json` for lldb debug.

⚓ **Deliverable:** Repo on GitHub, `cmake --build build` produces a binary, binary prints SDL3 and Vulkan versions.

## Day 2 (Tue) — Window and Vulkan instance

- Create `src/core/Window.{h,cpp}` wrapping SDL3 window creation. 1440×900, resizable, fixed for macOS Retina (use `SDL_WINDOW_HIGH_PIXEL_DENSITY`).
- Add main loop with SDL event polling. Handle quit on window close and Escape.
- Initialise Vulkan instance via vk-bootstrap. Enable validation layers in Debug builds only.
- Create surface from SDL3 window. Use `SDL_Vulkan_CreateSurface`.
- Pick physical device (prefer discrete or Apple Silicon integrated), create logical device, get graphics and present queues.

⚓ **Deliverable:** Window opens, Vulkan instance + device + surface initialised, clean exit on quit. No validation errors.

## Day 3 (Wed) — Swapchain and first triangle

- Create swapchain with vk-bootstrap. Triple-buffering, VSync (FIFO present mode).
- Acquire swapchain images, create image views.
- Create render pass (single colour attachment, clear on load, present on store).
- Create framebuffers per swapchain image.
- Set up command pools and per-frame command buffers (2 frames in flight).
- Write first triangle vertex + fragment shader pair (hardcoded vertex positions in vertex shader).
- Compile GLSL → SPIRV at build time via CMake custom command calling `glslangValidator` (ships with the Vulkan SDK).
- Build the simplest graphics pipeline (no descriptor sets yet), record and submit commands.

⚓ **Deliverable:** A coloured triangle on a dark background, no validation errors, clean shutdown.

## Day 4 (Thu) — Vertex buffers, texturing, mesh loading

- Refactor: vertex data now comes from a vertex buffer (host-visible for now, optimise later).
- Add UV attribute to vertex format. Modify shaders to sample a texture.
- Set up descriptor set layout, descriptor pool, descriptor set for the texture.
- Use `stb_image.h` (vendored, single header) to load a PNG as the texture. Create `VkImage`, transition layout, upload via staging buffer.
- Replace the triangle with a textured quad. Verify the texture renders correctly oriented.
- Add `cgltf` to `third_party/`. Write `src/core/Mesh.{h,cpp}` that loads a single static `.glb` and produces a vertex + index buffer + diffuse texture binding.

⚓ **Deliverable:** A textured glTF cube renders. Use any free `.glb` (Polyhaven has free models) to verify the loader.

## Day 5 (Fri) — Camera, depth buffer, phase checkpoint

- Add depth buffer. Recreate render pass with depth attachment. Update pipeline to depth-test.
- Create `src/core/Camera.{h,cpp}` with perspective projection and a third-person orbit camera (yaw + pitch + distance).
- Push view + projection matrices via push constants (avoid descriptor set churn for per-frame data).
- Make the cube rotate. Move the camera around it with WASD + mouse drag.
- Phase 1 retrospective: write `devlog.md` first entry. Record a 30-second clip of the rotating cube with camera control. Verify on Windows build (you can do this on the Air via cross-compile or skip until Omen is back — note the state).

⚓ **Phase 1 checkpoint:** Textured cube rotates, third-person camera works, no validation errors, clean shutdown, build is one-button on macOS. If any of these are broken, weekend is for fixing — Phase 2 does not start until they're all green.

---

# PHASE 2 — Skeletal Animation Pipeline (Week 2)

**Goal:** A Mixamo-rigged Tripo knight running an idle and walk animation on screen, controlled by a basic state machine. This is the single hardest week of the project — get it right and the rest is mostly content.

**Pillar served:** Polish as a deliverable. Animation is where prototypes die. Spending a full week here is correct.

**Phase resources:**
- *github.com/jkuhlmann/cgltf* — read the README on skin and animation extraction.
- *glTF 2.0 spec, sections 4.7 (Skins) and 4.6 (Animations)* — keep these tabs open all week. khronos.org has the spec.
- *vkguide.dev* — Chapter 5 (textures and descriptors) for the bone-matrix UBO pattern.
- *www.mixamo.com* — the auto-rigger lives here. You'll need an Adobe account (free).
- *github.com/facebookincubator/FBX2glTF* — CLI to convert Mixamo FBX exports to glTF. Pre-built binaries for macOS exist.
- *Sebastian Lague's animation videos (YouTube)* — conceptual primer if linear blend skinning math feels rusty.

## Day 6 (Mon) — Tripo → Mixamo pipeline, full dry run

This day is content-pipeline, no engine code. Validate the whole workflow on the knight FBX you already have, before trusting it for the other three characters.

- Install Blender 4.2 LTS if not already (free, blender.org).
- Import `tripo_convert_*.fbx` into Blender. In Object mode, delete the existing Tripo armature (the 31-bone auto-rig).
- With the mesh selected, verify T-pose, facing +Y (Blender's "forward"), feet on origin. If the Tripo mesh isn't axis-aligned, rotate and `Ctrl+A → Apply All Transforms`. Scale so the character is roughly 1.8m tall in Blender units.
- Export as FBX (just the mesh, no armature). Settings: forward = -Z, up = Y, embed textures.
- Upload to mixamo.com via "Upload Character." Place the seven joint markers (chin, wrists, elbows, knees, groin) on the 2D outline. Submit.
- If Mixamo accepts the bind (usually does on humanoid silhouettes), you now have a Mixamo-rigged knight previewing animations. Download the character with **T-pose** animation, with skin, FBX format.
- Convert that FBX to glTF: `FBX2glTF --binary --pbr-metallic-roughness mixamo_knight_tpose.fbx -o knight.glb`.
- Open the resulting `knight.glb` in macOS's QuickLook or in https://gltf-viewer.donmccurdy.com/ to verify the rig and skin look correct.

⚓ **Deliverable:** `assets/characters/knight/knight.glb` is a Mixamo-rigged knight in T-pose, viewable in browser, ~5–10 MB.

## Day 7 (Tue) — Skinned mesh data, GPU upload

- Add skinning attributes to vertex format: `vec4 joints` (uint16 indices) and `vec4 weights` (float).
- Extend `Mesh.cpp` to extract skin data from cgltf: joint indices and weights per vertex, inverse bind matrices per joint, joint hierarchy (parent indices).
- Create a `Skeleton` struct: array of joint inverse-bind matrices, parent indices, local-space rest transforms.
- Add a UBO (uniform buffer object) for the bone palette: array of 128 `mat4` joint matrices (Mixamo skeletons have ~65 joints; 128 is generous).
- Bind this UBO per draw call via a new descriptor set.
- Update vertex shader: for each vertex, blend up to 4 joint matrices weighted by `weights`, transform position and normal.

⚓ **Deliverable:** Knight renders in T-pose with all bone-palette matrices set to identity. Should look the same as the static mesh.

## Day 8 (Wed) — Animation sampling

- Extend cgltf usage to extract animations: array of channels, each with target joint + path (translation/rotation/scale), sampler with input times and output values.
- Create `Animation` struct: name, duration, per-channel keyframe arrays.
- Create `Animator` component: current animation, playback time, sample function that produces local-space joint transforms at time `t`.
- Implement sampling: for each channel, linear interpolation for translation/scale, slerp for rotation. Handle wrap-around (looping animations).
- Compute world-space joint transforms via forward kinematics traversing the parent hierarchy.
- Compute final bone palette: `world_transform * inverse_bind_matrix` per joint, upload to UBO.
- Pull a Mixamo "Idle" animation for the knight (search "idle" on Mixamo, pick one that's calm — no big sword swings), download as FBX, convert to glTF, place at `assets/characters/knight/anims/idle.glb`. Load the animation from this file, run on the knight.

⚓ **Deliverable:** Knight plays a Mixamo idle animation, looping, no jitter.

## Day 9 (Thu) — Multiple animations, state machine v1

- Pull from Mixamo: walk forward, jog forward, sword slash, dodge roll forward. Convert each, drop into `assets/characters/knight/anims/`.
- Refactor `Animator`: now supports a current animation and a *target* animation, with crossfade.
- Implement crossfade: sample both animations, blend joint transforms by interpolation factor over 0.2s.
- Create `AnimationStateMachine`: enum of states (Idle, Walk, Jog, Attack1, Roll), transitions defined as a table with conditions.
- Hook keyboard: hold W = Walk, hold Shift+W = Jog, LMB = Attack1, Space = Roll, release all = Idle.

⚓ **Deliverable:** Knight transitions between Idle, Walk, Jog, Attack1, Roll cleanly via keyboard.

## Day 10 (Fri) — Phase checkpoint, polish + safety

- Fix the inevitable issues from Day 9: jittery transitions, animations that don't loop cleanly, root motion you don't want (Mixamo animations include root translation — for now, *strip root motion* and lock the character to position; we'll re-introduce it for the roll specifically in Phase 3).
- Add a simple debug UI (Dear ImGui — vendor it from github.com/ocornut/imgui, vulkan + sdl3 backends). Show current animation, playback time, FPS.
- Phase 2 retrospective in `devlog.md`. Record a 60-second clip of the knight cycling through states. This is shareable footage already.

⚓ **Phase 2 checkpoint:** Knight runs five animations smoothly, state machine handles transitions, ImGui debug overlay works, FPS holds 60+. If any of these is broken, weekend is for fixing — Phase 3 does not start.

---

# PHASE 3 — Combat Core (Week 3)

**Goal:** All player combat mechanics functional against a stationary practice dummy. Light + heavy attacks, block, parry, roll with i-frames, stamina drain and regen, Holy Bolt spell. No enemies yet, no polish yet — just the mechanics.

**Pillar served:** Combat that demands attention. Numbers from GDD §6 land this week.

**Phase resources:**
- *Game Programming Patterns by Robert Nystrom* (free online at gameprogrammingpatterns.com) — State chapter, Observer chapter.
- *Sekiro / Dark Souls combat analysis videos* — watch one for the *feel* of parry windows. iMakeGames or Iron Pineapple's channels.

## Day 11 (Mon) — Player controller

- Create `src/game/Player.{h,cpp}`. Owns position, velocity, facing direction, animator.
- Implement WASD movement with the camera-relative rotation fix from the HTML prototype's diagnosis (don't repeat the `yaw + π` bug).
- Sprint = Shift, drains stamina at 30/s (don't drain yet, but track the input).
- Walk speed 3 m/s, sprint speed 6 m/s. Tune for *feel* — these are starting numbers.
- Third-person camera follows player at offset (2.2, 5.0) behind, with mouse-look yaw/pitch (lock cursor via SDL).

⚓ **Deliverable:** Knight moves around a flat plane with WASD and mouse-look, animation responds correctly (Idle ↔ Walk ↔ Jog).

## Day 12 (Tue) — Light attack and combo system

- Implement attack state with three sub-states (Attack1, Attack2, Attack3) corresponding to the three combo hits.
- Each attack: startup → active → recovery, with frame data from GDD §6 (0.30s / 0.10s / 0.40s for light).
- Combo window: 0.5s after recovery start. If LMB pressed in window, transition to next attack. Otherwise return to Idle.
- Pull three light attack animations from Mixamo ("Sword And Shield Slash," variants). Convert, drop into anims folder.
- Place a stationary "practice dummy" cube in the scene. On attack landing (active frames + within attack arc), print "HIT" to console.

⚓ **Deliverable:** Three-hit combo works, console prints hit on dummy when in range, broken combos return to idle.

## Day 13 (Wed) — Heavy attack, block, stamina

- Add Heavy attack (Shift + LMB), single animation, longer startup (0.55s), no combo, costs 24 stamina.
- Add Block (hold RMB): transitions to Block state, drains stamina = 50% of incoming damage. Pull Mixamo "Sword And Shield Block Idle" animation.
- Implement Stamina component: max 100, current value, regen rate 25/s when not attacking / blocking / sprinting.
- Light attack costs 12 stamina, heavy costs 24, sprint drains 30/s.
- ImGui debug bar showing current stamina value.
- Hook stamina to actions: can't attack if < 10, can't roll if < 25.

⚓ **Deliverable:** Stamina drains and regens correctly across all actions, gates input as designed.

## Day 14 (Thu) — Parry, riposte, roll with i-frames

- Implement Parry: tap RMB within a 0.15s window before getting hit. Track "block press time" — if a hit lands within 150ms of the press, it's a parry.
- (For now, "getting hit" is simulated via a debug button — N to fire a fake incoming attack at the knight in 0.5s.)
- Parry success: brief stagger animation on the *practice dummy*, opens Riposte state for 1.5s.
- Riposte: pull Mixamo "Standing Melee Combat Attack" animation. 60 damage, hitstop will come in Phase 6.
- Roll: Space, 0.5s total, 4m distance in input direction. **I-frames active from 0.10s to 0.35s** — store an `invulnerable` flag on the player that the damage system later respects.
- Pull Mixamo "Sword And Shield Roll Forward" animation, retain root motion for the roll specifically.

⚓ **Deliverable:** Parry → riposte chain works against fake attack. Roll moves the player 4m and has visible i-frames in debug.

## Day 15 (Fri) — Holy Bolt spell, Faith resource, checkpoint

- Implement Faith resource: 60 max, 5/s regen always.
- Holy Bolt cast: Q key, 0.8s cast time, 20 Faith cost, fires a projectile in camera-facing direction.
- Projectile entity: position, velocity, lifetime. Weak homing within 30° cone toward nearest enemy in 40m. (No enemies yet — for testing, just lock to the dummy.)
- On hit: 50 damage, despawn, particle burst placeholder (just print).
- Pull Mixamo "Standing 1H Magic Attack" animation for the cast, lock player in place during cast.
- Phase 3 retrospective. Record a 90-second clip demonstrating all combat moves: combo, heavy, block, parry, roll, spell. This becomes a piece of trailer footage.

⚓ **Phase 3 checkpoint:** All player combat mechanics work in isolation. Stamina/Faith economies feel right (tune the numbers if needed). The 90-second demo clip looks like a real game's combat tutorial. No enemies, no environment, no polish — just *mechanics*. If anything's missing, weekend is for fixing.

---

# PHASE 4 — Arena, ECS, Enemies (Week 4)

**Goal:** The full arena environment is built, lit, and atmospheric. Cultist and Wraith enemies fight back. Waves 1–6 spawn correctly in sequence.

**Pillar served:** Atmosphere over scope. The arena and its lighting do half the game's storytelling — this week is where the game starts to *look* like a game.

**Phase resources:**
- *github.com/skypjack/entt* — production-grade C++ ECS, single-header. Vendor it. Easier than writing your own when you have 4 weeks left.
- *learnopengl.com — shadow mapping chapter* — concepts translate cleanly to Vulkan. (Direct Vulkan CSM tutorials are rare; the OpenGL chapter is the canonical conceptual reference.)
- *Sascha Willems' Vulkan samples — `shadowmapping` and `deferredshadows`* — for the Vulkan-specific implementation details.

## Day 16 (Mon) — Arena environment

- Generate Tripo assets for the arena: ruined keep walls (5–7 modular pieces), broken altar (centre piece), 3 broken pillars, brazier (will be instanced ×4), 3 dead trees, 5 rocks. Aim for ~2 hours of Tripo generation.
- Layout pass: in Blender, position all pieces into the 80×80m courtyard layout from GDD §11. Export as a single `arena.glb` (or a few `.glb` files if too heavy as one).
- Load into the engine. Render the arena. Place the knight at the south breach.
- Tune scale: characters and arena pieces must match. A wall should be ~3m tall relative to the 1.8m knight.

⚓ **Deliverable:** Knight stands in the arena. You can walk around it. It looks like a place.

## Day 17 (Tue) — ECS and lighting

- Integrate entt. Define core components: `Transform`, `MeshRenderer`, `Animator`, `Health`, `CombatActor`, `AIController`.
- Refactor player to use components instead of a single class. (This will hurt for a few hours but pays back this week.)
- Implement directional sunlight + 4 point lights at brazier positions.
- Cascaded shadow maps for the sun (3 cascades, 2048×2048 each). This is non-trivial — budget all afternoon.
- Brazier point lights flicker (sine-noise on intensity, 0.7 base ± 0.3).

⚓ **Deliverable:** Sun casts shadows correctly across the arena. Braziers light up the player as he walks near. Scene looks atmospheric for the first time.

## Day 18 (Wed) — Cultist enemy

- Generate Tripo Cultist (robed humanoid, hooded). Run the Tripo → Mixamo pipeline from Day 6. Pull Mixamo animations: idle, walk, run, sword slash, lunge, hit reaction, death. Drop in `assets/characters/cultist/`.
- Create `Enemy` component (HP, attack damage, aggro range, attack cooldown).
- Spawn 3 cultists at perimeter spawn points.
- Implement basic AI: behaviour tree or simple state machine — Idle → Chase → Attack. Chase if player within 25m, attack if within 1.8m.
- Hook into combat: cultist hit by player attack → take damage → if HP ≤ 0, play death anim, despawn after 2s.
- Cultist attacks → trigger damage on player if not blocking, not rolling (no i-frames).

⚓ **Deliverable:** Cultists chase the player, attack on contact, die on third light combo. Player can take damage and die (death state just prints "YOU DIED" for now).

## Day 19 (Thu) — Wraith enemy

- Generate Tripo Wraith (gaunt, undead, two-handed sword). Same pipeline as Cultist.
- Pull Mixamo animations: idle, walk (slow shamble — "Zombie Walk"), heavy horizontal swing, overhead slam, hit reaction, death.
- Wraith AI inherits from Cultist's AI but with longer attack telegraphs and the queue rule from GDD §8 (won't engage if another Wraith is mid-attack within 4m).
- Wraith overhead slam: parry-able with the 90-damage riposte. Test this — the parry should feel earned.

⚓ **Deliverable:** Wraiths fight differently from cultists. Player has to slow down and read attacks. Parry-into-riposte against a Wraith feels satisfying.

## Day 20 (Fri) — Wave manager, checkpoint

- Create `WaveManager` system. Reads a wave definition table (see GDD §9), spawns enemies at random perimeter nodes, tracks remaining enemies, triggers next wave on clear.
- Implement waves 1–6 from the table. Skip wave 7 boss for now.
- 5-second breather between waves: heal player +20 HP, fade-in wave banner placeholder (ImGui text overlay for now).
- Phase 4 retrospective. Record a 2-minute clip surviving waves 1–4. This will be the spine of the eventual trailer.

⚓ **Phase 4 checkpoint:** A full session — start engine, run waves 1–6, die or survive — is playable end-to-end. The game has its skeleton. No boss, no polish, but you can *play it*. If waves don't chain or enemies don't AI correctly, weekend is for fixing.

---

# PHASE 5 — Boss and Flow (Week 5)

**Goal:** The Pale King fights. Full game loop runs: menu → 7 waves → boss → death/victory → back to menu.

**Pillar served:** One run, one arc. This is the week the game becomes a *complete experience* rather than a combat sandbox.

**Phase resources:**
- *Boss design talks from GDC* — search "GDC boss design" on YouTube. Mark Brown's *Boss Keys* series episode on Hollow Knight is the strongest single primer.

## Day 21 (Mon) — Pale King character

- Generate Tripo Pale King: larger character (tattered crown, long cape, greatsword). Spend extra time on Tripo prompts — this is the hero asset for the climax.
- Tripo → Mixamo pipeline. Pull animations: idle, walk, sword combo (3 hits), charge attack ("Rifle Run Forward" works visually for the lunge — scale duration), shockwave (find a "Hit From Top" or kneel-down animation, repurpose), summon gesture ("Praying" or "Casting"), roar, hit reaction, death (kneel).
- Scale up 1.4× during instantiation. He should tower over the knight.

⚓ **Deliverable:** Pale King stands in the arena, plays idle and walk. Looks intimidating relative to the player.

## Day 22 (Tue) — Boss AI phase 1

- Implement boss state machine: Idle → Approach → Sword Combo (random) / Charge (random) / Summon (HP gates at 75% and 60%).
- Sword Combo: 2-hit, parry-able on second. 35 damage per hit.
- Charge: telegraph (foot-stamp animation), then 8 m/s line attack across arena, 50 damage. **Vulnerable 1.5s after miss.**
- Summon: 2s cast, summons 2 Cultists from altar. Interruptible by 200+ damage in one hit.
- Boss HP 800. Display boss HP bar via ImGui (final UI in Phase 7).

⚓ **Deliverable:** Pale King phase 1 fight is playable. He uses all three attacks. Can be killed.

## Day 23 (Wed) — Boss AI phase 2

- Phase transition at 50% HP: roar animation, screen shake placeholder, music sting placeholder.
- Add Shockwave attack: slams sword down, AoE ring expands to 6m over 0.5s, 25 damage, knockdown. Must be rolled or jumped over.
- Extend sword combo to 3 hits. Third hit is overhead slam, parry-able.
- One-time summon at 25% HP: 2 Wraiths from altar.

⚓ **Deliverable:** Full Pale King fight phase 1 + phase 2 playable. Average fight length ~3–5 minutes.

## Day 24 (Thu) — Death and victory states

- Player HP = 0 → Death state. Camera drops to ground over 2s, fade to black, death screen with stats (wave reached, time, hits taken, parries).
- Boss HP = 0 → Victory state. King kneels (death animation), 4s camera pan, fade to white, victory screen with final stats.
- "Restart" returns to main menu; "Quit" exits.
- Implement run stats tracking: wave reached, run start time, hits taken counter (incremented on player damage event), parries landed counter.

⚓ **Deliverable:** Game ends correctly on death and victory. Stats screen shows real numbers.

## Day 25 (Fri) — Main menu, pause, phase checkpoint

- Main menu scene: title art (placeholder text "VIGIL" in large serif), three buttons ("Begin Vigil," "Options," "Quit").
- Pause menu: Esc during gameplay → Resume, Restart, Options, Main Menu.
- Options menu: master volume, music volume, SFX volume (sliders, no effect yet), brightness, controller toggle.
- Implement scene transitions (Main Menu → Game → Death/Victory → Main Menu).
- Phase 5 retrospective. Record full 30-minute playthrough clip. *This is your first full run.*

⚓ **Phase 5 checkpoint:** Full game loop runs end-to-end. Menu → wave 1 → wave 7 boss → death or victory → menu. The game is *complete in skeleton form*. From here on, every week is polish.

---

# PHASE 6 — Polish Layer 1 (Week 6)

**Goal:** Make the combat *feel* good. Hitstop, screen shake, hit flash, damage numbers, parry effect, sword trail, particles. Every item in GDD §14 except audio.

**Pillar served:** Polish as a deliverable. This is the week the game stops looking like a student project.

**Phase resources:**
- *"Juice it or lose it" — Martin Jonasson & Petri Purho* (YouTube). 20 minutes. Watch Monday morning.
- *"The Art of Screenshake" — Jan Willem Nijman* (YouTube). 30 minutes. Watch Tuesday morning.
- *Squirrel Eiserloh — Math for Game Programmers: Fast and Funky 1D Nonlinear Transformations* — the easing functions reference.

## Day 26 (Mon) — Hitstop, screen shake

- Global `TimeScale` value, defaults to 1.0. All game-logic deltas scale by it. UI does not.
- On landed light attack: TimeScale = 0 for 80ms, then 1.0.
- On landed heavy attack or riposte: TimeScale = 0 for 250ms.
- Screen shake: camera position offset by a Perlin-noise-driven vector, amplitude decays over duration. 0.15s duration, 4–8px amplitude depending on event.
- Trigger shake on: heavy attack land, boss shockwave, player taking damage.

⚓ **Deliverable:** Combat hits *feel* heavier. Compare against today's morning gameplay — there should be a visceral difference.

## Day 27 (Tue) — Hit flash, damage numbers

- Hit flash: on enemy taking damage, push a uniform to their fragment shader that tints the output white for 60ms. (Add a `material.tint_color` and `material.tint_strength` uniform.)
- Player hit flash: full-screen red vignette pulse, 200ms, via a post-process pass. (This is your first post-process — you'll add bloom and tone mapping in Phase 7.)
- Damage numbers: spawn a 3D billboard text at enemy hit position. Floats upward 60px over 1s, fades out. Colour-coded per GDD §14 (white/yellow/purple/gold).
- Use stb_truetype to rasterise a serif font (vendor it; load EB Garamond TTF from Google Fonts) into a texture atlas. Reuse this atlas for HUD later.

⚓ **Deliverable:** Hit feedback is dense. Every strike produces a number, a flash, and an audible *crack* (audio placeholder — empty space where it'll go).

## Day 28 (Wed) — Parry feedback

- On successful parry: TimeScale = 0.5 for 200ms, camera FOV briefly narrows by 10°, white radial flash from contact point.
- Riposte camera: temporarily zoom in on the attacker, hold for the 250ms hitstop, then resume.
- This is the *signature moment* of the game. Spend the day getting it right. Iterate on timings.
- Pull a "Crash Cymbal" or "Anvil Strike" SFX placeholder from Sonniss for the parry clang — even before the audio system is wired up, you can hardcode play it via a temporary `SDL_OpenAudioDevice` path.

⚓ **Deliverable:** Parry feels like the best move in the game. When you land one, you want to do it again.

## Day 29 (Thu) — Sword trail

- Implement a ribbon trail: each frame, sample blade tip world position. Maintain a ring buffer of last 8 positions.
- Generate a triangle strip from these positions (each segment is a quad facing the camera, edges along the trail).
- Render with additive blending, fade alpha from full at the tip to zero at the tail.
- Trail visible only during attack active frames.
- Tint trail colour per combo position: white (light 1), pale gold (light 2), gold (light 3, knockback), red-gold (heavy), blue-white (riposte).

⚓ **Deliverable:** Sword trails actually trail. Fixed from the HTML prototype's static-quad fake.

## Day 30 (Fri) — Particle system, phase checkpoint

- CPU particle system: pool of 500 particles, each with position, velocity, lifetime, scale, colour.
- Render as instanced quads facing camera.
- Footstep dust: spawn 3 particles per step.
- Brazier embers: continuous spawn from brazier positions, 20 active per brazier.
- Hit sparks: 8 particles on every hit, radiating from contact point.
- Phase 6 retrospective. Record a 90-second clip of combat with all polish layer 1 on. Compare to Phase 3 clip — should look like a different game.

⚓ **Phase 6 checkpoint:** Combat feels *great* without audio. Visual feedback is dense, responsive, satisfying. If hitstop, screen shake, hit flash, damage numbers, parry effect, sword trail, or particles aren't shipping, weekend is for fixing.

**Scope-cut review at this checkpoint.** Are you on schedule? If Phase 6 finishes late (Sat or Sun), invoke scope cut item 1 from GDD §19 (bloom + vignette → ship ACES tone mapping only) and consider item 2 (point-light shadows from braziers).

---

# PHASE 7 — Audio, Lighting Curve, HUD (Week 7)

**Goal:** Audio mix complete. Lighting transitions across waves. HUD final pass. The game looks and sounds like a finished product.

**Pillar served:** Atmosphere over scope.

**Phase resources:**
- *miniaudio.h documentation* (single-header, included in the file). Read the "Engine API" section.
- *Sonniss GameAudio bundle* — google "Sonniss GDC free SFX" each year, downloads are 20+ GB of CC-licensed game SFX. Curate your set on the weekend before Phase 7.
- *Pixabay Music* and *FreePD.com* — free instrumental tracks. Search "dark ambient," "medieval combat," "orchestral epic."

## Day 31 (Mon) — Audio integration

- Vendor `miniaudio.h` into `third_party/`.
- Initialise miniaudio engine in `src/audio/AudioSystem.{h,cpp}`. Bus structure: master → {music, sfx, ambient, ui}.
- Hook volume settings from options menu into bus gains.
- Implement `play_sfx(name, position)` and `play_music(name, loop, fade_in)`.
- Test with a single SFX placeholder.

⚓ **Deliverable:** Audio plays from miniaudio. Volume sliders affect it. No crashes on rapid SFX spam.

## Day 32 (Tue) — SFX library

- Curate ~40 SFX from your Sonniss/FreeSound pile: sword swings (3), impacts (4 — flesh, armour, parry-clang, miss), footsteps on stone (3), player roll grunt, player hit grunt (2), player death, cultist death (3), wraith moan (loop), Pale King roar (boss only), brazier crackle (loop), wind ambient (loop), distant ravens (random), UI menu click, wave banner sting, victory chime, death chime.
- Tag each SFX with its trigger event in code. Hook into existing events (hit landed, parry success, etc.) replacing the placeholder prints.
- 3D-positional audio for crackles and ambient enemy moans — use miniaudio's `ma_sound_set_position`.

⚓ **Deliverable:** Combat sounds dense and satisfying. Each event has a unique SFX. Spatial audio for ambient sounds works.

## Day 33 (Wed) — Music

- Pick 3 instrumental tracks: ambient watchtower (waves 1–3), the dark hours (waves 4–6), the Pale King's vigil (boss). Sources: Pixabay Music, FreePD, OpenGameArt, itch.io free music packs. CC0 or CC-BY only. Document attribution in a `CREDITS.md`.
- Implement track crossfading: 3-second crossfade on wave transitions.
- Boss music: trigger sting + crossfade on wave 7 spawn.
- Boss phase 2: layer in additional intensity (either crossfade to a remix or simply bump music volume by 1.2× — quickest correct path).

⚓ **Deliverable:** Music underscores the run. Crossfades feel natural. Boss arrival has weight.

## Day 34 (Thu) — Lighting curve

- Implement `LightingState` per wave (GDD §12 table). Each state has: sun colour + intensity, ambient colour + intensity, fog density + colour, brazier intensity multiplier, sky colour.
- During inter-wave breathers, interpolate from current state to next over the 5-second window.
- Wave 7 transition: faster shift (2s), darker, brazier multiplier ×1.4 for boss arena.
- Verify each state visually — record a screenshot at the start of each wave for `marketing/screenshots/`.

⚓ **Deliverable:** Lighting transitions are visible and meaningful. Wave 7 looks distinctly different from wave 1.

## Day 35 (Fri) — HUD final pass, phase checkpoint

- Replace ImGui debug HUD with the final HUD per GDD §15: HP/Stamina/Faith bars top-left, wave Roman numeral top-centre, boss bar bottom-centre during boss fight.
- Wave banner: centre-screen serif Roman numeral + wave title, fades in 0.5s, holds 2s, fades out 1s.
- Main menu polish: title art (use a rendered screenshot of the arena at dusk with the knight in pose), buttons with hover/click states.
- Death and victory screens: full layout, real fonts, run stats prominently displayed.
- Phase 7 retrospective. Record a *full* 30-minute playthrough. This is the rough cut of your trailer source.

⚓ **Phase 7 checkpoint:** The game *looks and sounds shipped*. Anyone watching the playthrough should not be able to tell it's an in-development build. If any UI element looks ImGui-ish, fix it now.

**Scope-cut review.** If you're behind here, invoke cut items 3 (boss phase 2 mechanics — ship phase 1 only) and 5 (sprint mechanic). These are recoverable cuts. Items 6+ are visible to reviewers; avoid.

---

# PHASE 8 — Ship (Week 8)

**Goal:** Game is on Steam and itch.io with a store page, trailer, screenshots, and devlog. Build is stable on macOS and ideally on Windows.

**Pillar served:** Polish as a deliverable.

**Phase resources:**
- *partner.steamgames.com* — Steam Partner site. Application takes 1–3 days to approve; **submit the Steamworks dev account application on Day 36 morning** to give it processing time.
- *Steamworks Documentation — Coming Soon Page* — what assets you need (capsule images, trailer, descriptions).
- *itch.io upload docs* — itch.io/docs/creators
- *Steam Direct fee: $100 USD* one-time payment required for Steam app distribution.

## Day 36 (Mon) — Save system, settings persistence

- **Morning first task:** submit Steamworks Direct application (steamworks.com). It takes 30+ hours to process; start now so it's ready by Day 39.
- Save file: `~/Library/Application Support/Vigil/save.dat` on macOS, `%APPDATA%/Vigil/save.dat` on Windows. JSON format (use nlohmann/json, vendor it).
- Persist: best run (wave, time, hits), settings (volumes, brightness, controller pref).
- Load on launch. Save on settings change and on run end.
- Test save/load across game restarts.

⚓ **Deliverable:** Settings and best run persist across launches.

## Day 37 (Tue) — Controller support

- SDL3 has GameController API built in. Bind: left stick = move, right stick = camera, A = roll, X = light attack, Y = heavy attack, RB = block/parry, B = spell, Start = pause.
- Implement input abstraction so KB+M and controller both feed into the same `InputState`.
- Test on whatever controller you have (DualShock, Xbox, Steam controller — SDL handles all of them).
- Add controller-prompt UI (show controller buttons instead of KB+M icons when controller is detected).

⚓ **Deliverable:** Full game playable on controller. Prompts switch dynamically.

## Day 38 (Wed) — Performance, bug fixing

- Profile with macOS Instruments. Look for: dropped frames, CPU spikes, memory leaks.
- Profile GPU with RenderDoc or Xcode Metal capture (via MoltenVK). Look for: unbalanced render passes, descriptor set churn, oversized textures.
- Run a 2-hour stress test: leave the game running, looping deaths, watch for crashes and memory growth.
- Fix the 10 most-noticeable bugs from your `bugs.md` (which you've been maintaining all 8 weeks, right?).

⚓ **Deliverable:** Stable 60 FPS on M3 Air. No crashes in 2-hour stress test. Top 10 bugs fixed.

## Day 39 (Thu) — Store pages and trailer

- **Trailer cut:** assemble 60 seconds from your captured clips across Phases 3–7. Structure: 0–10s atmospheric establishing (knight in arena at dusk), 10–35s combat highlights (montage of parries, ripostes, spells, dodges), 35–50s boss reveal + fight, 50–60s logo + title + "Coming to Steam." Use DaVinci Resolve (free) or iMovie.
- **Screenshots (8):** atmospheric arena, combat moment (with hit flash), parry mid-zoom, boss reveal, boss fight wide, victory screen, knight portrait, main menu.
- **itch.io page:** description (lift from GDD logline + elevator), price (free or pay-what-you-want is fine), upload macOS + Windows builds (zip them).
- **Steam Coming Soon page** (if Steamworks approval came through): description, capsules (use templates from steamdb.info/coming-soon — Steam has specific sizes: header 460×215, library 600×900, etc.), trailer, screenshots.

⚓ **Deliverable:** itch.io page is *live*. Steam Coming Soon page is *submitted* (Valve reviews these in 5 business days).

## Day 40 (Fri) — Launch, devlog, post-mortem

- Final smoke test: run the game from scratch, complete a victory run, complete a death run, change settings, verify save.
- Publish itch.io build (toggle from draft to public).
- Write a devlog post: technical breakdown of the project (Vulkan stack, Tripo + Mixamo pipeline, scope cuts you survived). Publish on itch.io and your portfolio site.
- Post the trailer to YouTube. Pin it on your portfolio.
- **Post-mortem in `devlog.md`:** what shipped, what was cut, what you'd do differently, what you learned. This is for *you*, not the audience.
- Optional: announce on Twitter/Bluesky, /r/gamedev, /r/IndieDev. Tag with #screenshotsaturday if it's a Saturday.

⚓ **Deliverable:** **Vigil is shipped.** Live on itch.io, Coming Soon on Steam, trailer on YouTube, devlog published, portfolio updated. Take Saturday off.

---

# Scope cut triggers (re-stated)

Re-read GDD §19. The order is non-negotiable. Triggers:

- **End of Phase 4 (Day 20):** If wave system or enemy AI isn't complete, cut item 1 (bloom + vignette) and item 2 (point-light shadows) preemptively to free Phase 7 time.
- **End of Phase 6 (Day 30):** If polish layer 1 (hitstop through particles) isn't complete, cut items 1–2 if not already, and consider item 3 (boss phase 2) and item 4 (Wraith overhead).
- **End of Phase 7 (Day 35):** If audio + lighting + HUD aren't all complete, cut items 5 (sprint) and 6 (Wave 6 sub-waves) — these are *visible* compromises but recoverable.
- **End of Phase 8 (Day 39):** If you're not ship-ready, cut items 7–10 ruthlessly. Save system can be in-memory only. Controller support can be deferred to a v1.1 patch. Steam can be pushed to next week if itch.io launches on time.

**What you never cut:** combat feel, lighting curve, audio mix, the Pale King existing.

---

# Definition of done (re-stated for ship day)

- [ ] itch.io page live with both macOS and Windows builds, trailer, 8 screenshots, description
- [ ] Steam Coming Soon page submitted (live within Valve's 5-day review window)
- [ ] 60-second trailer published on YouTube
- [ ] Devlog post published
- [ ] Portfolio site updated with project entry
- [ ] No crashes in 2-hour stress test
- [ ] Locked 60 FPS on M3 Air
- [ ] Controller and KB+M both work
- [ ] Best run + settings persist across restarts
- [ ] All seven waves play, Pale King phase 1 minimum (phase 2 if not cut)

---

**Companion deliverable:** `vigil_gdd_v1.1.md` will land alongside Day 1 work, with the asset BOM updated to Tripo-as-source-for-everything and all open questions marked resolved. The roadmap is the working document; the GDD is the reference.

**Start date: Monday 25 May 2026. Day 1 begins by creating the repo and Vulkan SDK install.**
