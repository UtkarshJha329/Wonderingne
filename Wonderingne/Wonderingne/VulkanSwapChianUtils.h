#pragma once

#include "VulkanCreateUtils.h"

void CreateSwapChain(GLFWwindow& window) {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vk_PhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vk_Surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(vk_PhysicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(vk_LogicalDevice, &createInfo, nullptr, &vk_SwapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(vk_LogicalDevice, vk_SwapChain, &imageCount, nullptr);
    vk_SwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vk_LogicalDevice, vk_SwapChain, &imageCount, vk_SwapChainImages.data());

    vk_SwapChainImageFormat = surfaceFormat.format;
    vk_SwapChainExtent = extent;
}


void CreateImageViews() {
    vk_SwapChainImageViews.resize(vk_SwapChainImages.size());

    for (size_t i = 0; i < vk_SwapChainImages.size(); i++) {

        vk_SwapChainImageViews[i] = CreateImageView(vk_SwapChainImages[i], vk_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    }
}

void CreateRenderPass() {

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vk_SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //VkSubpassDescription subpass{};
    //subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //subpass.colorAttachmentCount = 1;
    //subpass.pColorAttachments = &colorAttachmentRef;

    //VkSubpassDependency dependency{};
    //dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    //dependency.dstSubpass = 0;
    //dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependency.srcAccessMask = 0;
    //dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;



    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    //VkRenderPassCreateInfo renderPassInfo{};
    //renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //renderPassInfo.attachmentCount = 1;
    //renderPassInfo.pAttachments = &colorAttachment;
    //renderPassInfo.subpassCount = 1;
    //renderPassInfo.pSubpasses = &subpass;
    //renderPassInfo.dependencyCount = 1;
    //renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vk_LogicalDevice, &renderPassInfo, nullptr, &vk_RenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void CreateFramebuffers() {

    vk_SwapChainFramebuffers.resize(vk_SwapChainImageViews.size());

    for (size_t i = 0; i < vk_SwapChainImageViews.size(); i++) {

        std::array<VkImageView, 2> attachments = {
            vk_SwapChainImageViews[i],
            vk_DepthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vk_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = vk_SwapChainExtent.width;
        framebufferInfo.height = vk_SwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vk_LogicalDevice, &framebufferInfo, nullptr, &vk_SwapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void CreateDepthResources() {

    VkFormat depthFormat = FindDepthFormat();

    CreateImage_VMA(vk_SwapChainExtent.width, vk_SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_DepthImage, vma_DepthImageAllocation);
    vk_DepthImageView = CreateImageView(vk_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    TransitionImageLayout(vk_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


void CleanUpSwapChain() {

    vkDestroyImageView(vk_LogicalDevice, vk_DepthImageView, nullptr);
    vmaDestroyImage(vma_Allocator, vk_DepthImage, vma_DepthImageAllocation);

    for (auto framebuffer : vk_SwapChainFramebuffers) {
        vkDestroyFramebuffer(vk_LogicalDevice, framebuffer, nullptr);
    }

    for (auto imageView : vk_SwapChainImageViews) {
        vkDestroyImageView(vk_LogicalDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(vk_LogicalDevice, vk_SwapChain, nullptr);
}

void RecreateSwapChain(GLFWwindow& window) {

    int width = 0, height = 0;
    glfwGetFramebufferSize(&window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(&window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vk_LogicalDevice);

    CleanUpSwapChain();

    CreateSwapChain(window);
    CreateImageViews();
    CreateDepthResources();
    CreateFramebuffers();
}
