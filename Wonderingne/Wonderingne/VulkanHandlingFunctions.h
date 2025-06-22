#pragma once

#include "VulkanEngineVariables.h"
#include "VulkanDebugUtils.h"
#include "VulkanInitUtils.h"
#include "VulkanCreateUtils.h"

#include "VulkanSwapChianUtils.h"

#include "CreateVulkanGraphicsPipeline.h"

#include "VulkanRenderingFunctions.h"

#include "CameraUtils.h"
#include "ModelUtils.h"
#include "UIUtils.h"


void InitVKInstance(const std::string applicationName) {

    // Check for Validation Layer
    if (enableValidationLayers && !CheckForVulkanValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    // Get platform supported extensions list.
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.pEngineName = ENGINE_NAME.c_str();
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    //appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto requiredExtensions = GetRequiredExtensions();

    // CHECK IF REQUIRED EXTENSIONS ARE SUPPORTED!!!
    {
        for (int i = 0; i < requiredExtensions.size(); i++)
        {
            bool requiredVulkanExtensionNotSupported = true;

            for (const auto& curSupportedExtension : supportedExtensions) {
                if (std::strcmp(curSupportedExtension.extensionName, requiredExtensions[i]) == 0 || std::strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, requiredExtensions[i]) == 0) {
                    requiredVulkanExtensionNotSupported = false;

                    std::cout << "Required Vulkan extension := " << requiredExtensions[i] << " is supported." << std::endl;

                    break;
                }
            }

            if (requiredVulkanExtensionNotSupported) {
                throw std::runtime_error("Required vulkan extensions not found!! := " + std::string(requiredExtensions[i]));
            }

        }
    }

    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Set up debug messenger for monitoring vkCreateInstance and vkDestroyInstance
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }


    if (vkCreateInstance(&createInfo, nullptr, &vk_Instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void SetupDebugMessenger() {

    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(vk_Instance, &createInfo, nullptr, &vk_DebugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void CreateSurface(GLFWwindow& window) {
    if (glfwCreateWindowSurface(vk_Instance, &window, nullptr, &vk_Surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void PickPhysicalDevice() {

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk_Instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk_Instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsPhysicalDeviceGPUSuitable(device)) {
            vk_PhysicalDevice = device;
            break;
        }
    }

    if (vk_PhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(vk_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceVulkan11Features features11 = {};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.pNext = &features11;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(vk_PhysicalDevice, &createInfo, nullptr, &vk_LogicalDevice) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(vk_LogicalDevice, indices.graphicsFamily.value(), 0, &vk_GraphicsQueue);
    vkGetDeviceQueue(vk_LogicalDevice, indices.presentFamily.value(), 0, &vk_PresentQueue);
}

void CreateVulkanMemoryAllocator() {

    VmaAllocatorCreateInfo createInfo = {};
    createInfo.instance = vk_Instance;

    createInfo.physicalDevice = vk_PhysicalDevice;
    createInfo.device = vk_LogicalDevice;

    createInfo.preferredLargeHeapBlockSize = 0;

    createInfo.vulkanApiVersion = VK_API_VERSION_1_0;

    vmaCreateAllocator(&createInfo, &vma_Allocator);
}




void CreateCommandPool() {

    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(vk_PhysicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vk_LogicalDevice, &poolInfo, nullptr, &vk_CommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}




void CreateTextureSampler() {

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;                                 // ENABLE ANISOTROPY!!!
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vk_PhysicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;     // MAXIMUM ANISOTROPY THE DEVICE SUPPORTS!!!

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(vk_LogicalDevice, &samplerInfo, nullptr, &vk_TextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}



void CreateDescriptorPool() {

    const int largeRandomNumberOfDescriptorSets = 333 * MAX_FRAMES_IN_FLIGHT;


    std::array<VkDescriptorPoolSize, largeRandomNumberOfDescriptorSets> poolSizes{};

    int eachPartSize = largeRandomNumberOfDescriptorSets / 3;

    for (int i = 0; i < eachPartSize; i++)
    {
        poolSizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(eachPartSize);
    }

    for (int i = 2 * eachPartSize; i < largeRandomNumberOfDescriptorSets; i++)
    {
        poolSizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        //poolSizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(eachPartSize);
    }

    for (int i = eachPartSize; i < 2 * eachPartSize; i++)
    {
        poolSizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[i].descriptorCount = static_cast<uint32_t>(eachPartSize);
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(largeRandomNumberOfDescriptorSets);

    if (vkCreateDescriptorPool(vk_LogicalDevice, &poolInfo, nullptr, &vk_DescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}



void CreateCommandBuffers() {

    vk_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vk_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(vk_CommandBuffers.size());

    if (vkAllocateCommandBuffers(vk_LogicalDevice, &allocInfo, vk_CommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}



void CreateSyncObjects() {

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vk_LogicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vk_LogicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vk_LogicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

#pragma endregion

// VULKAN PRIVATE

void InitVulkan(const std::string& applicationName, GLFWwindow& window, std::string& vertexShaderPath, std::string& fragmentShaderPath, std::vector<std::string>& allModelsFilePaths, std::vector<std::string> allUIModelsFilePaths) {

    InitVKInstance(applicationName);

    SetupDebugMessenger();

    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateVulkanMemoryAllocator();

    CreateSwapChain(window);
    CreateImageViews();
    CreateRenderPass();


    CreateDescriptorSetLayoutForMaterials();
    CreateDescriptorSetLayoutForCameraUBO();
    CreateDescriptorSetLayoutForUIInstanceSSBO();




    CreateGraphicsPipeline(vertexShaderPath, fragmentShaderPath);

    CreateCommandPool();

    CreateDepthResources();
    CreateFramebuffers();


    CreateDescriptorPool();

    CreateTextureSampler();

    InitCamerasAndData();


    glm::mat4 modelA;
    modelA = glm::mat4(1.0);
    modelA = glm::translate(modelA, glm::vec3(1.0f, 0.5f, 0.0f));
    modelA = glm::rotate(modelA, glm::radians(90.0f) * -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    modelA = glm::scale(modelA, glm::vec3(0.1f));

    glm::mat4 modelB;
    modelB = glm::mat4(1.0);
    modelB = glm::translate(modelB, glm::vec3(-1.0f, 0.5f, 0.0f));
    modelB = glm::rotate(modelB, glm::radians(90.0f) * -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    modelB = glm::scale(modelB, glm::vec3(0.1f));

    UI::uiModelMatricesPerInstance.push_back(modelA);
    UI::uiModelMatricesPerInstance.push_back(modelB);


    CreateUIModelInstanceSSBOs_VMA();
    CreateDescriptorSetsForUIInstanceSSBO();

    LoadAllModelsDataToCPU(allModelsFilePaths, Model::allModelsThatNeedToBeLoadedAndRendered);
    UploadAllModelsAndMaterialDataToGPU(Model::allModelsThatNeedToBeLoadedAndRendered);

    LoadAllModelsDataToCPU(allUIModelsFilePaths, UI::allUIModelsThatNeedToBeLoadedAndRendered);
    UploadAllModelsAndMaterialDataToGPU(UI::allUIModelsThatNeedToBeLoadedAndRendered);

    //CreateDescriptorSets();

    CreateCommandBuffers();
    CreateSyncObjects();

}

void VulkanCleanup() {

    vkDeviceWaitIdle(vk_LogicalDevice);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vk_LogicalDevice, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vk_LogicalDevice, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(vk_LogicalDevice, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vk_LogicalDevice, vk_CommandPool, nullptr);

    CleanUpSwapChain();




    for (int i = 0; i < Model::allModelsThatNeedToBeLoadedAndRendered.size(); i++)
    {
        CleanUpModelData(Model::allModelsThatNeedToBeLoadedAndRendered[i]);
    }

    for (int i = 0; i < UI::allUIModelsThatNeedToBeLoadedAndRendered.size(); i++)
    {
        CleanUpModelData(UI::allUIModelsThatNeedToBeLoadedAndRendered[i]);
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vmaDestroyBuffer(vma_Allocator, UI::vk_UI_Instance_Model_SSBOBuffers[i], UI::vk_UI_Model_Instance_SSBOBuffersAllocations[i]);
    }

    for (int i = 0; i < Camera::numCameras; i++)
    {
        for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
        {
            vmaDestroyBuffer(vma_Allocator, Camera::all_vk_CameraUniformBuffers[i][j], Camera::all_vk_CameraUniformBuffersAllocations[i][j]);
        }
    }

    for (int i = 0; i < Texture::allLoadedTextures.size(); i++)
    {
        Texture& curTexture = Texture::allLoadedTextures[i];

        vkDestroyImageView(vk_LogicalDevice, curTexture.vk_TextureImageView, nullptr);
        vmaDestroyImage(vma_Allocator, curTexture.vk_TextureImage, curTexture.vma_TextureImageAllocation);
    }

    vkDestroySampler(vk_LogicalDevice, vk_TextureSampler, nullptr);




    vkDestroyPipeline(vk_LogicalDevice, vk_GraphicsPipeline, nullptr);

    vkDestroyDescriptorPool(vk_LogicalDevice, vk_DescriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(vk_LogicalDevice, Material::vk_DescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk_LogicalDevice, Camera::vk_CameraUBODescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(vk_LogicalDevice, UI::vk_uiSSBODescriptorSetLayout, nullptr);

    vkDestroyPipelineLayout(vk_LogicalDevice, vk_PipelineLayout, nullptr);
    vkDestroyRenderPass(vk_LogicalDevice, vk_RenderPass, nullptr);

    vmaDestroyAllocator(vma_Allocator);

    vkDestroyDevice(vk_LogicalDevice, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(vk_Instance, vk_DebugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(vk_Instance, vk_Surface, nullptr);
    // destroy instance only after other vulkan resources are cleaned up.
    vkDestroyInstance(vk_Instance, nullptr);
}


