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

## Day 8 — 2026-05-22 — Animation sampling, idle.glb on knight

**Shipped:**
- src/anim/Animation.{h,cpp} — AnimChannel (target joint, path enum
  Translation/Rotation/Scale, times array, vec3 or quat values),
  Animation (name, duration, channels), and load_animation(path,
  target_skeleton) that pulls channels out of a Mixamo .glb and maps
  node names to the knight's joint indices (with mixamorig: prefix-
  strip fallback so files with mismatched prefixes still resolve)
- src/anim/Animator.{h,cpp} — skeleton pointer + active animation
  pointer, update(dt) advances playback_time_ via fmod for looping,
  compute_bone_palette(mat4*) writes the per-frame palette. Pure CPU.
- Skeleton.h split: rest_translation / rest_rotation / rest_scale as
  separate components so animation channels can override one TRS slot
  at a time without re-decomposing. joint_names added for name-based
  channel resolution at load
- Mesh.cpp — node_to_trs() helper pulls T/R/S directly from cgltf_node;
  matrix-form nodes log + fall back to identity (Mixamo doesn't emit
  matrix-form)
- Renderer.{h,cpp} — owns one Animation idle_animation_ and one
  Animator animator_; create_animation() loads
  assets/characters/knight/anims/idle.glb. draw_frame() computes dt
  from last_frame_time_, ticks the animator, computes the palette into
  palette_scratch_, then memcpys into bone_palette_buffers_[
  current_frame_] AFTER the in-flight fence wait — order matters so
  the upload doesn't race the GPU still reading the previous frame's
  palette
- Removed the Day 4–7 model rotation from record_command_buffer; model
  matrix is identity now (animation owns the motion)
- idle.glb load is clean: 53 channels kept, 0 unresolvable target
  nodes, duration 3.500s — that's 52 joint rotations + 1 root
  translation, the standard Mixamo "in place" idle layout
- Knight plays the Mixamo combat idle: bent knees, hands raised in a
  fighter's guard, looping cleanly at 3.5s

**Bug found and fixed mid-day:**
- First run with animation on: legs spread wide, upper body collapsed
  to a thin vertical sliver. Disabled animation entirely (commented
  out animator_.set_animation so compute_bone_palette falls through to
  rest TRS via the FK pipeline) — still broken, body and head OK but
  arms completely missing. Ruled out animation sampling as the cause;
  with no channels overriding, world[i]*IBM[i] must equal identity for
  every joint and the render must match Day 7's T-pose
- Hypothesis: skin->joints not topologically sorted. The FK was a
  single forward pass — world[i] = (parent>=0) ? world[parent]*local
  : local — which assumes parent_indices[i] < i everywhere. If a
  joint's parent appears later in the array, world[parent] is
  uninitialised memory (glm's default mat4 constructor leaves storage
  undefined in this build config), and the child gets garbage
- Added a one-shot hierarchy dump from Mesh.cpp. Confirmed: 5 of 52
  joints with parent index >= own index. The killer was Spine2 at
  index 4 with parent Spine1 at index 27. When the loop processed
  Spine2, world[27] was uninitialised; from then on Neck, Head, both
  shoulders, both arms, both hands, every finger inherited garbage.
  Matches the symptom precisely — leg chain branches from Hips(0) and
  survives any ordering, upper-body chain rooted at Spine2 collapses.
  Other 4 offenders: LeftArm(5)->LeftShoulder(6), and three finger
  joints — none broke the silhouette as visibly as Spine2
- Mixamo / FBX2glTF quirk — Mixamo's FBX retains hierarchy semantically
  but doesn't promise array order, and FBX2glTF doesn't re-sort.
  Cultist / Wraith / Pale Sovereign in Week 4 will hit the same thing
  unless FK is order-independent
