#include "render/Renderer.h"
#include "render/Swapchain.h"
#include "render/GraphicsPipeline.h"
#include "render/Vertex.h"
#include "core/VulkanContext.h"
#include "core/Camera.h"
#include "core/Window.h"
#include "game/Player.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace vigil {

namespace {
constexpr float kCrossfadeDuration  = 0.2f;  // roadmap Day 9: 0.2s blend.
constexpr float kRollTargetDuration = 0.5f;  // GDD §6: roll lasts 0.5s.
constexpr float kAtkTargetDuration  = 0.8f;  // GDD §6: light atk 0.30+0.10+0.40s.
constexpr float kHeavyTargetDuration = 1.4f; // GDD §6: heavy atk 0.55+0.15+0.70s.
constexpr float kRiposteTargetDuration = 1.5f; // GDD §6: riposte 0.20+0.20+1.10s (Day 14).
}  // namespace

Renderer::Renderer(VulkanContext& vk,
                   const Swapchain& swapchain,
                   const GraphicsPipeline& pipeline,
                   Window& window)
    : vk_(vk), swapchain_(swapchain), pipeline_(pipeline), window_(window) {
    last_frame_time_ = std::chrono::high_resolution_clock::now();
    palette_scratch_.assign(MAX_BONES, glm::mat4(1.0f));

    create_command_pool();
    create_command_buffers();
    create_sync_objects();
    create_mesh();
    dummy_mesh_ = Mesh::make_unit_cube(vk_);  // Day 12 practice dummy
    create_texture();
    create_bone_palette_buffers();
    load_animations();
    create_descriptor_pool();
    create_descriptor_sets();
    init_imgui();
}

Renderer::~Renderer() {
    wait_idle();
    shutdown_imgui();

    for (auto& frame : frames_) {
        if (frame.image_available) vkDestroySemaphore(vk_.device(), frame.image_available, nullptr);
        if (frame.in_flight)       vkDestroyFence(vk_.device(),     frame.in_flight,       nullptr);
    }
    for (auto sem : render_finished_) {
        if (sem) vkDestroySemaphore(vk_.device(), sem, nullptr);
    }
    if (descriptor_pool_)       vkDestroyDescriptorPool(vk_.device(), descriptor_pool_,       nullptr);
    if (imgui_descriptor_pool_) vkDestroyDescriptorPool(vk_.device(), imgui_descriptor_pool_, nullptr);
    if (command_pool_)          vkDestroyCommandPool(vk_.device(), command_pool_, nullptr);
}

void Renderer::wait_idle() {
    vkDeviceWaitIdle(vk_.device());
}

void Renderer::create_command_pool() {
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    info.queueFamilyIndex = vk_.graphics_family();
    if (vkCreateCommandPool(vk_.device(), &info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateCommandPool failed");
    }
}

void Renderer::create_command_buffers() {
    VkCommandBuffer buffers[FRAMES_IN_FLIGHT]{};
    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool        = command_pool_;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = FRAMES_IN_FLIGHT;
    if (vkAllocateCommandBuffers(vk_.device(), &info, buffers) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateCommandBuffers failed");
    }
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        frames_[i].command_buffer = buffers[i];
    }
}

void Renderer::create_sync_objects() {
    VkSemaphoreCreateInfo sem_info{};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame : frames_) {
        if (vkCreateSemaphore(vk_.device(), &sem_info, nullptr, &frame.image_available) != VK_SUCCESS ||
            vkCreateFence(vk_.device(),     &fence_info, nullptr, &frame.in_flight)      != VK_SUCCESS) {
            throw std::runtime_error("Failed to create per-frame sync objects");
        }
    }

    render_finished_.resize(swapchain_.image_count(), VK_NULL_HANDLE);
    for (auto& sem : render_finished_) {
        if (vkCreateSemaphore(vk_.device(), &sem_info, nullptr, &sem) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create per-image render-finished semaphore");
        }
    }
}

void Renderer::create_mesh() {
    mesh_ = Mesh(vk_, "assets/characters/knight/knight.glb");
}

