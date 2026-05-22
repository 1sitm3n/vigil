**Shipped:**
- Repo created, SSH auth solved, project skeleton committed and pushed
- CMakeLists.txt with FetchContent for SDL3 (release-3.2.0) and glm (1.0.1)
- Vulkan SDK 1.4.350 wired up via MoltenVK
- Foundation binary builds and prints SDL + Vulkan versions cleanly

**Broken / pending:**
- Old /usr/local/share/vulkan layer files cause harmless duplicate warnings
- Overwrote previous SSH key; old GitHub key entries should be cleaned up at some point

**Notes for tomorrow:**
- Day 2: SDL3 window + Vulkan instance via vk-bootstrap, swapchain, first triangle
- vk-bootstrap needs vendoring into third_party/ — first thing tomorrow morning

## Day 2 — 2026-05-20 — Window + Vulkan device

**Shipped:**
- vk-bootstrap vendored via FetchContent
- src/core/Window — SDL3 RAII wrapper with Vulkan surface integration
- src/core/VulkanContext — instance, surface, physical device, logical device, queues
- Validation layers active in Debug, debug callback routes everything to stderr
- 1440×900 high-DPI window opens, clean shutdown on Esc

**Broken / pending:**
- Stale /usr/local/share/vulkan still emits 7 duplicate-layer warnings on startup

**Notes for tomorrow:**
- Day 3: swapchain, render pass, framebuffers, first triangle
- vkguide.dev Chapter 2 is the reference
- glslangValidator is in $VULKAN_SDK/bin — set up CMake custom command for shader compile


## Day 3 — 2026-05-21 — First triangle

**Shipped:**
- src/render/Swapchain — swapchain + render pass + framebuffers, all RAII
- src/render/GraphicsPipeline — pipeline layout + graphics pipeline, dynamic viewport/scissor
- src/render/Renderer — 2-frames-in-flight command buffers + sync, draw_frame loop
- Build-time GLSL → SPIR-V compilation via glslangValidator + CMake custom target
- shaders/triangle.{vert,frag} — hardcoded positions, Vigil palette colors
- Triangle renders at 60Hz, locked vsync, no validation errors
- GDD v1.2 — boss renamed Pale Sovereign, mockup pinned as Phase 7 reference,
  Continue removed, Validation Layers toggle stripped, design tokens locked,
  custom Vulkan UI build path declared

**Broken / pending:**
- Resize handling deferred (swapchain recreation comes Day 4+)
- Old /usr/local/share/vulkan still emits duplicate-layer warnings

**Notes for tomorrow:**
- Day 4: vertex buffers, UV attribute, descriptor sets, stb_image for textures,
  cgltf for mesh loading, textured quad → textured glTF cube
- Need to pick a free .glb to validate the loader (Polyhaven has good options)

## Day 4 — 2026-05-21 — Geometry, textures, depth, cgltf

**Shipped:**
- src/render/Buffer — RAII VkBuffer + VkDeviceMemory wrapper, host-visible persistent map
- src/render/Vertex — vec3 pos + vec2 uv, binding + attribute descriptions
- src/render/Texture — VkImage + memory + view + sampler, stb_image PNG load,
  staging buffer upload, layout transitions via one-shot command buffer
- src/render/Mesh — cgltf-based .glb/.gltf loader: positions, UVs, indices into
  vertex + index Buffers
- Descriptor set layout (combined image sampler), descriptor pool, per-FIF sets
- Push constants (mat4 MVP) on the vertex stage
- Depth attachment in Swapchain — VkImage + view + memory, render pass extended
  with depth attachment + reference + subpass dependency, framebuffers carry both
  color and depth views
- GraphicsPipeline depth stencil state enabled (compare op LESS, write enabled)
- assets/test/checker.png — Python-generated 256x256 checker with coloured corner
  markers for orientation debugging
- Cube → cgltf load of concrete_cat_statue from Polyhaven, animated tumble via
  time-based model rotation and perspective projection (Y-flipped for Vulkan NDC)