- Fix: replaced the forward pass with memoised recursion in
  Animator::compute_bone_palette. computed[] bitmap collapses repeated
  walks — topo-sorted skin hits each joint exactly once (cost matches
  the original loop), non-topo skin hits each at most twice (once on
  its own iteration, once when walked as someone's parent).
  std::function overhead is meaningless at 52 joints per frame. Max
  recursion depth on Mixamo is ~11
  (Hips->Spine->Spine1->Spine2->Shoulder->Arm->ForeArm->Hand->
  Finger1->Finger2->Finger3), stack-safe

**Broken / pending:**
- 1m bind-pose height instead of 1.8m — carried from Day 7. Blender
  Apply Scale didn't take or Mixamo normalised. Fix in Blender before
  Week 4 so Cultist/Wraith/Sovereign don't all come out short
- Knight renders with the Day 4 checker, not the diffuse from
  knight.glb — carried from Day 7, Phase 2 cleanup
- Visible backface bleed on legs — carried from Day 7, cullMode=NONE
  plus non-manifold Tripo spots; switch to BACK after the winding-
  order pass on Tripo output
- Stale /usr/local/share/vulkan duplicate-layer warnings still emit
  at startup (carried since Day 1)
- NODE_SKINNED_MESH_NON_ROOT + UNUSED_OBJECT /skins/N informational
  glTF warnings carry — engine intentionally ignores the mesh-node
  transform on skinned meshes per spec

**Notes for tomorrow:**
- Day 9: multiple animations + state machine v1 (per roadmap Phase 2)
- Pull from the Mixamo pack already on disk: walk_forward.glb,
  jog_forward.glb (or run.glb — ls assets/characters/knight/anims/
  to confirm exact filenames), slash_1.glb / slash_2.glb / slash_3.glb
  for the combo, roll_forward.glb
- Refactor Animator: support a current animation AND a target
  animation with crossfade. 0.2s crossfade per roadmap brief. The
  split-TRS approach in Skeleton lets us blend per-channel (lerp t/s,
  slerp r) — linear blending of composed mat4s breaks under rotation
- AnimationStateMachine: enum of states (Idle, Walk, Jog, Attack1,
  Attack2, Attack3, Roll), transitions table with conditions and
  durations. Owns the timer for each phase (startup, active, recovery,
  combo-window) — not a flat enum-with-instant-transitions
- Frame data per GDD §6: light attack startup 0.30s / active 0.10s /
  recovery 0.40s, combo window 0.5s from recovery start; roll 0.5s
  with i-frames 0.10–0.35s, 4m forward in facing direction (forward-
  only per GDD v1.3 §21 #11)
- Keyboard: hold W = Walk, Shift+W = Jog, LMB = Attack1 (chains to
  Attack2/Attack3 in combo window), Space = Roll, release all = Idle
- Mixamo animations include root translation. Strip it in code for
  locomotion + combat by zeroing the root bone's translation each
  frame. EXCEPTION: roll_forward keeps root motion — the 4m forward
  distance comes from the animation, not from a hand-coded velocity
- Carryovers (1m scale, checker texture, backface bleed) are NOT in
  scope for Day 9 — they have their own fix windows in Phase 2
  cleanup or pre-Week 4

## Handoff to Day 9 instance

**State at end of Day 8:**
- Phase 1 complete (Days 1–5): Vulkan foundation, orbit camera,
  textured cgltf static-mesh load all working
- Phase 2 in progress (Days 6–8): Mixamo pipeline validated (knight
  + 51 animations staged), skinning machinery (Day 7), animation
  sampling (Day 8). Knight plays Mixamo combat idle at 60 FPS.
- Animator owns one Animation pointer + playback_time_. Skeleton has
  split rest_translation / rest_rotation / rest_scale + joint_names.
  Mesh loads skin via cgltf with attr.index==0 gating (Day 7 fix).
  FK is order-independent via memoised recursion (Day 8 fix). Both
  fixes critical to remember on Cultist/Wraith/Sovereign in Week 4.

**Day 9 work (per roadmap):**
- Refactor Animator to support current + target animation with 0.2s
  crossfade. Per-channel blending: lerp(t), slerp(r), lerp(s) — NOT
  matrix lerp
- Build AnimationStateMachine: enum + transitions table, owns timers
  for startup / active / recovery / combo-window phases
- Hook keyboard input through Window's InputFrame into the state
  machine, drive animation selection
- Strip root motion from locomotion + combat animations (zero root
  bone translation each frame). roll_forward keeps root motion
- Frame data per GDD §6 (light 0.30/0.10/0.40, combo window 0.5s
  from recovery start; roll 0.5s with i-frames 0.10–0.35s, 4m
  forward, forward-only)

**Files the next instance will need on turn 1:**
- src/anim/Animator.h, Animator.cpp — the refactor target; owns one
  animation pointer today, needs to become current + target + blend_t_
- src/anim/Animation.h, Animation.cpp — channel sampling reference
- src/render/Renderer.h, Renderer.cpp — where idle_animation_ lives
  today; will need a map or array of loaded animations keyed by state
- src/core/Window.h, Window.cpp — input is consumed via InputFrame;
  state machine will read it
- src/main.cpp — where input feeds the camera; state machine plumbing
  goes nearby
- assets/characters/knight/anims/ — `ls` it first to confirm exact
  filenames before hardcoding paths. Pipeline doc has canonical names
  but on-disk names may differ from Day 6 download

**Watch for:**
- skin->joints is non-topological — Day 8 solved this with memoised
  recursive FK in Animator::compute_bone_palette. Do NOT revert to a
  forward pass even if it "looks" sorted; Mixamo doesn't guarantee
  order. Same bug will appear when loading Cultist/Wraith/Sovereign
  in Week 4
- FBX2glTF emits JOINTS_1/WEIGHTS_1 (zero-filled fallback set) alongside
  the real JOINTS_0/WEIGHTS_0. Day 7's fix gates attribute reading on
  attr.index == 0 — don't touch that path without preserving it
- Mixamo animations include root translation. Strip it for everything
  except roll_forward — the player owns position for locomotion and
  combat, but the animation owns the 4m forward distance for the roll
- Crossfade math: blend per-channel TRS, not composed matrices. Linear
  blending of mat4s breaks under rotation — sword-tip ends up halfway
  through the body during transitions
- State machine transitions need REAL frame data per GDD §6, not snap-
  on-input. The combo window is the heart of Phase 2's feel. Make it
  a state machine with timers, not a flat enum
- The 1m bind-pose / checker texture / backface bleed bugs are carried
  forward — NOT Day 9 work. They have their own fix windows

## Day 9 — 2026-05-23 — Crossfade + player state machine

**Shipped:**
- src/anim/Animator refactored: current + target Animation pointers,
  0.2s crossfade with per-channel TRS blend (lerp T/S, slerp R), per-
  anim playback_speed and strip_root_motion knobs. Old set_animation
  retained as snap-no-blend wrapper for back-compat. Memoised
  recursive FK preserved from Day 8
- src/game/PlayerState.{h,cpp} — PlayerStateId enum (Idle/Walk/Jog/
  Attack1-3/Roll), AttackPhase enum, PlayerInput struct. FSM owns
  state_time_, attack_buffer_, state_changed_; transitions emit
  state_changed_ for one frame so Renderer can trigger crossfade
- Frame data per GDD §6: light atk 0.30/0.10/0.40, combo window
  0.50s from recovery start (0.10s grace past recovery end), roll
  0.50s with i-frames 0.10–0.35s exposed as iframes_active() bool
- Input buffering: LMB during startup/active is buffered, consumed
  at recovery start to chain Attack1->Attack2->Attack3. Attack3 is
  terminal (no chain). Dodge during attack recovery cancels into
  Roll. Roll cannot be cancelled (per GDD §6)
- Window: added lmb_pressed / space_pressed edge fields plus
  shift_held. SDL3 KEY_DOWN gates on !event.key.repeat so the OS
  key-repeat doesn't queue a stream of rolls. LMB is naturally
  one-event-per-click (no repeat)
- Renderer: const PlayerState& draw_frame arg. Owns a 7-slot
  AnimSlot array indexed by PlayerStateId — slot carries Animation
  + speed + strip_root. switch_to(id, fade) calls
  animator_.play(slot.anim, fade, slot.speed, slot.strip_root).
  Triggered each frame on player.state_changed()
- WASD camera pan retired from main.cpp. WASD now exclusively
  feeds the SM. Camera is RMB-orbit + scroll only. Cleaner
  separation; less to migrate when Day 11 lands the Player class
  + camera-follow
- CMakeLists patched to include src/game/PlayerState.cpp
- All test sequences pass: W/Shift+W locomotion crossfades, LMB
  combo chain (slash -> slash_v2 -> slash_v3), LMB buffering
  during startup/active, Space roll, roll-cancel from attack
  recovery. roll_forward.glb (1.267s native) auto-scaled to
  2.533x to hit GDD §6 0.5s target

**Broken / pending:**
- Attack anim durations vs SM phase budgets are mismatched:
  slash.glb 1.500s / slash_v2.glb 3.500s / slash_v3.glb 1.567s
  vs the SM's 0.80s attack budget. slash_v2 at 23% completion
  before transition is visibly cut. slash_v2 also matches idle.glb
  exactly at 3.500s — possible mis-tagged file in the pack;
  preview in donmccurdy on Day 10 before deciding to compress
  (4.4x on slash_v2 looks fast) or swap for attack_v*/slash_v4-5
- Mid-blend play() restarts from current_ alone, not the live
  blended pose. 0.2s windows + sane input makes pops rare;
  visible during chain-spam if a reviewer hammers LMB during
  the 0.2s window. Snapshot-as-current would fix; deferred
- Knight drifts 4m forward on roll while camera stays at world
  origin — visually leaves the frame. Day 11's camera-follow
  fixes this naturally
- 1m bind-pose carried from Day 7
- Checker texture instead of knight diffuse, carried from Day 7
- Backface bleed on legs, carried from Day 7
- Stale /usr/local/share/vulkan duplicate-layer warnings carry
  since Day 1

**Notes for tomorrow:**
- Day 10: Phase 2 checkpoint = polish + ImGui debug overlay +
  60-second retrospective clip
- Vendor Dear ImGui via FetchContent (github.com/ocornut/imgui),
  vulkan + sdl3 backends. v1.91+ has clean SDL3 support
- Overlay: current PlayerStateId, AttackPhase, state_time_,
  attack_buffer_, animator playback_time + is_blending, FPS
  rolling average, iframes_active dot
- Inspect attack animations in donmccurdy viewer: preview slash /
  slash_v2-v5 and attack / attack_v2-v4, pick three for a
  left-right-finisher arc ~0.8s each (or compress via
  target_duration in the AnimSlot load). slash_v2 at 3.500s is
  the obvious first target — likely a mis-tagged or
  bake-in-recovery clip
- Animation polish pass: fix any loop jumps, smooth out jittery
  transitions, confirm walk/jog cycles loop without snap
- Record 60-second Phase 2 clip cycling Idle -> Walk -> Jog ->
  Combo -> Roll -> Idle. QuickTime screen record at 60fps
- Phase 2 GATE: knight runs five animations smoothly, SM handles
  transitions, ImGui overlay works, FPS holds 60+. If not green,
  weekend is for fixing

## Handoff to Day 10 instance

**State at end of Day 9:**
- Phase 2 mechanism complete: skinning (Day 7), sampling (Day 8),
  crossfade + FSM (Day 9). Knight responds to keyboard across all
  7 states with 0.2s per-channel TRS blending
- Renderer takes const PlayerState&. PlayerStateId is the shared
  contract between game/PlayerState and render/Renderer's
  AnimSlot array. Adding a new state requires adding a slot
- Animator's old set_animation(a) is the back-compat snap-no-blend
  wrapper around play(a, 0, 1, true) — usable as a shortcut
- Stand-to-Roll's native 1.267s -> 0.5s GDD target pattern via
  per-anim speed scaling at load is reusable for any anim that
  needs to fit a custom budget (e.g. attack slots on Day 10)
- WASD camera pan retired from main.cpp; WASD feeds the SM
  exclusively. Camera is RMB-orbit + scroll only

**Day 10 work (per roadmap Phase 2 checkpoint):**
- FetchContent ImGui into third_party, init Vulkan + SDL3 backends
- ImGui needs its own VkDescriptorPool sized for ImGui_ImplVulkan
  (combined image samplers, ~1k descriptors is plenty)
- Refactor Window::poll_events to give ImGui first dibs on SDL
  events: easiest path is an event-callback parameter, e.g.
  window.poll_events([](const SDL_Event& e){
      ImGui_ImplSDL3_ProcessEvent(&e); })
- Per-frame: ImGui_ImplVulkan_NewFrame, ImGui_ImplSDL3_NewFrame,
  ImGui::NewFrame, immediate-mode draws, ImGui::Render,
  ImGui_ImplVulkan_RenderDrawData in record_command_buffer
  AFTER the main mesh draw and BEFORE vkCmdEndRenderPass
- Debug window contents per "Notes for tomorrow"
- Fix attack-anim mismatch: preview, swap or compress to ~0.8s each
- Record the 60-sec clip

**Files the next instance will need on turn 1:**
- src/anim/Animator.{h,cpp} — current shape, in case ImGui
  surface needs to query is_blending or playback_time
- src/game/PlayerState.{h,cpp} — overlay reads id/phase/state_time
- src/render/Renderer.{h,cpp} — the ImGui integration point
- src/core/Window.{h,cpp} — event-callback refactor lands here
- src/main.cpp — overall loop, where ImGui::NewFrame/EndFrame
  bookend each tick
- CMakeLists.txt — FetchContent block for ImGui + new sources

**Watch for:**
- ImGui Vulkan backend wants its own descriptor pool, separate
  from Renderer's main pool. Don't try to share
- ImGui_ImplSDL3_ProcessEvent must see events BEFORE the Window
  switch consumes them, or text input + capture-want flags
  won't work. Refactor poll_events to a callback, or process
  events from main.cpp directly
- ImGui draws should always run at timeScale=1.0; don't apply
  Phase 3+ hitstop/parry slow-mo to the overlay
- slash_v2.glb at 3.500s native (identical to idle.glb at 3.500s)
  may be a mis-tagged file in the Mixamo pack — preview before
  trusting. attack_v* / slash_v4-5 are fallback candidates from
  the on-disk pack
- Compressing >4x via playback_speed produces visibly snappy
  joints; better to find a shorter source clip than to over-
  compress
- 1m bind-pose carried bug — fix in Blender re-upload before
  Week 4 so Cultist/Wraith/Sovereign don't all come out short
- Mid-blend play() restart-from-current pop is mostly invisible
  but worth a snapshot-as-current fix if reviewers hammer input

## Day 10 — 2026-05-23 — ImGui overlay + Phase 2 close-out

**Shipped:**
- ImGui v1.91.5 vendored via FetchContent (cmake configures it as
  sources compiled into the vigil target, no upstream CMakeLists).
  Vulkan + SDL3 backends initialised cleanly on MoltenVK / M3
- Window::poll_events refactored to take an optional
  std::function<void(const SDL_Event&)> so ImGui_ImplSDL3_ProcessEvent
  sees raw events BEFORE Window's switch consumes them. Existing
  callers that pass nothing keep working — the callback's default
  is empty
- Renderer owns the entire ImGui lifecycle: dedicated 1000-slot
  combined-image-sampler descriptor pool (separate from the main
  pool — Khronos sample recipe), init in ctor, shutdown in dtor
- ImGui_ImplVulkan_RenderDrawData layered inside
  record_command_buffer after vkCmdDrawIndexed and BEFORE
  vkCmdEndRenderPass, so the overlay draws into the same render
  pass as the mesh. main.cpp calls ImGui::Render() before
  renderer.draw_frame() so the draw data is finalised by the time
  recording starts
- Debug overlay (top-left, ~78% opacity, no-decoration, no-nav):
  FPS rolling 60-frame avg + dt(ms), State, AttackPhase,
  state_time, attack_buf YES/-, i-frames red dot, anim.t,
  blending bool + blend_t ProgressBar during active crossfade
- Animator::blend_t() exposed so the overlay can visualise the
  0.2s crossfade ramp filling left-to-right
- PlayerState::attack_buffered() accessor +
  to_string(PlayerStateId) / to_string(AttackPhase) free functions
  in PlayerState for overlay label rendering
- Attack-anim audit via inspect_attack_candidates() — loads every
  slash_v*/attack_v* glb on disk at startup and printf's native
  duration. Confirmed slash_v2 @ 3.500s == idle @ 3.500s exactly
  (the smoking gun for mis-tagged Mixamo export)
- Combo set locked: slash.glb (1.500s -> 1.875x), slash_v5.glb
  (1.367s -> 1.708x, swapped in for slash_v2), slash_v3.glb
  (1.567s -> 1.958x). All under 2x compression, none in Day-9's
  ">4x looks jittery" zone
- target_duration = 0.80s applied to all three attack slots so
  any future filename swap stays budget-correct without touching
  the SM phase data (GDD §6: 0.30 startup + 0.10 active + 0.40
  recovery = 0.80)
- Phase 2 retrospective clip captured to
  marketing/day10_phase_checkpoint.mp4 — 60s, overlay visible
  in corner, full state cycle (Idle->Walk->Jog->combo->Roll->Idle)

**Crash bisected + fixed:**
- First run after ImGui integration: SIGSEGV at pc=0x0 inside
  ImGui_ImplVulkan_Init+200. Cause: CMakeLists had
  `target_compile_definitions(... IMGUI_IMPL_VULKAN_NO_PROTOTYPES=0)`
  — the backend checks `#if defined(...)`, not the value, so =0
  still flips it to loader mode where every vk* function pointer
  stays NULL until ImGui_ImplVulkan_LoadFunctions() runs.
  Fix: removed the define entirely; find_package(Vulkan) +
  Vulkan::Vulkan provides direct prototypes which is the
  backend's default mode. Re-run = clean init

**Broken / pending:**
- 1m bind-pose carried from Day 7 — Blender re-upload before
  Week 4 so Cultist/Wraith/Sovereign don't inherit the scale
- Checker texture instead of knight diffuse, carried from Day 7
- Backface bleed on legs, carried from Day 7
- Mid-blend play() restart-from-current pop, carried from Day 9
- inspect_attack_candidates() still called unconditionally at
  startup; remove once we're sure no Week-4 character will want
  a re-audit
- Stale /usr/local/share/vulkan duplicate-layer warnings,
  carry since Day 1
- Knight at world origin, camera at world origin too — visible
  drift on Roll (knight leaves frame). Day 11's Player class +
  follow camera fixes by construction

**Phase 2 GATE — ALL GREEN:**
- [x] Knight runs seven animations smoothly
      (Idle, Walk, Jog, Attack1, Attack2, Attack3, Roll)
- [x] State machine handles all transitions with 0.2s crossfade
- [x] ImGui debug overlay shows live SM + animator state
- [x] FPS holds 60+ on M3 Air at 1440x900 (60.2 avg, 16.6 ms)
- [x] 60s portfolio-grade clip in marketing/

**Notes for tomorrow:**
- Day 11: Phase 3 (Combat Core) opens. Per roadmap, the day's
  deliverable is a Player class with WASD movement + third-person
  follow camera + mouse-look. End-of-day demo: knight walks
  around a flat plane, anim transitions Idle <-> Walk <-> Jog
  correctly
- Create src/game/Player.{h,cpp}. Owns position vec3, facing yaw,
  velocity vec3, and the PlayerState (move ownership up from
  main.cpp). update(dt, InputFrame, camera) drives the SM and
  applies camera-relative locomotion
- WASD camera-relative: input vector rotated by camera yaw into
  world space. Use Camera::forward_xz()/right_xz() — Phase 1
  already got the rotation correct, don't re-derive
- Walk 3.0 m/s, sprint 6.0 m/s (Shift). Track stamina intent but
  don't drain yet — Day 13 lands the drain
- Camera flips from RMB-orbit-at-origin to always-on follow at
  offset ~(2.2m behind, 5.0m above) the player, mouse-look
  yaw/pitch with SDL_SetWindowRelativeMouseMode(true) for the
  session. Scroll-zoom optional; Esc still quits
- Renderer::draw_frame gains a world-transform input (replaces
  the model = identity in record_command_buffer)
- Roll's 4m root motion will now read on-screen correctly because
  the camera follows. But the Player must also commit position
  from the anim's root each frame during Roll, or the next state
  snaps back. Easiest path: Player reads animator root joint
  during Roll and writes its own position to match
- Phase 2's ImGui overlay is now permanent infrastructure. Extend
  it for Phase 3: add player position, velocity magnitude,
  facing yaw, stamina/HP/Faith bars (placeholders for now)

## Handoff to Day 11 instance

**State at end of Day 10 / Phase 2:**
- Phase 2 COMPLETE. All checkpoint gates green. 60s portfolio
  clip in marketing/day10_phase_checkpoint.mp4
- ImGui v1.91.5 fully integrated. Backend uses direct prototypes
  (NO IMGUI_IMPL_VULKAN_NO_PROTOTYPES define — that was the bug
  that crashed Init at NULL fp on Day 10; do not re-add)
- Window::poll_events takes std::function<void(const SDL_Event&)>.
  main.cpp wires ImGui_ImplSDL3_ProcessEvent through it
- Renderer owns ImGui lifecycle. Separate descriptor pool
  (imgui_descriptor_pool_) from the main one; do NOT share
- ImGui::Render() must run BEFORE renderer.draw_frame() — main.cpp
  does this. RenderDrawData fires inside record_command_buffer
  after the mesh draw, before vkCmdEndRenderPass
- Attack combo locked: slash / slash_v5 / slash_v3, all targeting
  0.80s. inspect_attack_candidates() is debug helper; remove its
  call in Renderer ctor when ready
- Knight + camera both at world origin. Day 11 introduces a
  Player position + follow camera by construction

**Day 11 work (per roadmap Phase 3 opening):**
- src/game/Player.{h,cpp}: position (vec3), facing (yaw float),
  velocity (vec3). update(dt, InputFrame, Camera) — input from
  Window, camera supplies forward_xz/right_xz for camera-relative
  WASD. Player owns PlayerState; main.cpp shifts from holding
  PlayerState directly to holding Player
- Walk 3.0 m/s, sprint 6.0 m/s. Locomotion direction = WASD
  vector rotated into world space via camera yaw. Player yaw
  slerps to face movement direction (try 0.15s smoothing)
- Camera mode flip: RMB-orbit -> always-on follow. Offset
  (5.0m above, 2.2m behind) the player; LookAt the player +0.5m
  head height. Mouse-look replaces RMB orbit, cursor locked for
  the session via SDL_SetWindowRelativeMouseMode(true). Scroll
  zoom may stay as debug
- Renderer::draw_frame gains a world-transform arg (or pulls
  from Player). Replaces model = identity in record_command_buffer
- Roll: 4m root motion finally visible thanks to camera-follow.
  Player commits position from animator root each frame during
  Roll so the post-Roll state doesn't snap back

**Files the next instance will need on turn 1:**
- src/core/Camera.{h,cpp} — refactor from orbit to follow-mode
- src/core/Window.{h,cpp} — confirm InputFrame has what's needed;
  may want SDL_EVENT_WINDOW_FOCUS_LOST handling to release cursor
- src/game/PlayerState.{h,cpp} — owned by new Player class;
  SM logic unchanged
- src/render/Renderer.{h,cpp} — draw_frame signature gains a
  world transform
- src/main.cpp — loop calls player.update() instead of
  player_state.update() directly

**Watch for:**
- Cursor lock: relative mouse mode is currently RMB-gated. Day 11
  makes it always-on during gameplay. Cmd-Tab away and back
  should not leave cursor stuck; SDL_EVENT_WINDOW_FOCUS_LOST is
  the hook (release lock on focus loss, re-acquire on regain)
- Camera-relative WASD: the Phase 1 "yaw + pi" bug from the HTML
  prototype lives here too. forward_xz/right_xz are already
  signed correctly; use them, don't re-derive trig
- Facing slerp: Mixamo idle and walk face -Z by default. Yaw=0
  should match. Test strafe (A or D held alone): knight should
  turn to face strafe direction over ~0.15s
- Root motion strip: locomotion is strip_root=true (Player owns
  position). Roll is strip_root=false (root motion is the 4m).
  During Roll the Player must read animator root EACH FRAME and
  commit to its own position; otherwise the post-Roll frame
  snaps back to pre-Roll position
- inspect_attack_candidates() bloats startup by ~150ms parsing
  glTFs we don't use. Acceptable for Day 10; remove during Day 11
  cleanup

## Day 11 — 2026-05-24 — Player class + follow camera + Phase 3 opens

**Shipped:**
- src/game/Player.{h,cpp}: position vec3, velocity vec3, yaw_facing
  float, owns PlayerState (ownership migrated up from main.cpp).
  update(dt, InputFrame, Camera) drives the SM, camera-relative
  locomotion, Roll code-motion, and facing slerp
- Walk 3.0 m/s, sprint 6.0 m/s per GDD §6. Sprint intent forwarded
  to PlayerInput but no stamina drain yet (Day 13)
- Roll 8.0 m/s × 0.5s = 4m in latched direction. Direction = wish_dir
  at roll entry if held, else current facing. GDD v1.3 §6 #11
  forward-only constraint satisfied — Mixamo only ships
  sword-and-shield forward-roll for this skeleton
- Roll design call (deviated from Day 10 handoff): Roll AnimSlot
  flipped to strip_root=true; the 4m is code-driven via velocity
  integration, same path as Walk/Jog. Snap-back on Roll exit gone
  by construction (palette carries no translation)
- src/core/Camera retargeted to follow mode. Defaults: yaw=0,
  pitch=20°, distance=4.0m, FoV=55°, sensitivity=0.0025 rad/px.
  min_pitch raised to -5° so camera never inverts. target_ pinned
  to player.pos + (0, 0.5, 0) each frame in main.cpp
- src/core/Window: SDL_SetWindowRelativeMouseMode on at construction
  for the whole session, RMB gate dropped. SDL_EVENT_WINDOW_FOCUS_
  LOST/GAINED release+regain so Cmd-Tab doesn't strand the cursor
- src/render/Renderer: draw_frame + draw_debug_overlay take
  const Player&. record_command_buffer takes world_transform arg;
  replaces the identity matrix in MVP. The Day-10 param rename to
  'p' collided with the local 'const ImVec2 p' for the i-frames dot
  — landed on 'player_obj' to dodge
- Overlay extended with pos / |vel| / yaw under the i-frames dot

**WASD convention — ended at the non-textbook version:**
- W -= fwd, S += fwd, D -= right, A += right. Camera-relative
  original was the inverse; user reported it reversed on first run
  so the signs got flipped and stayed
- Tried Skyrim-style (body tracks camera.yaw, no rotation on strafe)
  as a deeper-hypothesis fix; user preferred the simpler sign flip
- Likely explanation: Tripo→Mixamo knight may face +Z by default
  (toward camera) rather than -Z. Day-10 handoff asserted -Z but
  was untested because Day 9-10 didn't visibly move the knight.
  With a +Z-facing model, the textbook convention reads as
  "reversed." Diagnose for real when the Week-4 Blender re-upload
  gives us a textured knight with unambiguous face/back

**Broken / pending:**
- 1m bind-pose, checker texture, backface bleed — Day 7 carry,
  Week 4 Blender re-upload
- Mid-blend pose-pop on chain-spam — Day 9 carry, deferred
- inspect_attack_candidates() still called in Renderer ctor,
  ~150ms startup. Remove after Day-13 attack work settles
- Stale /usr/local/share/vulkan dup-layer warnings, Day 1 carry
- WASD sign convention deviates from third-person standard;
  diagnose model facing direction in Week 4
- Body rotation still tied to wish_dir (rotates knight to face
  strafe direction). User chose this over camera-yaw-tracking.
  Reconsider when locomotion blend-tree + strafe anims land

**Day 11 deliverable per roadmap: GREEN.**
- Knight moves around a flat plane with WASD + mouse-look
- Idle ↔ Walk ↔ Jog crossfades clean
- Attack combo unchanged from Day 10
- Roll 4m reads on-screen, camera follows, no snap-back
- Cmd-Tab away and back recovers cursor

**Notes for tomorrow:**
- Day 12 per roadmap: light attack + combo system + practice dummy
  + console hit detection
- Combo SM + animations already done (Day 9 + Day 10). Remaining
  work: place a static practice dummy, write cone-overlap hit test
  during Attack1/2/3 active frames, print "HIT" once per landing
- Dummy mesh: procedural unit cube OR a single-cube .glb. Skinned
  shader expects a bone palette UBO — non-skinned dummy needs
  either a fake identity palette (cheapest), a shader branch, or
  a second pipeline. Day 12 picks the fast path
- Hit detection: 90° cone (45° half-angle) centered on Player::yaw(),
  2.0m reach (GDD §6), XZ only. One-shot per attack via a
  has_landed_ flag on PlayerState reset on transition_to()
- Cone direction may need a sign correction matching the WASD flip
  — if hits register on the wrong side of the knight, flip the
  cone-center vector and log it

## Handoff to Day 12 instance

**State at end of Day 11:**
- Player class owns position, velocity, yaw_facing_, and the SM.
  main.cpp holds Player; Renderer extracts player.state() and
  player.world_transform()
- Follow camera at offset derived from yaw/pitch/distance.
  Defaults: yaw 0, pitch 20°, distance 4.0m, FoV 55°,
  sensitivity 0.0025 rad/px. target = player.position + (0,0.5,0)
- Mouse-look on for the entire session, cursor releases on
  focus loss / regains on focus gain
- WASD: W -= fwd, S += fwd, D -= right, A += right. Departed
  from camera-relative-original — see Day 11 entry for context
- Roll: 8 m/s × 0.5s code-driven, AnimSlot strip_root=true
- Day-9 attack SM intact: 3-hit combo with 0.30/0.10/0.40 phase
  data, input buffering, dodge-cancel from recovery
- Overlay shows pos / |vel| / yaw under the i-frames dot

**Day 12 work (per roadmap Phase 3):**
- Place stationary practice dummy in the world, e.g. at (3, 0, -2).
  Procedural unit cube in Mesh OR a tiny one-mesh .glb. Skinned
  pipeline expects bone palette — cheapest path is to bind an
  identity palette per dummy draw and reuse the existing pipeline
- Hit detection during Attack1/2/3 with AttackPhase::Active:
  90° cone, 45° half-angle, centered on Player::yaw(), 2.0m reach,
  XZ only. On first hit per attack instance print "[HIT]" and set
  a has_landed_ flag on PlayerState; reset on transition_to() into
  an attack state. 0.10s active × 60Hz = 6 frames, must fire once
- Console output is sufficient for Day 12. Visual hit reaction
  (white flash, damage number, hitstop) is Phase 6

**Files the next instance will need on turn 1:**
- src/render/Renderer.{h,cpp} — add second draw call for the dummy
- src/render/Mesh.{h,cpp} — existing load_from_glb surface; may
  want load_unit_cube() helper
- src/game/Player.{h,cpp} — yaw() accessor for cone direction
- src/game/PlayerState.{h,cpp} — has_landed_ flag + reset hook
- src/main.cpp — instantiate the dummy + drive hit detection

**Watch for:**
- One-shot per attack: 6 active frames at 60Hz. Store has_landed_
  on PlayerState, reset only on attack-state entry
- Cone direction sign: if hits register on the wrong side of the
  knight, the same +Z-facing-model hypothesis from Day 11 needs a
  flip. Log angle(knight_facing, dummy_vector) during bring-up
- Non-skinned dummy: identity bone palette is the smallest diff;
  clean alternative is a second pipeline + non-skinned vertex
  shader. Pick the fast one for Day 12
- Don't get pulled into: model-facing diagnosis (Week 4 work),
  strafe anims (Phase 4+), heavy attack (Day 13), Renderer's
  inspect_attack_candidates() cleanup

**Phase 3 plan reminder:**
- Day 12: practice dummy + hit detection (combo system done Day 9)
- Day 13: heavy attack + block + stamina drain
- Day 14: parry + riposte (i-frames already exposed via
  PlayerState::iframes_active())
- Day 15: holy bolt + faith resource + Phase 3 checkpoint

## Day 12 — 2026-05-24 — Practice dummy, hit detection, controls

**Shipped:**
- src/render/Mesh: Mesh::make_unit_cube factory. 8-vertex cube,
  half-extent 0.5m. Vertex JOINTS_0=(127,0,0,0), WEIGHTS_0=(1,0,0,0)
  target reserved palette slot 127, which the per-frame std::fill
  in draw_frame leaves at identity (now annotated as a contract).
  Single extra drawIndexed, no new pipeline / descriptor set / shader
- src/render/Renderer: dummy_mesh_ built in ctor. DUMMY_POSITION
  (3,0,-2) as inline static constexpr in the public interface.
  Second drawIndexed in record_command_buffer between knight and ImGui
- src/game/PlayerState: attack_landed_ flag + accessor +
  mark_attack_landed mutator. Reset on every transition_to so
  Attack1/2/3 each get one shot. 0.10s Active * 60Hz = 6 frames;
  without the latch [HIT] would print 6x per swing
- src/game/Player: register_hit() forwards to
  state_.mark_attack_landed. SM internals stay private to Player
- src/main.cpp: cone hit detection between player.update and ImGui
  frame. 90 deg (45 half), 2.0m reach, XZ only. Gated on
  AttackPhase::Active && !attack_landed(). Prints [HIT] <state>
  dist=Xm angle=Ydeg on landing

**Control-convention fixes the dummy surfaced:**
- src/game/Player.cpp: WASD signs reverted to textbook camera-relative
  (W += fwd, A -= right, etc). Day 11's flip was a first-run "looks
  reversed" call made without a fixed reference object; the dummy at
  (3,0,-2) showed pos.z went +ve on W = moving toward camera
- Model rest pose faces +Z, NOT Mixamo's -Z (Day 11 hypothesis
  confirmed). yaw solver is atan2(face_dir.x, face_dir.z); roll
  fallback and hit-test cone facing both use (sin yaw, 0, cos yaw).
  Re-test if Week 4 Blender re-upload corrects orientation in-asset
- src/core/Camera.cpp: orbit dx sign flipped. Mouse RIGHT now rotates
  view RIGHT (Souls / Skyrim convention). dy left positive

**Broken / pending:**
- 1m bind-pose / checker tex / backface bleed - Day 7 carry, Week 4
- Mid-blend pose-pop on chain-spam - Day 9 carry, deferred
- inspect_attack_candidates() still called in Renderer ctor, ~150ms
  startup. Removal triggers Day 13 once attack set is locked
- Body rotation tied to wish_dir (Day 11 design call) - S press
  pivots knight 180 to face camera. Stylised, not buggy; revisit
  when strafe anims land Phase 4
- Stale /usr/local/share/vulkan dup-layer warnings, Day 1 carry

**Notes for tomorrow:**
- Day 13 per roadmap: heavy attack + block + stamina
- Heavy = Shift+LMB, single hit, GDD 6 0.55/0.15/0.70, costs 24, no
  combo, can roll-cancel during recovery
- Block = hold RMB, locks position, drains stamina on hit (no hits
  until Phase 4). Day 14 will layer parry edge-detection on the same
  held state
- Stamina = new class on Player. Max 100, regen 25/s except
  attack/block/sprint, costs 12/24/25, sprint drain 30/s
- Gate-at-cost (12/24/25) instead of roadmap's literal "<10 for
  attack" which would allow negative stamina


## Day 13 — 2026-05-24 — Heavy, block, stamina

**Shipped:**
- src/game/Stamina: header-only resource. Constants per GDD 6/7
  (MAX_VALUE 100, REGEN_RATE 25, SPRINT_DRAIN 30, costs 12/24/25).
  available(cost) gate, drain(amount), drain_rate(rate,dt),
  regen_rate(rate,dt). Day 14 Faith will mirror this layout
- src/game/PlayerState: enum grew Heavy=7, Block=8, Count=9.
  Heavy reuses Attack1/2/3's phase machine via update_attack with
  is_heavy selector (0.55/0.15/0.70 vs 0.30/0.10/0.40, combo chain
  bypassed). Block is held state, no internal timer, exits only on
  RMB release. PlayerInput grew heavy_attack and block fields
- src/game/Player: PlayerInput build splits Shift+LMB to heavy_attack,
  gates every action through stamina.available(cost). pin.sprint
  drops false the tick stamina hits 0; SM drops Jog -> Walk next
  tick. Drain on entry latched off state_changed(). Tick at end of
  update applies regen / sprint drain
- src/render/Renderer: Heavy slot loads attack_v3.glb compressed to
  1.40s (1.24x). Block slot loads block_idle.glb at native speed.
  Debug overlay grew green stamina bar between i-frames dot and
  pos/vel/yaw (GDD 15.2 spec)
- src/render/Renderer: inspect_attack_candidates() removed (Day 10
  audit, attack set locked Day 13). ~150ms startup recovered.
  Declaration also stripped from Renderer.h

**Broken / pending:**
- 1m bind-pose / checker tex / backface bleed - Day 7 carry, Week 4
- Mid-blend pose-pop - Day 9 carry, deferred
- Body rotation tied to wish_dir - Day 11 design call, defer to
  Phase 4 strafe anims
- Stale /usr/local/share/vulkan dup-layer warnings, Day 1 carry
- Block stance-locks all movement (GDD doesn't require this; could
  allow slow walk in block when it feels right)
- Roll allows regen during the 0.5s i-frame window (GDD-literal:
  regen-except-attack/block/sprint, roll not listed). Net effect:
  rolling claws back ~12.5 of the 25 cost. Generous; revisit if it
  trivialises sustained combat

**Notes for tomorrow:**
- Day 14 per roadmap: parry + riposte. Roll + i-frames shipped Day 9;
  iframes_active() is the damage-system contract
- Parry: tap RMB (edge, not held) opens 0.15s parry window. If
  incoming hit during the window -> parry -> Riposte. Else if in
  Block -> block damage (drain 50% of damage in stamina). Else ->
  take full damage
- Riposte: new PlayerStateId. Pull Mixamo "Standing Melee Combat
  Attack", 1.5s budget, 60 dmg, terminal (no chain, NOT
  roll-cancellable). attack.glb at 2.33s -> 1.55x to 1.5s is a fine
  fallback if exact filename not present