void Renderer::create_texture() {
    diffuse_texture_ = Texture(vk_, command_pool_, "assets/test/checker.png");
}

void Renderer::create_bone_palette_buffers() {
    const VkDeviceSize palette_size = sizeof(glm::mat4) * MAX_BONES;
    const std::vector<glm::mat4> identity_palette(MAX_BONES, glm::mat4(1.0f));

    for (auto& buf : bone_palette_buffers_) {
        buf = Buffer(
            vk_, palette_size,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        buf.upload(identity_palette.data(), palette_size);
    }
}

void Renderer::load_animations() {
    auto load_slot = [&](PlayerStateId id, const char* path,
                         bool strip_root, float target_duration) {
        AnimSlot& s = anim_slots_[static_cast<size_t>(id)];
        s.anim       = load_animation(path, mesh_.skeleton());
        s.strip_root = strip_root;
        // Compress/stretch native duration to fit SM budget. <=0 = native.
        s.speed = (target_duration > 0.0f && s.anim.duration > 0.0f)
                      ? s.anim.duration / target_duration
                      : 1.0f;
        std::printf("[Anim] slot=%d  file=%s  native=%.3fs  speed=%.3fx  strip_root=%d\n",
                    static_cast<int>(id), path, s.anim.duration, s.speed,
                    static_cast<int>(strip_root));
    };

    // GDD-canonical mapping. Locomotion + combat strip root motion (player
    // class owns position from Day 11). Roll keeps root motion — the 4m
    // forward distance from GDD §6 lives in the animation, not in code —
    // and gets its native ~1.27s compressed to the 0.5s spec.
    //
    // Attack slots: Day 10 — compress to the 0.8s GDD budget so the SM phase
    // timeline (0.30 startup + 0.10 active + 0.40 recovery) lines up with
    // the swing. slash_v2.glb at 3.500s native -> 4.375x will look snappy;
    // confirm visually and swap to a shorter clip from the candidate set
    // (see inspect_attack_candidates() console output) if it reads poorly.
    load_slot(PlayerStateId::Idle,    "assets/characters/knight/anims/idle.glb",         true,  0.0f);
    load_slot(PlayerStateId::Walk,    "assets/characters/knight/anims/walk.glb",         true,  0.0f);
    load_slot(PlayerStateId::Jog,     "assets/characters/knight/anims/run.glb",          true,  0.0f);
    load_slot(PlayerStateId::Attack1, "assets/characters/knight/anims/slash.glb",        true,  kAtkTargetDuration);
    load_slot(PlayerStateId::Attack2, "assets/characters/knight/anims/slash_v5.glb",     true,  kAtkTargetDuration);
    load_slot(PlayerStateId::Attack3, "assets/characters/knight/anims/slash_v3.glb",     true,  kAtkTargetDuration);
    load_slot(PlayerStateId::Roll,    "assets/characters/knight/anims/roll_forward.glb", true,  kRollTargetDuration);
    load_slot(PlayerStateId::Heavy,   "assets/characters/knight/anims/attack_v3.glb",    true,  kHeavyTargetDuration);
    load_slot(PlayerStateId::Block,   "assets/characters/knight/anims/block_idle.glb",   true,  0.0f);
    load_slot(PlayerStateId::Riposte, "assets/characters/knight/anims/attack.glb",       true,  kRiposteTargetDuration);

    animator_.set_skeleton(mesh_.skeleton());
    // Snap-load Idle as the starting pose (no blend on first frame).
    switch_to(PlayerStateId::Idle, 0.0f);
}

void Renderer::switch_to(PlayerStateId id, float fade) {
    const AnimSlot& s = anim_slots_[static_cast<size_t>(id)];
    animator_.play(s.anim, fade, s.speed, s.strip_root);
}

void Renderer::create_descriptor_pool() {
    std::array<VkDescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = FRAMES_IN_FLIGHT;
    pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[1].descriptorCount = FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    info.pPoolSizes    = pool_sizes.data();
    info.maxSets       = FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(vk_.device(), &info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateDescriptorPool failed");
    }
}

void Renderer::create_descriptor_sets() {
    std::array<VkDescriptorSetLayout, FRAMES_IN_FLIGHT> layouts;
    layouts.fill(pipeline_.descriptor_set_layout());

    VkDescriptorSetAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool     = descriptor_pool_;
    ai.descriptorSetCount = FRAMES_IN_FLIGHT;
    ai.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(vk_.device(), &ai, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateDescriptorSets failed");
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView   = diffuse_texture_.view();
        image_info.sampler     = diffuse_texture_.sampler();

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = bone_palette_buffers_[i].handle();
        buffer_info.offset = 0;
        buffer_info.range  = sizeof(glm::mat4) * MAX_BONES;

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet          = descriptor_sets_[i];
        writes[0].dstBinding      = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo      = &image_info;

        writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet          = descriptor_sets_[i];
        writes[1].dstBinding      = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(vk_.device(),
                               static_cast<uint32_t>(writes.size()), writes.data(),
                               0, nullptr);
    }
}

void Renderer::init_imgui() {
    // Generous descriptor pool: ImGui spec docs recommend 1k of each type
    // for headroom against user-bound textures. The font texture alone needs
    // one combined-image-sampler.
    const std::array<VkDescriptorPoolSize, 1> pool_sizes{{
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
    }};

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    if (vkCreateDescriptorPool(vk_.device(), &pool_info, nullptr, &imgui_descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("ImGui: vkCreateDescriptorPool failed");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForVulkan(window_.native_handle())) {
        throw std::runtime_error("ImGui_ImplSDL3_InitForVulkan failed");
    }

    // v1.91: RenderPass lives in InitInfo; no separate render-pass arg to
    // ImGui_ImplVulkan_Init. Font texture upload is lazy on first
    // ImGui_ImplVulkan_NewFrame — no explicit CreateFontsTexture call.
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = vk_.instance();
    init_info.PhysicalDevice = vk_.physical_device();
    init_info.Device         = vk_.device();
    init_info.QueueFamily    = vk_.graphics_family();
    init_info.Queue          = vk_.graphics_queue();
    init_info.DescriptorPool = imgui_descriptor_pool_;
    init_info.RenderPass     = swapchain_.render_pass();
    init_info.Subpass        = 0;
    init_info.MinImageCount  = FRAMES_IN_FLIGHT;
    init_info.ImageCount     = swapchain_.image_count();
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.Allocator      = nullptr;
    init_info.CheckVkResultFn = nullptr;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
    }

    imgui_ready_ = true;
    std::printf("[ImGui] Initialised (Vulkan + SDL3 backends, v%s)\n", IMGUI_VERSION);
}

void Renderer::shutdown_imgui() {
    if (!imgui_ready_) return;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    imgui_ready_ = false;
}

void Renderer::draw_debug_overlay(const Player& player_obj, float dt) {
    const PlayerState& player = player_obj.state();
    if (!imgui_ready_) return;

    // FPS rolling window.
    fps_window_[fps_cursor_] = dt;
    fps_cursor_ = (fps_cursor_ + 1) % FPS_WINDOW;
    const float dt_sum = std::accumulate(fps_window_.begin(), fps_window_.end(), 0.0f);
    const float avg_dt = dt_sum > 0.0f ? dt_sum / static_cast<float>(FPS_WINDOW) : dt;
    const float fps    = avg_dt > 0.0f ? 1.0f / avg_dt : 0.0f;

    ImGui::SetNextWindowPos({16, 16}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.78f);
    constexpr ImGuiWindowFlags kFlags =
        ImGuiWindowFlags_NoDecoration   | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    ImGui::Begin("Vigil — Day 10", nullptr, kFlags);

    ImGui::Text("FPS  %6.1f   dt %5.2f ms", fps, avg_dt * 1000.0f);
    ImGui::Separator();

    ImGui::Text("State        %s", to_string(player.id()));
    ImGui::Text("AttackPhase  %s", to_string(player.attack_phase()));
    ImGui::Text("state_time   %.3f s", player.state_time());
    ImGui::Text("attack_buf   %s", player.attack_buffered() ? "YES" : "  -");

    // i-frames dot: red while active, dim otherwise. Reviewers see at a
    // glance whether the roll is currently absorbing damage.
    const bool iframes = player.iframes_active();
    const ImU32 dot_col = iframes ? IM_COL32(220, 60, 60, 255)
                                  : IM_COL32(80, 80, 80, 255);
    ImGui::Text("i-frames    ");
    ImGui::SameLine();
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float  r = ImGui::GetFontSize() * 0.45f;
    ImGui::GetWindowDrawList()->AddCircleFilled(
        ImVec2(p.x + r, p.y + ImGui::GetFontSize() * 0.5f), r, dot_col);
    ImGui::Dummy(ImVec2(r * 2.0f + 6.0f, ImGui::GetFontSize()));

    // Day 13: stamina bar (green per GDD §15.2). Drains 12/24/25 on
    // light/heavy/roll entry; 30/s during Jog+sprint; regens 25/s
    // otherwise. Bar width matches the i-frames row above.
    ImGui::Separator();
    const auto& stam = player_obj.stamina();
    ImGui::Text("stamina  %5.1f / %.0f", stam.current(), Stamina::MAX_VALUE);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(80, 200, 110, 255));
    ImGui::ProgressBar(stam.fraction(), ImVec2(180, 0), "");
    ImGui::PopStyleColor();

    // Day 14: parry window indicator. Renders only while > 0 — short
    // burst (max 150ms) so the panel stays clean otherwise. Useful for
    // tuning the press-to-hit timing window.
    const float pw = player_obj.parry_window_remaining();
    if (pw > 0.0f) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 220, 90, 255));
        ImGui::Text("PARRY    %.3f s", pw);
        ImGui::PopStyleColor();
    }

    ImGui::Separator();
    const glm::vec3 pp = player_obj.position();
    const glm::vec3 pv = player_obj.velocity();
    ImGui::Text("pos   %+6.2f %+6.2f %+6.2f", pp.x, pp.y, pp.z);
    ImGui::Text("|vel|  %5.2f m/s", glm::length(pv));
    ImGui::Text("yaw    %+6.1f deg", glm::degrees(player_obj.yaw()));
    ImGui::Separator();
    ImGui::Text("anim.t       %.3f s", animator_.playback_time());
    ImGui::Text("blending     %s", animator_.is_blending() ? "yes" : "no");
    if (animator_.is_blending()) {
        ImGui::Text("blend_t      %.2f", animator_.blend_t());
        ImGui::ProgressBar(animator_.blend_t(), ImVec2(180, 0), "");
    }

    ImGui::End();
}