**Broken / pending:**
- Mesh ignores the glTF's own diffuse texture — using the checker for everything
- Camera is a static lookAt with hardcoded numbers; orbit camera lands Day 5
- Index buffer is uint32 regardless of source; could specialise per-mesh
- Stale /usr/local/share/vulkan duplicate-layer warnings still present at startup
- Mesh assumes first mesh, first primitive — multi-primitive .glbs ignore the rest

**Notes for tomorrow:**
- Day 5: orbit camera (yaw/pitch/distance from input), phase 1 checkpoint
- Depth buffer already shipped (pulled forward from Day 5) — frees time for camera
  polish and the 30-second clip
- Decide whether to extract glTF diffuse textures next or punt to Phase 2 with the
  knight skinning work

## Handoff to Day 5 instance

**State at end of Day 4:**
- Tumbling concrete cat statue rendering via cgltf, textured with checker.png
- Camera hardcoded in record_command_buffer at eye (0.6, 0.5, 0.9), target (0, 0.15, 0)
- Depth buffer, push constants, MVP all working (pulled depth forward from Day 5)
- All Day 4 work committed and pushed

**Day 5 remaining (per vigil_roadmap_v1.md):**
- src/core/Camera.{h,cpp} — orbit camera (yaw/pitch/distance around a target)
- Input system to feed the camera (WASD + mouse drag, scroll for zoom)
- Replace hardcoded lookAt in Renderer with camera_.view() / camera_.projection(aspect)
- Phase 1 checkpoint: 30-second clip of the cat orbiting under camera control, devlog
- Note: depth buffer item from Day 5 is already done (Day 4)

**Files the next instance will need to see on turn 1:**
- src/main.cpp (where SDL_PollEvent lives)
- src/core/Window.h and src/core/Window.cpp (to know what's exposed for events)

## Day 5 — 2026-05-21 — Orbit camera, WASD pan, phase 1 checkpoint

**Shipped:**
- src/core/Camera — orbit camera (yaw/pitch/distance around a target), Vulkan
  Y-flip baked into projection() so render code stops doing it inline
- Input plumbing in Window: InputFrame struct, consume_input(), RMB-gated
  mouse delta accumulation, scroll accumulation, WASD via SDL_GetKeyboardState
- SDL_SetWindowRelativeMouseMode toggles on RMB down/up — cursor hides and
  feeds raw deltas during drag, restores on release
- Renderer::draw_frame(camera) — view/projection driven by Camera, hardcoded
  lookAt gone
- WASD pans the target; signs flipped from the camera-frame convention so the
  scene slides under input (W = cat appears to go forward, A = left, etc.)
- Scroll zoom with min/max distance clamps (0.3m – 10m)
- Phase 1 checkpoint clip recorded: cat tumbles, orbit + zoom + pan all in
  one take, 60 FPS, no validation errors

**Broken / pending:**
- Cat model now rotates around Y only at 0.3 rad/s (dropped Day 4's X-axis
  tumble for camera readability)
- Resize handling still deferred — swapchain recreation lands when it lands
- Target pan is unbounded; not an issue at Day 5 scale but worth caging later
- Stale /usr/local/share/vulkan duplicate-layer warnings still emit at startup

**Notes for tomorrow:**
- Day 6: Tripo → Mixamo pipeline dry run on the existing knight FBX
  (vigil_tripo_mixamo_pipeline.md). Content day, no engine code.
- Knight inspection PNG shows the mesh upside-down with a rough silhouette —
  Blender pass per the pipeline doc: delete Tripo armature, R/X/-90 to stand
  it up, Ctrl+A → All Transforms, scale to ~1.8m, export
- Mixamo auto-rigger may struggle with the silhouette — if rejected after
  two marker attempts, regenerate with a more conservative Tripo prompt;
  budget 90 min, no more
- Phase 2 (skeletal animation) gates on a clean rigged knight.glb out of
  this pipeline

## Day 6 — 2026-05-21 — Tripo→Mixamo pipeline, knight + 52 animations, GDD v1.3

**Shipped:**
- FBX2glTF 0.13.1 installed (Godot fork, x86_64 binary via Rosetta 2 on M3);
  ~/bin added to PATH, quarantine attr cleared
- Tripo→Mixamo pipeline validated end-to-end on the knight FBX — upload,
  auto-rig, pack download, batch convert all working
- knight.glb (20.5 MB skinned mesh + diffuse texture) staged at
  assets/characters/knight/
- 51 animations from the Pro Sword and Shield Pack batch-converted with
  snake_case naming, staged at assets/characters/knight/anims/
- Stand to Roll downloaded separately and converted to roll_forward.glb
  (Mixamo has no Sword-and-Shield-specific roll; generic on the same
  humanoid skeleton works)
- Smoke test passed in donmccurdy gltf-viewer; Khronos validator clean
  except for standard Mixamo+FBX2glTF informational warnings
  (NODE_SKINNED_MESH_NON_ROOT, UNUSED_OBJECT /skins/N) — both expected
- GDD bumped v1.2 → v1.3: roll cut to forward-only (backward unavailable
  in Mixamo for this skeleton), §10 Shockwave reframed so inner ring
  is safe and forward-roll INTO the boss survives, §19 forced-cut
  logged, §21 #11 resolved
- Roadmap patched: Day 14 deliverable now reads forward-only roll, Day 23
  Shockwave is parry-or-roll-through

**Broken / pending:**
- Stand to Roll is 38 frames ≈ 1.27s at 30fps; engine will need
  per-animation playback-rate scaling to hit the 0.5s GDD target on
  Day 14
- Pack ships variants we likely won't use (crouch_*, kick, 180_turn,
  multiple idle/walk takes) — ~25 MB of disk noise in anims/; prune
  in Week 4 when scope is concrete