- Window needs pending_rmb_press_ (edge mirror of rmb_down_) and
  pending_n_press_ (debug fake-hit trigger)
- N key fires a fake incoming attack at the knight 0.5s after press
  (no enemies yet - parry has nothing to react to without it)
- Visual hit reaction (white flash, damage number, hitstop) is
  Phase 6. Day 14 console-prints [PARRY], [BLOCKED], [DAMAGED]


## Handoff to Day 14 instance

**State at end of Day 13:**
- Player owns Stamina (header-only). All actions gated through
  available(cost); drain on state_changed entry; tick at end of
  update for regen / sprint drain
- PlayerStateId: Idle/Walk/Jog/Attack1/Attack2/Attack3/Roll/Heavy/
  Block/Count=9. anim_slots_ array auto-sized
- PlayerInput: move_forward/sprint/attack/heavy_attack/dodge/block.
  attack and heavy_attack are mutex on shift_held
- Heavy: Shift+LMB, single hit, attack_v3.glb at 1.24x, can
  roll-cancel during recovery, no combo
- Block: hold RMB, position locked, facing locked, no regen during
  block. Exits only on RMB release
- Practice dummy at (3,0,-2) - vigil::Renderer::DUMMY_POSITION
- Hit detection: main.cpp, 90 deg cone, 2.0m, AttackPhase::Active +
  attack_landed_ gated. Prints [HIT] <state> dist angle