void Renderer::draw_frame(const Camera& camera, const Player& player) {
    auto& frame = frames_[current_frame_];

    vkWaitForFences(vk_.device(), 1, &frame.in_flight, VK_TRUE, UINT64_MAX);

    uint32_t image_index = 0;
    VkResult acquire = vkAcquireNextImageKHR(
        vk_.device(), swapchain_.handle(), UINT64_MAX,
        frame.image_available, VK_NULL_HANDLE, &image_index);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
        return;
    } else if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    // SM transitioned this tick? Crossfade to the new slot.
    if (player.state().state_changed()) {
        switch_to(player.state().id(), kCrossfadeDuration);
    }

    // Advance animation + write the palette into this frame's UBO.
    // The wait on frame.in_flight above guarantees the previous draw using
    // this FIF's resources is complete; safe to overwrite.
    const auto now = std::chrono::high_resolution_clock::now();
    const float dt = std::chrono::duration<float>(now - last_frame_time_).count();
    last_frame_time_ = now;

    animator_.update(dt);
    // CONTRACT (Day 12): the std::fill leaves slots joint_count..127 set
    // to identity. The practice dummy's vertex JOINTS_0 = (127,0,0,0)
    // and WEIGHTS_0 = (1,0,0,0) target slot 127 to render untransformed.
    // If this fill is removed or reordered, Mesh::make_unit_cube needs
    // its own descriptor set + identity-palette UBO.
    std::fill(palette_scratch_.begin(), palette_scratch_.end(), glm::mat4(1.0f));
    animator_.compute_bone_palette(palette_scratch_.data());
    bone_palette_buffers_[current_frame_].upload(palette_scratch_.data(),
                                                 sizeof(glm::mat4) * MAX_BONES);

    vkResetFences(vk_.device(), 1, &frame.in_flight);
    vkResetCommandBuffer(frame.command_buffer, 0);
    record_command_buffer(frame.command_buffer, image_index, camera, player.world_transform());

    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &frame.image_available;
    submit.pWaitDstStageMask    = &wait_stage;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &frame.command_buffer;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &render_finished_[image_index];

    if (vkQueueSubmit(vk_.graphics_queue(), 1, &submit, frame.in_flight) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }

    VkSwapchainKHR swap = swapchain_.handle();
    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = &render_finished_[image_index];
    present.swapchainCount     = 1;
    present.pSwapchains        = &swap;
    present.pImageIndices      = &image_index;

    VkResult present_result = vkQueuePresentKHR(vk_.present_queue(), &present);
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
        // Resize handled later.
    } else if (present_result != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    current_frame_ = (current_frame_ + 1) % FRAMES_IN_FLIGHT;
}