- FBX2glTF is x86_64 only (Godot fork is end-of-life since Godot 4.3
  switched to ufbx); runs under Rosetta 2 — one-time conversion cost,
  not a runtime concern
- Old stale /usr/local/share/vulkan duplicate-layer warnings still emit
  at startup, carried since Day 1
- Hit-front / hit-back reactions not covered by the pack; "impact"
  variants may substitute, decide on Day 18 when cultist AI lands

**Notes for tomorrow:**
- Day 7: Phase 2 begins — skeletal animation in the engine
- vkguide.dev Chapter 5 (textures and descriptors) for the bone-matrix
  UBO pattern; glTF 2.0 spec §4.6 (Animations) and §4.7 (Skins) in tabs
- Add skinning attributes to Vertex: vec4 joints (uint16 indices) +
  vec4 weights (float), with binding + attribute descriptions extended
- Extend Mesh.cpp to extract skin data via cgltf: joint indices/weights
  per vertex, inverse bind matrices per joint, joint parent hierarchy
- Add bone-palette UBO: mat4[128] (Mixamo skeletons are ~65 joints,
  128 is generous), new descriptor set bound per draw
- Vertex shader: for each vertex, blend up to 4 joint matrices weighted
  by `weights`, transform position and normal
- Day 7 deliverable: knight renders in T-pose with all bone-palette
  matrices set to identity — visually identical to a static mesh, but
  the skinning machinery is in place

## Handoff to Day 7 instance

**State at end of Day 6:**
- Phase 1 complete (Days 1–5): Vulkan foundation, orbit camera, textured cgltf
  load all working
- Day 6 complete: knight pipeline validated, knight.glb + 52 animations staged
  at assets/characters/knight/{knight.glb, anims/}
- GDD v1.3 and patched roadmap live in docs/; v1.2 retained for diff history
- FBX2glTF 0.13.1 installed via Rosetta 2; ready for cultist/wraith/sovereign
  in Weeks 4–5 without re-setup
- Roll is forward-only (Stand to Roll → roll_forward.glb, ~1.27s native;
  engine will speed up to 0.5s on Day 14). Shockwave is parry-OR-roll-through
  per GDD v1.3 §10.

**Day 7 work (per roadmap):**
- Add skinning attributes to Vertex: vec4 joints (uint16) + vec4 weights (float)
- Extend Mesh.cpp via cgltf: joint indices/weights, inverse bind matrices,
  joint parent hierarchy
- Skeleton struct: array of joint inverse-bind matrices, parent indices,
  local-space rest transforms
- Bone-palette UBO: mat4[128], new descriptor set bound per draw
- Vertex shader: per-vertex blend of up to 4 joint matrices weighted by
  `weights`, transform position and normal