- WASD textbook camera-relative. Model rest = +Z forward at yaw=0.
  Mouse RIGHT turns view RIGHT (Souls convention)

**Day 14 work (per roadmap Phase 3):**
- New PlayerStateId::Riposte. 1.5s budget, single hit, terminal -
  not roll-cancellable, not combo-chainable. Pull "Standing Melee
  Combat Attack" (or attack.glb at 2.33s -> 1.55x as fallback)
- Parry edge detection. parry_window_remaining_ float on Player or
  PlayerState. RMB edge press (not held) sets to 0.15s. Decrement
  each tick. While > 0, an incoming hit upgrades the response from
  "block damage" to "parry -> riposte"
- Window: add pending_rmb_press_ (edge mirror of rmb_down_) and
  pending_n_press_ (debug fake-hit trigger)
- Fake incoming attack on N key. incoming_hit_time_ float in
  main.cpp, set to now+0.5 on N press, fires when reached. On fire:
  parry_window > 0 -> Riposte. Currently in Block (window expired)
  -> drain stamina 50% of damage. Else -> print [DAMAGED]
- Riposte hit detection rides the existing main.cpp cone test.
  Needs AttackPhase::Active during its active frames - wire via
  update_attack's is_heavy/is_riposte split, OR add a is_attacker
  helper that includes Heavy + Riposte

