#include "render/GraphicsPipeline.h"
#include "render/Swapchain.h"
#include "core/VulkanContext.h"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace vigil {

namespace {

std::vector<char> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    const std::streamsize size = file.tellg();
    std::vector<char> buf(static_cast<size_t>(size));
    file.seekg(0);
    file.read(buf.data(), size);
    return buf;
}

}  // namespace

GraphicsPipeline::GraphicsPipeline(VulkanContext& vk,
                                   const Swapchain& swapchain,
                                   const std::string& vert_spv_path,
                                   const std::string& frag_spv_path)
    : vk_(vk) {
    VkShaderModule vert = load_shader_module(vert_spv_path);
    VkShaderModule frag = load_shader_module(frag_spv_path);

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag;
    stages[1].pName  = "main";

    // No vertex buffers — shader hardcodes positions for now.
    VkPipelineVertexInputStateCreateInfo vertex_input{};
    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode    = VK_CULL_MODE_NONE;
    rasterizer.frontFace   = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable    = VK_FALSE;
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend{};
    color_blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.attachmentCount = 1;
    color_blend.pAttachments    = &color_blend_attachment;

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(vk_.device(), &layout_info, nullptr, &layout_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreatePipelineLayout failed");
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = stages;
    pipeline_info.pVertexInputState   = &vertex_input;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisample;
    pipeline_info.pColorBlendState    = &color_blend;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = layout_;
    pipeline_info.renderPass          = swapchain.render_pass();
    pipeline_info.subpass             = 0;

    if (vkCreateGraphicsPipelines(vk_.device(), VK_NULL_HANDLE, 1, &pipeline_info,
                                  nullptr, &pipeline_) != VK_SUCCESS) {
        vkDestroyShaderModule(vk_.device(), vert, nullptr);
        vkDestroyShaderModule(vk_.device(), frag, nullptr);
        throw std::runtime_error("vkCreateGraphicsPipelines failed");
    }

    // Shader modules can be destroyed once the pipeline is built.
    vkDestroyShaderModule(vk_.device(), vert, nullptr);
    vkDestroyShaderModule(vk_.device(), frag, nullptr);
}

GraphicsPipeline::~GraphicsPipeline() {
    if (pipeline_) vkDestroyPipeline(vk_.device(), pipeline_, nullptr);
    if (layout_)   vkDestroyPipelineLayout(vk_.device(), layout_, nullptr);
}

VkShaderModule GraphicsPipeline::load_shader_module(const std::string& path) {
    const auto code = read_file(path);
    VkShaderModuleCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = code.size();
    info.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule mod = VK_NULL_HANDLE;
    if (vkCreateShaderModule(vk_.device(), &info, nullptr, &mod) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateShaderModule failed for " + path);
    }
    return mod;
}

}  // namespace vigil
