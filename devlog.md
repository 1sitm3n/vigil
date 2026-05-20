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