void Renderer::record_command_buffer(VkCommandBuffer cmd, uint32_t image_index,
                                     const Camera& camera,
                                     const glm::mat4& world_transform) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(cmd, &begin) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    std::array<VkClearValue, 2> clears{};
    clears[0].color        = { { 0.031f, 0.027f, 0.039f, 1.0f } };
    clears[1].depthStencil = { 1.0f, 0 };

    const VkExtent2D ext = swapchain_.extent();

    VkRenderPassBeginInfo rp{};
    rp.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp.renderPass        = swapchain_.render_pass();
    rp.framebuffer       = swapchain_.framebuffer(image_index);
    rp.renderArea.offset = {0, 0};
    rp.renderArea.extent = ext;
    rp.clearValueCount   = static_cast<uint32_t>(clears.size());
    rp.pClearValues      = clears.data();

    vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.handle());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout(),
                            0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(ext.width);
    viewport.height   = static_cast<float>(ext.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = ext;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Day 11: Player owns position + facing. world_transform is T(pos) *
    // Ry(yaw); the per-bone palette still rides on top of this in the
    // vertex shader. Locomotion + Roll integration both pass through here.
    glm::mat4 model = world_transform;
    const float aspect = static_cast<float>(ext.width) / static_cast<float>(ext.height);
    glm::mat4 mvp = camera.projection(aspect) * camera.view() * model;

    vkCmdPushConstants(cmd, pipeline_.layout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(mvp), &mvp);

    VkBuffer     vbufs[]   = { mesh_.vertex_buffer_handle() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vbufs, offsets);
    vkCmdBindIndexBuffer(cmd, mesh_.index_buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, mesh_.index_count(), 1, 0, 0, 0);

    // Day 12: practice dummy. Same pipeline + descriptor set as the knight
    // (see Mesh::make_unit_cube header comment for why this works). Only
    // MVP push + vertex/index rebind + drawIndexed differ.
    {
        glm::mat4 dummy_model = glm::translate(glm::mat4(1.0f), DUMMY_POSITION);
        glm::mat4 dummy_mvp   = camera.projection(aspect) * camera.view() * dummy_model;
        vkCmdPushConstants(cmd, pipeline_.layout(),
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(dummy_mvp), &dummy_mvp);

        VkBuffer     dvbufs[]   = { dummy_mesh_.vertex_buffer_handle() };
        VkDeviceSize doffsets[] = { 0 };
        vkCmdBindVertexBuffers(cmd, 0, 1, dvbufs, doffsets);
        vkCmdBindIndexBuffer(cmd, dummy_mesh_.index_buffer_handle(), 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, dummy_mesh_.index_count(), 1, 0, 0, 0);
    }

    // ImGui overlay layered on top of the mesh, still inside the main render
    // pass. Draw data was finalised by main.cpp's ImGui::Render() call
    // earlier this frame.
    if (imgui_ready_) {
        if (ImDrawData* dd = ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(dd, cmd);
        }
    }

    vkCmdEndRenderPass(cmd);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

}  // namespace vigil
