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
