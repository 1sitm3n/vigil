#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace vigil {

class VulkanContext;
class Swapchain;

class GraphicsPipeline {
public:
    GraphicsPipeline(VulkanContext& vk,
                     const Swapchain& swapchain,
                     const std::string& vert_spv_path,
                     const std::string& frag_spv_path);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

    VkPipeline            handle()                const { return pipeline_; }
    VkPipelineLayout      layout()                const { return layout_; }
    VkDescriptorSetLayout descriptor_set_layout() const { return descriptor_set_layout_; }

private:
    VkShaderModule load_shader_module(const std::string& path);
    void create_descriptor_set_layout();

    VulkanContext&        vk_;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkPipelineLayout      layout_                = VK_NULL_HANDLE;
    VkPipeline            pipeline_              = VK_NULL_HANDLE;
};

}  // namespace vigil
