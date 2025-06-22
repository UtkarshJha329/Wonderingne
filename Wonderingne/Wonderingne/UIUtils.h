#pragma once

#include "UI.h"
#include "Model.h"

#include "VulkanCreateUtils.h"

void CreateDescriptorSetLayoutForUIInstanceSSBO() {

    VkDescriptorSetLayoutBinding uiSSBOLayoutBinding{};
    uiSSBOLayoutBinding.binding = UI_INSTANCE_MODEL_SSBO_BINDING_LOCATION;
    uiSSBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    //uiSSBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    uiSSBOLayoutBinding.descriptorCount = 1;
    uiSSBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uiSSBOLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vk_LogicalDevice, &layoutInfo, nullptr, &UI::vk_uiSSBODescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void CreateDescriptorSetsForUIInstanceSSBO() {

    UI::vk_UI_Instance_Model_SSBO_DescriptorSetIndex = static_cast<int>(vk_DescriptorSetsForEachFlightFrame.size());
    vk_DescriptorSetsForEachFlightFrame.push_back(std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>());

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, UI::vk_uiSSBODescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vk_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(vk_LogicalDevice, &allocInfo, vk_DescriptorSetsForEachFlightFrame[UI::vk_UI_Instance_Model_SSBO_DescriptorSetIndex].data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate Instance SSBO descriptor set! Index := " + std::to_string(UI::vk_UI_Instance_Model_SSBO_DescriptorSetIndex));
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = UI::vk_UI_Instance_Model_SSBOBuffers[i];
        bufferInfo.offset = 0;
        //bufferInfo.range = VK_WHOLE_SIZE;
        bufferInfo.range = sizeof(glm::mat4);

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = vk_DescriptorSetsForEachFlightFrame[UI::vk_UI_Instance_Model_SSBO_DescriptorSetIndex][i];
        descriptorWrites[0].dstBinding = UI_INSTANCE_MODEL_SSBO_BINDING_LOCATION;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        //descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(vk_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void CreateUIModelInstanceSSBOs_VMA()
{
    VkDeviceSize bufferSize = sizeof(glm::mat4) * UI::uiModelMatricesPerInstance.size();

    UI::vk_UI_Instance_Model_SSBOBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    UI::vk_UI_Model_Instance_SSBOBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UI::vk_UI_Instance_Model_SSBOBuffers[i], UI::vk_UI_Model_Instance_SSBOBuffersAllocations[i]);
        //vmaSetAllocationName(vma_Allocator, vk_CameraUniformBuffersAllocations[i], "camera buffer allocation");

        if (vmaCopyMemoryToAllocation(vma_Allocator, UI::uiModelMatricesPerInstance.data(), UI::vk_UI_Model_Instance_SSBOBuffersAllocations[i], 0, bufferSize) != VK_SUCCESS) {
            throw std::runtime_error("failed to copy ui model instance data to storage buffer!");
        }
    }
}

void UpdateUIModelUniformBuffer(Mesh& currentMesh, uint32_t indexOfDataForCurrentFrame) {

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ModelUniformBufferObject model_ubo{};

    model_ubo.model = glm::mat4(1.0);
    model_ubo.model = glm::rotate(model_ubo.model, glm::radians(90.0f) * -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    //model_ubo.model = glm::rotate(model_ubo.model, time * glm::radians(90.0f) * -1.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    model_ubo.model = glm::scale(model_ubo.model, glm::vec3(0.1f));

    vmaCopyMemoryToAllocation(vma_Allocator, &model_ubo, currentMesh.vk_ModelUniformBuffersAllocations[indexOfDataForCurrentFrame], 0, sizeof(model_ubo));
}

void UpdateUIModelInstanceDynamicShaderBuffer(uint32_t indexOfDataForCurrentFrame) {

    vmaCopyMemoryToAllocation(vma_Allocator, UI::uiModelMatricesPerInstance.data(), UI::vk_UI_Model_Instance_SSBOBuffersAllocations[indexOfDataForCurrentFrame], 0, sizeof(glm::mat4) * UI::uiModelMatricesPerInstance.size());
}
