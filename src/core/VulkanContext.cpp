#include "core/VulkanContext.h"
#include "core/Window.h"

#include <cstdio>
#include <stdexcept>

namespace vigil {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user_data*/) {
    const char* sev = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   ? "ERROR"
                    : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "WARN "
                    : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)    ? "INFO "
                                                                                   : "VERB ";
    std::fprintf(stderr, "[vk %s] %s\n", sev, data->pMessage);
    return VK_FALSE;
}

}  // namespace

VulkanContext::VulkanContext(const Window& window) {
    // --- Instance ---
    vkb::InstanceBuilder builder;
    builder.set_app_name("Vigil")
           .set_engine_name("Vigil Engine")
           .require_api_version(1, 2, 0)
           .set_debug_callback(debug_callback);

#ifndef NDEBUG
    builder.request_validation_layers(true);
#endif

    for (const char* ext : window.required_vulkan_extensions()) {
        builder.enable_extension(ext);
    }

    auto inst_ret = builder.build();
    if (!inst_ret) {
        throw std::runtime_error("Failed to create Vulkan instance: " +
                                 inst_ret.error().message());
    }
    instance_ = inst_ret.value();

    // --- Surface ---
    surface_ = window.create_vulkan_surface(instance_.instance);

    // --- Physical device ---
    vkb::PhysicalDeviceSelector selector{instance_};
    auto phys_ret = selector
        .set_surface(surface_)
        .set_minimum_version(1, 2)
        .select();
    if (!phys_ret) {
        throw std::runtime_error("Failed to select physical device: " +
                                 phys_ret.error().message());
    }
    physical_device_ = phys_ret.value();
    std::printf("[Vigil] GPU: %s\n", physical_device_.name.c_str());

    // --- Logical device + queues ---
    vkb::DeviceBuilder device_builder{physical_device_};
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        throw std::runtime_error("Failed to create logical device: " +
                                 dev_ret.error().message());
    }
    device_ = dev_ret.value();

    auto gq = device_.get_queue(vkb::QueueType::graphics);
    auto pq = device_.get_queue(vkb::QueueType::present);
    if (!gq || !pq) {
        throw std::runtime_error("Failed to get device queues");
    }
    graphics_queue_ = gq.value();
    present_queue_  = pq.value();

    auto gqf = device_.get_queue_index(vkb::QueueType::graphics);
    auto pqf = device_.get_queue_index(vkb::QueueType::present);
    if (!gqf || !pqf) {
        throw std::runtime_error("Failed to get queue family indices");
    }
    graphics_family_ = gqf.value();
    present_family_  = pqf.value();

    std::printf("[Vigil] Graphics queue family: %u\n", graphics_family_);
    std::printf("[Vigil] Present queue family:  %u\n", present_family_);
}

VulkanContext::~VulkanContext() {
    vkb::destroy_device(device_);
    if (surface_) vkDestroySurfaceKHR(instance_.instance, surface_, nullptr);
    vkb::destroy_instance(instance_);
}

}  // namespace vigil