**Files the next instance will need on turn 1:**
- src/game/PlayerState.{h,cpp} - add Riposte, parry window state
- src/game/Player.{h,cpp} - parry window or pass-through hooks
- src/core/Window.{h,cpp} - pending_rmb_press_ + pending_n_press_
- src/render/Renderer.{h,cpp} - Riposte anim slot
- src/main.cpp - N-key fake-hit dispatcher

**Watch for:**
- RMB tap-vs-hold ambiguity. Block triggers on rmb_held (true the
  whole time button is down). Parry triggers on the EDGE press. A
  single RMB press fires BOTH on first frame: edge=true AND
  held=true. Sequence: enter Block, open 0.15s parry window. Hit in
  first 150ms -> parry. Hit after window decays (still held) -> block.
  Matches Souls feel - defensive tap is parry, held wall is block
- Riposte must NOT be roll-cancellable. Mirror Attack3 (terminal)
  in update_attack, OR special-case the Riposte branch
- "Standing Melee Combat Attack" filename: FBX2glTF snake_cases, so
  likely standing_melee_combat_attack.glb. ls grep -i melee to
  confirm. attack_v3.glb is taken by Heavy; attack.glb at 2.33s ->
  1.55x to 1.5s budget is the clean fallback

**Phase 3 plan reminder:**
- Day 12: practice dummy + hit detection (shipped)
- Day 13: heavy + block + stamina (shipped)
- Day 14: parry + riposte (today)
- Day 15: holy bolt + faith resource + Phase 3 checkpoint