- Day 7 deliverable: knight renders in T-pose with all bone-palette matrices
  set to identity — visually identical to a static mesh, skinning machinery
  ready for Day 8 animation sampling

**Files the next instance will need on turn 1:**
- src/render/Vertex.h, Vertex.cpp (extending the vertex format)
- src/render/Mesh.h, Mesh.cpp (extending cgltf load with skin data)
- src/render/GraphicsPipeline.h, GraphicsPipeline.cpp (new descriptor set
  for bone palette)
- src/render/Renderer.h, Renderer.cpp (UBO allocation + binding)
- shaders/triangle.vert (skinning math in the vertex stage)

**Watch for:**
- The two NODE_SKINNED_MESH_* warnings from the validator mean the mesh node
  isn't root — engine code must transform the armature root, not the mesh
  node, or skinning will read wrong
- Mixamo skeleton has ~65 joints; UBO sized at 128 mat4 = 8 KB, well within
  the 16 KB push-constant-adjacent budget but lives in a UBO not a push
- cgltf gives you joints as node indices; you'll need to map them to a flat
  joint array (the order matters — it's what the skin's `joints` array
  defines)

## Day 7 — 2026-05-22 — Skinning machinery, knight in T-pose

**Shipped:**
- src/render/Vertex.h — vertex format extended with uvec4 joints and vec4
  weights, defaults so non-skinned meshes pass through unchanged
- src/render/Skeleton.h — new struct: joint count, inverse bind matrices,
  local rest transforms, parent indices (joint-array-local, -1 for outside)
- src/render/Mesh.{h,cpp} — cgltf skin extraction: per-vertex JOINTS_0 +
  WEIGHTS_0, owning-node lookup for the skin pointer, IBMs unpacked, joint
  parents resolved via node*->index map, rest transforms baked from TRS or
  matrix per node
- src/render/GraphicsPipeline.cpp — descriptor set layout: binding 0 sampler
  (frag, unchanged) + binding 1 bone palette UBO (vert)
- src/render/Renderer.{h,cpp} — per-FIF bone palette Buffer array
  (mat4[128] = 8 KB), descriptor pool sized for UBO + sampler, both bindings
  written per set; mesh path swapped to knight.glb
- shaders/triangle.vert — uvec4/vec4 skin attributes, linear-blend skinning
  math; identity palette + normalised weights collapses to inPos pass-through
- Knight renders in T-pose, 52 joints reported, no new validation errors

**Bug found and fixed mid-day:**
- First run rendered as triangle-sized shards radiating from a central
  point. Diagnosed via two-step isolation:
    1. Swapped mesh back to the cat statue → rendered cleanly. Confirmed
       the new Vertex layout, bone palette UBO, and shader skinning math
       were sound; the bug was specific to skin extraction.
    2. Added a debug printf for first three vertices' joints + weights.
       Output: joints=(0,0,0,0) weights=(0,0,0,0,sum=0).
- Root cause: FBX2glTF emits both JOINTS_0/WEIGHTS_0 (the real influences)
  AND JOINTS_1/WEIGHTS_1 (zero-filled fallback for vertices needing >4
  influences — Mixamo clamps to 4 so these are always zero). In cgltf both
  sets share the same cgltf_attribute_type; attr.index disambiguates. The
  attribute switch took whichever came last, landing on the zero set.
  Weights sum 0 → zero skin matrix → w=0 in clip space → NaN/inf after
  perspective divide → shards.
- Fix: gate texcoord/joints/weights cases on attr.index == 0. Also added
  per-iteration zero-init on the cgltf read buffers as defensive cover.

**Broken / pending:**
- Bone palette is host-visible host-coherent (right call for Day 8's
  per-frame update path, not as fast as device-local + staging — fine)
- Mesh still ignores the glTF's own diffuse texture — knight renders with
  the Day 4 checker; texture extraction is Phase 2 cleanup
- Knight has visible backface bleed on the legs (cullMode=NONE plus
  non-manifold spots in the Tripo mesh). Will close with face culling once
  winding order is verified across all Tripo-generated assets
