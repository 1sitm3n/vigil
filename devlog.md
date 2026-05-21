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