## Day 14 — 2026-05-25 — Parry, riposte, fake-hit dispatcher

**Shipped:**
- src/game/PlayerState: Riposte=9, Count=10. Riposte mirrors Heavy in
  update_attack via is_riposte branch with its own frame data
  (0.20/0.20/1.10 = 1.5s). Terminal: no combo chain, NOT roll-cancellable
  (dodge gate clauses on !is_riposte). pin.riposte added to PlayerInput;
  top-priority transition out of Idle/Walk/Jog/Block. Block exits on
  pin.riposte even with RMB still held - the parry consumes the stance
- src/game/Player: parry_window_remaining_ float + riposte_pending_ bool.
  tick_parry_window(dt, input) decays first then refreshes on rmb_pressed
  edge - fresh press gets full 0.15s, not 0.15-dt. try_parry() consumes
  window + sets riposte_pending_, returns bool. absorb_block_hit(dmg)
  drains 50% as stamina (GDD 6). update() reads riposte_pending_ into
  pin.riposte and clears, single-shot. Riposte joins the spending
  state list (no regen during)
- src/core/Window: InputFrame grew rmb_pressed (edge) and n_pressed
  (debug). pending_rmb_press_ set inside BUTTON_DOWN/RIGHT alongside
  rmb_down_; pending_n_press_ on SDLK_N with !repeat. consume_input
  drains both pendings per existing pattern