- Knight bounds came out at +-0.5 instead of +-0.9 — model is 1m tall not
  the intended 1.8m. Either Blender's Ctrl+A -> Apply Scale didn't take or
  Mixamo's auto-rigger normalised. Cosmetic for now (scroll out with the
  orbit cam); fix the Blender step before Week 4 so Cultist/Wraith/Pale
  Sovereign don't all come out at the wrong scale
- Stale /usr/local/share/vulkan duplicate-layer warnings still emit
- NODE_SKINNED_MESH_NON_ROOT + UNUSED_OBJECT /skins/N informational
  warnings carry over from Day 6 — engine correctly ignores the mesh-node
  transform per the glTF spec

**Notes for tomorrow:**
- Day 8: animation sampling. cgltf gives us per-channel keyframes for
  translation/rotation/scale per target joint
- Animation struct (name, duration, per-channel keyframe arrays) +
  Animator (current animation, playback time, sample -> local-space joint
  transforms at time t)
- Forward kinematics: walk parent_indices in Skeleton, world[i] =
  world[parent[i]] * local[i]. -1 parents fall back to identity (skin root
  transform is identity for the Mixamo knight at rest)
- Final palette: world[i] * inverse_bind_matrices[i], upload to the
  bone_palette_buffers_[current_frame_] via Buffer::upload each frame
- Load idle.glb as the smoke-test animation, sample at t = (now -
  start_time_), watch the knight breathe

---

## Day 8 — Animation sampling

**Goal:** take the Day 7 skinning machinery (knight in T-pose, identity bone palette) and make it sample an actual Mixamo animation. Knight should idle.

### What landed

New `src/anim/` directory:

- `Animation.h` / `Animation.cpp` — `AnimChannel` (target joint, path enum, times, values), `Animation` (name, duration, channels), and a `load_animation(path, target_skeleton)` that pulls channels out of a Mixamo `.glb` and maps node names to the knight's joint indices (with a `mixamorig:` prefix-strip fallback so files with mismatched prefixes still resolve).
- `Animator.h` / `Animator.cpp` — owns a skeleton pointer + an active animation, advances playback time on `update(dt)` with looping `fmod`, writes the bone palette on `compute_bone_palette(mat4*)`.

Updates:

- `Skeleton.h` — replaced Day 7's composed `local_rest_transforms` with split `rest_translation` / `rest_rotation` / `rest_scale` so animation channels can override one TRS component at a time. Added `joint_names` so channels resolve by name.
- `Mesh.cpp` — `node_to_trs()` helper that pulls T / R / S directly from `cgltf_node`. Matrix-form nodes aren't supported yet — logs and falls back to identity if we ever hit one.
- `Renderer.{h,cpp}` — owns one `Animation idle_animation_` and one `Animator animator_`. New `create_animation()` loads `assets/characters/knight/anims/idle.glb`. `draw_frame()` computes dt from `last_frame_time_`, ticks the animator, calls `compute_bone_palette()` into a scratch array, then memcpys into `bone_palette_buffers_[current_frame_]` **after** the in-flight fence wait so the upload doesn't race the GPU still reading the previous frame's palette. Removed the Day 4–7 model rotation in `record_command_buffer`; model matrix is identity now.

Load output is clean: `53 channels kept, 0 unresolvable target nodes, duration 3.500s`. That's 52 joint rotations plus one root translation — the standard Mixamo "in place" idle layout.

### The bug: rest pose broken too

First run with animation enabled, legs spread wide, upper body collapsed into a thin vertical sliver. Disabled the animation entirely (commented out `animator_.set_animation`, so `compute_bone_palette` falls through to pure rest TRS via the FK pipeline). Still broken — body and head OK, **arms completely missing**.

That ruled out animation sampling as the cause. With no channels overriding, `world[i] * IBM[i]` must equal identity for every joint and the render must match Day 7's T-pose. It didn't. The bug had to be in either rest TRS extraction, FK, or IBM application.

Hypothesis: `skin->joints` was not topologically sorted. My FK was a single forward pass:

```cpp
for (uint32_t i = 0; i < N; ++i) {
    const int32_t parent = skeleton_->parent_indices[i];
    world[i] = (parent >= 0) ? world[parent] * local : local;
}
```

