#pragma once

#include "VulkanHandlingFunctions.h"

#include "ModelUtils.h"
#include "CameraUtils.h"
#include "UIUtils.h"


void RenderModels(VkCommandBuffer& commandBuffer, std::vector<Model>& modelsToRender, int cameraIndex, int shaderFunctionIndex, uint32_t instanceCount) {

    SimplePushConstantData simplePushConstantData = {};
    simplePushConstantData.shaderFunctionUseID = shaderFunctionIndex;
    vkCmdPushConstants(commandBuffer, vk_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SimplePushConstantData), &simplePushConstantData);

    for (int i = 0; i < modelsToRender.size(); i++)
    {
        for (int j = 0; j < modelsToRender[i].meshes.size(); j++)
        {
            Mesh& curMesh = modelsToRender[i].meshes[j];
            Material& curMaterial = Material::allLoadedMaterials[curMesh.materialIndex];

            VkBuffer vertexBuffers[] = { curMesh.vk_VertexBuffer };
            VkDeviceSize offsets[] = { 0 };

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, curMesh.vk_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

            std::array<VkDescriptorSet, 3> descriptorSetsToBindForThisDrawCommand = { vk_DescriptorSetsForEachFlightFrame[Camera::allCameraUBODescriptorSetIndices[cameraIndex]][indexOfDataForCurrentFrame], vk_DescriptorSetsForEachFlightFrame[curMaterial.descriptorSetIndex][indexOfDataForCurrentFrame], vk_DescriptorSetsForEachFlightFrame[UI::vk_UI_Instance_Model_SSBO_DescriptorSetIndex][indexOfDataForCurrentFrame] };
            std::array<uint32_t, 1> dynamicOffsets = { 0 };

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_PipelineLayout, 0, static_cast<uint32_t>(descriptorSetsToBindForThisDrawCommand.size()), descriptorSetsToBindForThisDrawCommand.data(), static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(curMesh.indices.size()), instanceCount, 0, 0, 0);
        }
    }
}

void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vk_RenderPass;
    renderPassInfo.framebuffer = vk_SwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vk_SwapChainExtent;


    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_GraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vk_SwapChainExtent.width);
    viewport.height = static_cast<float>(vk_SwapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vk_SwapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    RenderModels(commandBuffer, Model::allModelsThatNeedToBeLoadedAndRendered, 0, 0, 1);
    RenderModels(commandBuffer, UI::allUIModelsThatNeedToBeLoadedAndRendered, 1, 1, UI::uiModelMatricesPerInstance.size());

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void DrawFrame(GLFWwindow& window) {

    vkWaitForFences(vk_LogicalDevice, 1, &inFlightFences[indexOfDataForCurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vk_LogicalDevice, vk_SwapChain, UINT64_MAX, imageAvailableSemaphores[indexOfDataForCurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain(window);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(vk_LogicalDevice, 1, &inFlightFences[indexOfDataForCurrentFrame]);

    vkResetCommandBuffer(vk_CommandBuffers[indexOfDataForCurrentFrame], 0);



    for (int i = 0; i < Camera::numCameras; i++)
    {
        UpdateCameraUniformBuffer(indexOfDataForCurrentFrame, i);
    }

    for (int i = 0; i < Model::allModelsThatNeedToBeLoadedAndRendered.size(); i++)
    {
        for (int j = 0; j < Model::allModelsThatNeedToBeLoadedAndRendered[i].meshes.size(); j++)
        {
            UpdateModelUniformBuffer(Model::allModelsThatNeedToBeLoadedAndRendered[i].meshes[j], indexOfDataForCurrentFrame);
        }
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        UpdateUIModelInstanceDynamicShaderBuffer(i);
    }

    for (int i = 0; i < UI::allUIModelsThatNeedToBeLoadedAndRendered.size(); i++)
    {
        for (int j = 0; j < UI::allUIModelsThatNeedToBeLoadedAndRendered[i].meshes.size(); j++)
        {
            UpdateUIModelUniformBuffer(UI::allUIModelsThatNeedToBeLoadedAndRendered[i].meshes[j], indexOfDataForCurrentFrame);
        }
    }

    RecordCommandBuffer(vk_CommandBuffers[indexOfDataForCurrentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[indexOfDataForCurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vk_CommandBuffers[indexOfDataForCurrentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[indexOfDataForCurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vk_GraphicsQueue, 1, &submitInfo, inFlightFences[indexOfDataForCurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { vk_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vk_PresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapChain(window);
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    indexOfDataForCurrentFrame = (indexOfDataForCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