- src/render/Renderer: Riposte slot loads attack.glb (2.333s native ->
  1.556x to 1.5s). Debug overlay grew a PARRY line in gold, visible
  only while window > 0 - short 150ms burst so the panel stays quiet
- src/main.cpp: per-frame pipeline split - tick_parry_window FIRST,
  then fake-hit timer + damage resolve, then player.update(). Order is
  load-bearing: lets a same-frame RMB press catch an N-pressed-prior
  fake hit firing this frame, with no 1-frame latency to the riposte
  transition. fake_hit_timer is -1 = inactive; latest N press wins
- src/main.cpp: cone hit detection now fires for Riposte automatically
  (shares AttackPhase::Active). [HIT] Riposte prints during the 0.20s
  active window - confirmed in the test log at dist=1.15m

**Broken / pending:**
- 1m bind-pose / checker tex / backface bleed - Day 7 carry, Week 4
- Mid-blend pose-pop - Day 9 carry, deferred
- Body rotation tied to wish_dir - Day 11 design call, defer to Phase 4
- Stale /usr/local/share/vulkan dup-layer warnings, Day 1 carry
- [PARRY] window=0.150s print is always the PARRY_WINDOW constant, not
  the remaining-at-consume value (try_parry zeros the window before
  there is anything to log). Cosmetic; useful tuning data lost. Fix is
  one-liner - either drop the field from the print or return float
  from try_parry. Deferred to Day 15 morning