That assumes `parent_indices[i] < i` everywhere. If any joint's parent appears later in the array, `world[parent]` is uninitialised — glm's default `mat4` constructor leaves storage uninitialised in this build config — and the child gets garbage. The leg chain branches from Hips at index 0 and would survive almost any ordering; the arms branching through Spine → Spine1 → Spine2 → Shoulder are far more sensitive.

Added a one-shot hierarchy dump from `Mesh.cpp`. Confirmed:
[Skeleton] hierarchy (52 joints):
[ 0] mixamorig:Hips             parent= -1
[ 1] mixamorig:LeftUpLeg        parent=  0
[ 2] mixamorig:RightUpLeg       parent=  0
[ 3] mixamorig:Spine            parent=  0
[ 4] mixamorig:Spine2           parent= 27   <-- non-topological
[ 5] mixamorig:LeftArm          parent=  6   <-- non-topological
[ 6] mixamorig:LeftShoulder     parent=  4
...
[27] mixamorig:Spine1           parent=  3
...
[Skeleton] WARNING: 5 joints have parent index >= their own index

Five offenders total: `Spine2(4) → Spine1(27)`, `LeftArm(5) → LeftShoulder(6)`, `RightHandThumb1(9) → RightHand(24)`, `LeftHandThumb1(12) → LeftHand(26)`, `LeftHandIndex3(35) → LeftHandIndex2(36)`. Spine2 is the bad one — it sits at the root of the entire upper-body chain. When the loop processed Spine2 at i=4, `world[27]` was uninitialised memory; from then on Neck, Head, both shoulders, both arms, both hands, and every finger inherited garbage. That matches the symptom precisely: legs perfectly fine, everything above the diaphragm scrambled.

This is presumably a Mixamo / FBX2glTF quirk — Mixamo's FBX retains the hierarchy semantically but doesn't promise array order, and FBX2glTF doesn't re-sort. Worth remembering for Cultist / Wraith / Pale Sovereign in Week 4: same pipeline, same bug if FK isn't order-independent.

### The fix

Replaced the forward pass with a memoised recursion in `Animator::compute_bone_palette`:

```cpp
std::vector<glm::mat4> world(N);
std::vector<uint8_t>   computed(N, 0);
std::function<void(uint32_t)> walk = [&](uint32_t i) {
    if (computed[i]) return;
    const int32_t parent = skeleton_->parent_indices[i];
    if (parent >= 0) walk(static_cast<uint32_t>(parent));
    const glm::mat4 local = glm::translate(glm::mat4(1.0f), t[i])
                          * glm::mat4_cast(r[i])
                          * glm::scale(glm::mat4(1.0f), s[i]);
    world[i] = (parent >= 0) ? world[parent] * local : local;
    computed[i] = 1;
};
for (uint32_t i = 0; i < N; ++i) walk(i);
```

The `computed[]` flag collapses repeated walks — for a topologically-sorted skin every joint is hit exactly once and the cost matches the original loop. For a non-topo skin every joint is hit at most twice (once on its own iteration, once when walked as somebody's parent). `std::function` overhead is meaningless at 52 joints once per frame.

### Result

Knight plays the Mixamo combat idle: bent knees, hands raised in a fighter's guard, looping cleanly at 3.5s. Legs, spine, head, arms all render correctly through the full FK chain.

### Day 7 leftovers still standing

- 1m bind-pose height instead of 1.8m (Blender Apply Scale didn't take, or Mixamo normalised). Cosmetic; fix before Week 4.
- Knight uses the Day 4 checker texture, not the diffuse from `knight.glb`. Phase 2 cleanup.
- Visible backface bleed on legs (`cullMode = NONE` plus non-manifold Tripo spots).
- Stale `/usr/local/share/vulkan` duplicate-layer warnings on startup.
- `NODE_SKINNED_MESH_NON_ROOT` informational glTF warning (we intentionally ignore the mesh-node transform on skinned meshes per spec).

### Next (Day 9)

Same pipeline, different `.glb`. Swap the load path from `idle.glb` to `walk.glb` / `run.glb` / `slash.glb` and confirm the 50-odd anims in the Mixamo pack all drive the same skeleton without surprises. Then wire animation selection to game state — first the Idle / Walk / Jog state machine from GDD §6.
