#pragma once

#include "Camera.h"
#include "VulkanCreateUtils.h"

void CreateDescriptorSetLayoutForCameraUBO() {

    VkDescriptorSetLayoutBinding cameraUBOLayoutBinding{};
    cameraUBOLayoutBinding.binding = CAMERA_UBO_BINDING_LOCATION;
    cameraUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraUBOLayoutBinding.descriptorCount = 1;
    cameraUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 1> bindings = { cameraUBOLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vk_LogicalDevice, &layoutInfo, nullptr, &Camera::vk_CameraUBODescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void CreateDescriptorSetsForCameraData() {

    for (int i = 0; i < Camera::numCameras; i++)
    {
        Camera::allCameraUBODescriptorSetIndices[i] = static_cast<int>(vk_DescriptorSetsForEachFlightFrame.size());
        vk_DescriptorSetsForEachFlightFrame.push_back(std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>());

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, Camera::vk_CameraUBODescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vk_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(vk_LogicalDevice, &allocInfo, vk_DescriptorSetsForEachFlightFrame[Camera::allCameraUBODescriptorSetIndices[i]].data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate camera descriptor set! Index := " + std::to_string(Camera::allCameraUBODescriptorSetIndices[i]));
        }

        for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = Camera::all_vk_CameraUniformBuffers[i][j];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraUniformBufferObject);

            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = vk_DescriptorSetsForEachFlightFrame[Camera::allCameraUBODescriptorSetIndices[i]][j];
            descriptorWrites[0].dstBinding = CAMERA_UBO_BINDING_LOCATION;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(vk_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}

void CreateCameraUniformBuffers_VMA() {

    VkDeviceSize bufferSize = sizeof(CameraUniformBufferObject);

    for (int i = 0; i < Camera::numCameras; i++)
    {
        Camera::all_vk_CameraUniformBuffers[i].resize(MAX_FRAMES_IN_FLIGHT);
        Camera::all_vk_CameraUniformBuffersAllocations[i].resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {

            CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, Camera::all_vk_CameraUniformBuffers[i][j], Camera::all_vk_CameraUniformBuffersAllocations[i][j]);
            //vmaSetAllocationName(vma_Allocator, vk_CameraUniformBuffersAllocations[i], "camera buffer allocation");


            if (vmaCopyMemoryToAllocation(vma_Allocator, Camera::all_vk_CameraUniformBuffers[i].data(), Camera::all_vk_CameraUniformBuffersAllocations[i][j], 0, bufferSize) != VK_SUCCESS) {
                throw std::runtime_error("failed to copy camera ubo data to staging buffer!");
            }
        }
    }
}

void InitCamerasAndData() {

    for (int i = 0; i < Camera::numCameras; i++)
    {
        Camera::camera_ubos.push_back({});
        Camera::allCameraUBODescriptorSetIndices.push_back({});

        Camera::all_vk_CameraUniformBuffers.push_back({});
        Camera::all_vk_CameraUniformBuffersAllocations.push_back({});
    }

    CreateCameraUniformBuffers_VMA();
    CreateDescriptorSetsForCameraData();
}


void UpdateCameraUniformBuffer(uint32_t indexOfDataForCurrentFrame, int indexOfCameraToUpdate) {

    if (indexOfCameraToUpdate == 0) {
        glm::vec3 cameraPos = glm::vec3(5.0f);

        Camera::camera_ubos[indexOfCameraToUpdate].view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        Camera::camera_ubos[indexOfCameraToUpdate].proj = glm::perspective(glm::radians(45.0f), vk_SwapChainExtent.width / (float)vk_SwapChainExtent.height, 0.1f, 1000.0f);
        Camera::camera_ubos[indexOfCameraToUpdate].proj[1][1] *= -1;
    }
    else if (indexOfCameraToUpdate == 1) {

        glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 0.0f);
        Camera::camera_ubos[indexOfCameraToUpdate].view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        //float x = static_cast<float>(vk_SwapChainExtent.width) * 0.5f;
        float x = 1.0f;
        float y = static_cast<float>(vk_SwapChainExtent.height) / static_cast<float>(vk_SwapChainExtent.width);

        Camera::camera_ubos[indexOfCameraToUpdate].proj = glm::ortho(-1.0f, 1.0f, -y, y, 0.1f, 1000.0f);

        //glm::vec3 cameraPos = glm::vec3(5.0f);

        //camera_ubos[indexOfCameraToUpdate].view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        //camera_ubos[indexOfCameraToUpdate].proj = glm::perspective(glm::radians(45.0f), vk_SwapChainExtent.width / (float)vk_SwapChainExtent.height, 0.1f, 1000.0f);
        //camera_ubos[indexOfCameraToUpdate].proj[1][1] *= -1;
    }

    vmaCopyMemoryToAllocation(vma_Allocator, &Camera::camera_ubos[indexOfCameraToUpdate], Camera::all_vk_CameraUniformBuffersAllocations[indexOfCameraToUpdate][indexOfDataForCurrentFrame], 0, sizeof(Camera::camera_ubos[indexOfCameraToUpdate]));
}