- Block stance-locks all movement - Day 13 carry
- Roll allows regen during the 0.5s i-frame window - Day 13 carry

**Notes for tomorrow:**
- Day 15 per roadmap: Holy Bolt spell + Faith resource + Phase 3 checkpoint
- Faith: clone of Stamina header layout. MAX_VALUE 60, REGEN_RATE 5/s,
  BOLT_COST 20. No "spending" gate - regens always (GDD 6: 5/s, always).
  Add Faith bar to debug overlay between stamina and pos/vel/yaw (blue
  per GDD 15.2)
- Holy Bolt: Q key (new edge). New PlayerStateId::Cast (10), Count=11.
  0.8s cast, locked in place, interruptible (taking damage cancels +
  refunds 50% Faith). Cast anim slot: casting.glb (Pro Pack already in
  anims/) - native duration TBC, scale to 0.8s budget
- Projectile entity. New src/game/Projectile.{h,cpp} - position,
  velocity, lifetime, alive flag, optional target ref. main.cpp owns
  std::vector<Projectile>. Spawn on cast complete; despawn on target
  hit or lifetime expiry. Travel 18 m/s, weak homing in 30 deg cone
  toward nearest target (just the dummy until Phase 4)
- Projectile mesh: reuse the slot-127 identity trick. Mesh::make_unit_orb
  factory (or scale the cube small) - the cube path already proves the
  pipeline can draw a non-skinned object with the existing descriptor
  set. Tripo for a crystal/orb Holy Bolt mesh is GDD 18.4 work but
  the cube placeholder ships fast
- Projectile hit detection: per-projectile vs dummy AABB inside main.cpp,
  same shape as the existing player cone test. Console-print [BOLT HIT]
  dmg=50 on landing
- Phase 3 checkpoint clip: record 90s of all combat moves - combo, heavy,
  block, parry into riposte, roll w/ i-frames, holy bolt cast + hit.
  Per roadmap Day 15 deliverable: this becomes trailer footage

## Handoff to Day 15 instance

**State at end of Day 14:**
- Player owns parry_window_remaining_ + riposte_pending_. tick_parry_window
  decays then refreshes on rmb_pressed edge. try_parry consumes window
  and sets riposte_pending_. absorb_block_hit drains 50% as stamina.
  update() reads riposte_pending_ into pin.riposte, clears single-shot
- PlayerStateId: Idle/Walk/Jog/Attack1/Attack2/Attack3/Roll/Heavy/Block/
  Riposte/Count=10. Riposte 1.5s budget (0.20/0.20/1.10), terminal,
  attack.glb at 1.556x. anim_slots_ array auto-sized
- PlayerInput: move_forward/sprint/attack/heavy_attack/dodge/block/riposte.
  riposte set by try_parry pipeline, top-priority transition out of
  Idle/Walk/Jog/Block
- Window: InputFrame.rmb_pressed (edge) and .n_pressed (debug).
  pending_rmb_press_ set alongside rmb_down_ in BUTTON_DOWN/RIGHT.
  SDLK_N with !repeat
- main.cpp per-frame pipeline:
  1) tick_parry_window  (window decay + edge refresh)
  2) fake-hit timer tick + on-fire resolve (try_parry / block / damaged)
  3) player.update     (SM consumes riposte_pending_)
  4) cone hit detection (now fires for Riposte too)
- Practice dummy at (3,0,-2) - vigil::Renderer::DUMMY_POSITION
- Stamina shipped Day 13. Faith follows the same header pattern Day 15

**Day 15 work (per roadmap Phase 3):**
- src/game/Faith.h: clone Stamina header. MAX_VALUE 60, REGEN_RATE 5/s
  (GDD 6, always-on regen), BOLT_COST 20. Mirror the available/drain/
  drain_rate/regen_rate API exactly so Player.update() loop reads
  consistent. No spending gate
- src/game/Player: add Faith member + accessor. Cast gates on
  faith_.available(Faith::BOLT_COST), drains on Cast entry, refunds
  50% on cancellation
- src/game/PlayerState: add PlayerStateId::Cast (10), Count=11. 0.8s
  budget, locked in place. Cancellable by incoming damage (Phase 4
  contract); Day 15 just locks in the cast lifecycle. Recovery exits
  to default_locomotion. pin.cast added to PlayerInput; q_pressed edge
- src/core/Window: pending_q_press_ + InputFrame.q_pressed, mirror
  space_pressed pattern
- src/render/Renderer: Cast slot loads casting.glb (verify native, scale
  to 0.8s). Add blue Faith bar to debug overlay under green stamina
- src/game/Projectile.{h,cpp}: new files. position/velocity/lifetime/
  alive struct, update(dt) integrates, optional weak-homing 30 deg
  cone toward nearest target. main.cpp owns std::vector<Projectile>,
  spawns on Cast active frame, ticks each frame, deletes dead.
  Renderer.cpp adds a per-projectile draw loop using dummy_mesh_ +
  per-projectile MVP push (same slot-127 trick)
- src/main.cpp: spawn projectile when Cast hits AttackPhase::Active
  (single-shot per cast via attack_landed_ analog). Per-projectile
  AABB test against DUMMY_POSITION, half-extent 0.5m. [BOLT HIT]
  dmg=50 on landing; despawn projectile
- Phase 3 checkpoint clip: 90s recording of all combat moves

**Files the next instance will need on turn 1:**
- src/game/Stamina.h - template for Faith.h
- src/game/PlayerState.{h,cpp} - add Cast, pin.cast
- src/game/Player.{h,cpp} - add Faith member + projectile spawn hook
- src/core/Window.{h,cpp} - pending_q_press_ + InputFrame.q_pressed
- src/render/Renderer.{h,cpp} - Cast anim slot + faith bar + projectile draw
- src/main.cpp - projectile pool ownership + tick + AABB test

**Watch for:**
- casting.glb already in anims (Day 13 inventory: casting.glb +
  casting_v2.glb). Native duration TBC at load; scale to 0.8s budget
  per GDD 6
- Cast must be cancellable by incoming damage. Day 15 lays the
  contract; Phase 4 enemies wire the cancellation. For Day 15, gate
  the cancel behind a debug hook (N during cast? recycle the fake-hit
  dispatcher) to verify the refund-50% path
- Projectile lifetime cap: ~3s. Without a cap, missed bolts fly forever
- Slot-127 cube trick proves the pipeline can draw non-skinned objects.
  Projectile mesh can ride the same path - reuse dummy_mesh_ or add
  Mesh::make_unit_sphere if the cube looks too geometric. Tripo orb
  is GDD 18.4 work, fine to defer

**Phase 3 plan reminder:**
- Day 12: practice dummy + hit detection (shipped)
- Day 13: heavy + block + stamina (shipped)
- Day 14: parry + riposte (shipped)
- Day 15: holy bolt + faith + Phase 3 checkpoint (today)
