
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "StbImage/stb_image.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <string>
#include <vector>

#include <optional>
#include <set>

#include <cstdint>
#include <limits>
#include <algorithm>

#include <fstream>
#include <array>

#include <chrono>

static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file! path := " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    void Run() {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:

    // Mesh stuff
private:

    struct Vertex {

        glm::vec3 position;
        //glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription GetBindingDescription() {

            VkVertexInputBindingDescription bindingDescription{};

            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }


        static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, position);

            //attributeDescriptions[1].binding = 0;
            //attributeDescriptions[1].location = 1;
            //attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            //attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }
    };

    struct Mesh {

    public:
        std::vector<Vertex> vertices = {};
        std::vector<uint32_t> indices = {};

        std::string texturePath = "";
    };

    struct Model {

    public:

        std::string path = "";
        std::string directory = "";
        std::vector<Mesh> meshes = {};

        void LoadModelDataAndSingleTexturePathWithAssimp() {

            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
                return;
            }
            directory = path.substr(0, path.find_last_of('/'));

            ProcessNode(scene->mRootNode, scene);
        }

        void ProcessNode(aiNode* node, const aiScene* scene)
        {
            // process all the node's meshes (if any)
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(ProcessMesh(mesh, scene));
            }
            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                //std::cout << "Processing children meshes." << std::endl;
                ProcessNode(node->mChildren[i], scene);
            }
        }

        Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene)
        {
            // data to fill
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // walk through each of the mesh's vertices
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex vertex;

                // positions
                vertex.position.x = mesh->mVertices[i].x;
                vertex.position.y = mesh->mVertices[i].y;
                vertex.position.z = mesh->mVertices[i].z;

                // texture coordinates
                if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
                {
                    glm::vec2 vec;

                    vec.x = mesh->mTextureCoords[0][i].x;
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.texCoord = vec;
                }
                else
                    vertex.texCoord = glm::vec2(0.0f, 0.0f);

                vertices.push_back(vertex);
            }

            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];

                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    indices.push_back(face.mIndices[j]);
                    //std::cout << indices[indices.size() - 1] << std::endl;
                }
            }

            return Mesh(vertices, indices, GetTypeTexturePath(scene->mMaterials[mesh->mMaterialIndex], aiTextureType_DIFFUSE, "texture_diffuse"));
        }

        std::string GetTypeTexturePath(aiMaterial* mat, aiTextureType type, std::string typeName)
        {
            for (unsigned int i = 0; i < mat->GetTextureCount(type) && i < 1; i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);

                return directory + "/" + std::string(str.C_Str());
            }
        }

    };

    //const std::vector<Vertex> vertices = {  {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //                                        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //                                        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //                                        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    //                                        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    //                                        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    //                                        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    //                                        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    //                                    };

    //const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0,
    //                                        4, 5, 6, 6, 7, 4 };

    const std::string texturedSuzanneOBJModelFilePath = "Assets/Models/TexturedSuzanne/TexturedSuzanne.obj";

    const std::string texturedLowPolyForestTerrainOBJModelFilePath = "Assets/Models/LowPolyForestTerrain/LowPolyForestTerrain.obj";

    std::string currentModelFilePath = texturedLowPolyForestTerrainOBJModelFilePath;
    Model currentModel;

// APPLICATION PRIVATE FUNCTIONS
private:

    void MainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            DrawFrame(currentModel);
        }
    }

    void Cleanup() {
        VulkanCleanup();
        GlfwCleanup();
    }

    // GLFW PRIVATE FUNCTIONS
private:

    void InitWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE.c_str(), nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    }

    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void GlfwCleanup() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // VULKAN PRIVATE
private:

    void InitVulkan() {
        InitVKInstance();

        SetupDebugMessenger();

        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();

        CreateVulkanMemoryAllocator();

        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreateGraphicsPipeline();

        CreateCommandPool();

        CreateDepthResources();
        CreateFramebuffers();

        LoadSingleModel();

        CreateTextureImage(currentModel.meshes[0].texturePath);
        CreateTextureImageView();
        CreateTextureSampler();

        CreateVertexBuffer_VMA(currentModel);
        CreateIndexBuffer_VMA(currentModel);
        CreateUniformBuffers_VMA();

        CreateDescriptorPool();
        CreateDescriptorSets();

        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void VulkanCleanup() {

        //vkDestroySemaphore(vk_LogicalDevice, imageAvailableSemaphore, nullptr);
        //vkDestroySemaphore(vk_LogicalDevice, renderFinishedSemaphore, nullptr);
        //vkDestroyFence(vk_LogicalDevice, inFlightFence, nullptr);

        vkDeviceWaitIdle(vk_LogicalDevice);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(vk_LogicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(vk_LogicalDevice, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(vk_LogicalDevice, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(vk_LogicalDevice, vk_CommandPool, nullptr);

        CleanUpSwapChain();







        vkDestroyImageView(vk_LogicalDevice, vk_TextureImageView, nullptr);
        vmaDestroyImage(vma_Allocator, vk_TextureImage, vma_TextureImageAllocation);
        vkDestroySampler(vk_LogicalDevice, vk_TextureSampler, nullptr);

        vmaDestroyBuffer(vma_Allocator, vk_VertexBuffer, vma_VertexBufferAllocation);
        vmaDestroyBuffer(vma_Allocator, vk_IndexBuffer, vma_IndexBufferAllocation);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vmaDestroyBuffer(vma_Allocator, vk_UniformBuffers[i], vk_UniformBuffersAllocations[i]);
        }






        vkDestroyPipeline(vk_LogicalDevice, vk_GraphicsPipeline, nullptr);

        vkDestroyDescriptorPool(vk_LogicalDevice, vk_DescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vk_LogicalDevice, vk_DescriptorSetLayout, nullptr);

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

    // VULKAN PRIVATE FUNCTIONS
private:

#pragma region Static Initialize Vulkan Stuff

    void InitVKInstance() {

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
        appInfo.pApplicationName = APPLICATION_NAME.c_str();
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
        appInfo.pEngineName = ENGINE_NAME.c_str();
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
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

    void CreateSurface() {
        if (glfwCreateWindowSurface(vk_Instance, window, nullptr, &vk_Surface) != VK_SUCCESS) {
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

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

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

    
    
    void CreateSwapChain() {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(vk_PhysicalDevice);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

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

    void CreateDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(vk_LogicalDevice, &layoutInfo, nullptr, &vk_DescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void CreateGraphicsPipeline() {
        auto vertShaderCode = ReadFile("Assets/Shaders/CompiledShaders/vert.spv");
        auto fragShaderCode = ReadFile("Assets/Shaders/CompiledShaders/frag.spv");

        VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &vk_DescriptorSetLayout;

        if (vkCreatePipelineLayout(vk_LogicalDevice, &pipelineLayoutInfo, nullptr, &vk_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = vk_PipelineLayout;

        pipelineInfo.renderPass = vk_RenderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(vk_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk_GraphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(vk_LogicalDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(vk_LogicalDevice, vertShaderModule, nullptr);
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



    void CreateDepthResources() {

        VkFormat depthFormat = FindDepthFormat();

        CreateImage_VMA(vk_SwapChainExtent.width, vk_SwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_DepthImage, vma_DepthImageAllocation);
        vk_DepthImageView = CreateImageView(vk_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        TransitionImageLayout(vk_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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


    void LoadSingleModel() {

        currentModel.path = currentModelFilePath;

        currentModel.LoadModelDataAndSingleTexturePathWithAssimp();
    }

    void UploadSingleModelToGPU() {

        CreateTextureImage(currentModel.meshes[0].texturePath);
        CreateTextureImageView();
        CreateTextureSampler();

        CreateVertexBuffer_VMA(currentModel);
        CreateIndexBuffer_VMA(currentModel);
        CreateUniformBuffers_VMA();


    }

    void CreateTextureImage(std::string texturePath) {

        // "Textures/Statue Texture.jpg"

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load texture image! path := " + texturePath);
        }

        uint64_t imageDataSize = texWidth * texHeight * 4;

        VkBuffer stagingBuffer;
        VmaAllocation vma_StagingBufferAllocation;

        CreateBuffer_VMA(imageDataSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

        if (vmaCopyMemoryToAllocation(vma_Allocator, pixels, vma_StagingBufferAllocation, 0, imageDataSize) != VK_SUCCESS) {
            throw std::runtime_error("failed to copy texture data to staging buffer!");
        }

        stbi_image_free(pixels);

        CreateImage_VMA(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_TextureImage, vma_TextureImageAllocation);

        TransitionImageLayout(vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        CopyBufferToImage(stagingBuffer, vk_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        TransitionImageLayout(vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);
    }

    void CreateTextureImageView() {
        vk_TextureImageView = CreateImageView(vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
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

    void CreateVertexBuffer_VMA(const Model& modelToLoadToGPU) {

        //VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        VkDeviceSize bufferSize = sizeof(modelToLoadToGPU.meshes[0].vertices[0]) * modelToLoadToGPU.meshes[0].vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation vma_StagingBufferAllocation;
        CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

        //if (vmaCopyMemoryToAllocation(vma_Allocator, vertices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
        if (vmaCopyMemoryToAllocation(vma_Allocator, modelToLoadToGPU.meshes[0].vertices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
            throw std::runtime_error("failed to copy vertex data to staging buffer!");
        }

        CreateBuffer_VMA(bufferSize, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_VertexBuffer, vma_VertexBufferAllocation);

        CopyBuffer(stagingBuffer, vk_VertexBuffer, bufferSize);
        vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);
        //vmaFreeMemory(vma_Allocator, vma_StagingBufferAllocation);
    }

    void CreateIndexBuffer_VMA(const Model& modelToLoadToGPU) {

        //VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkDeviceSize bufferSize = sizeof(modelToLoadToGPU.meshes[0].indices[0]) * modelToLoadToGPU.meshes[0].indices.size();

        //std::cout << "size of indices struct := " << sizeof(modelToLoadToGPU.meshes[0].indices[0]) << ", size of indices := " << modelToLoadToGPU.meshes[0].indices.size() << std::endl;

        VkBuffer stagingBuffer;
        VmaAllocation vma_StagingBufferAllocation;
        CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

        //if (vmaCopyMemoryToAllocation(vma_Allocator, indices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
        if (vmaCopyMemoryToAllocation(vma_Allocator, modelToLoadToGPU.meshes[0].indices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
            throw std::runtime_error("failed to copy vertex data to staging buffer!");
        }

        CreateBuffer_VMA(bufferSize, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_IndexBuffer, vma_IndexBufferAllocation);

        CopyBuffer(stagingBuffer, vk_IndexBuffer, bufferSize);
        vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);
    }

    void CreateUniformBuffers_VMA() {

        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        vk_UniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        vk_UniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);
        vk_UniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk_UniformBuffers[i], vk_UniformBuffersAllocations[i]);

            if (vmaCopyMemoryToAllocation(vma_Allocator, vk_UniformBuffers.data(), vk_UniformBuffersAllocations[i], 0, bufferSize) != VK_SUCCESS) {
                throw std::runtime_error("failed to copy vertex data to staging buffer!");
            }
        }
    }

    

    void CreateDescriptorPool() {

        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(vk_LogicalDevice, &poolInfo, nullptr, &vk_DescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void CreateDescriptorSets() {

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vk_DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vk_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        vk_DescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(vk_LogicalDevice, &allocInfo, vk_DescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = vk_UniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = vk_TextureImageView;
            imageInfo.sampler = vk_TextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = vk_DescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = vk_DescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(vk_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    /*
    //Only creates a single command buffer.
    void CreateCommandBuffer() {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vk_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(vk_LogicalDevice, &allocInfo, &vk_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    */

    
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

    /*
    void CreateSyncObjects() {

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(vk_LogicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(vk_LogicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(vk_LogicalDevice, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores or fences!!");
        }
    }
    */

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

#pragma region Dynamic Initialize Vulkan Stuff

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

    void RecreateSwapChain() {
        
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(vk_LogicalDevice);

        CleanUpSwapChain();

        CreateSwapChain();
        CreateImageViews();
        CreateDepthResources();
        CreateFramebuffers();
    }

#pragma endregion

#pragma region Rendering

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const Model& model) {

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

        //VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        //renderPassInfo.clearValueCount = 1;
        //renderPassInfo.pClearValues = &clearColor;

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

        VkBuffer vertexBuffers[] = { vk_VertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        //vkCmdBindIndexBuffer(commandBuffer, vk_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindIndexBuffer(commandBuffer, vk_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        //vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_PipelineLayout, 0, 1, &vk_DescriptorSets[indexOfDataForCurrentFrame], 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model.meshes[0].indices.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    /*
    void DrawFrame() {

        vkWaitForFences(vk_LogicalDevice, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk_LogicalDevice, 1, &inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(vk_LogicalDevice, vk_SwapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        vkResetCommandBuffer(vk_CommandBuffer, 0);
        RecordCommandBuffer(vk_CommandBuffer, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vk_CommandBuffer;

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vk_GraphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
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

        vkQueuePresentKHR(vk_PresentQueue, &presentInfo);
    }
    */

    void DrawFrame(const Model& model) {

        vkWaitForFences(vk_LogicalDevice, 1, &inFlightFences[indexOfDataForCurrentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vk_LogicalDevice, vk_SwapChain, UINT64_MAX, imageAvailableSemaphores[indexOfDataForCurrentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapChain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(vk_LogicalDevice, 1, &inFlightFences[indexOfDataForCurrentFrame]);

        UpdateUniformBuffer(indexOfDataForCurrentFrame);

        vkResetCommandBuffer(vk_CommandBuffers[indexOfDataForCurrentFrame], 0);
        RecordCommandBuffer(vk_CommandBuffers[indexOfDataForCurrentFrame], imageIndex, model);

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
            RecreateSwapChain();
        }
        else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        indexOfDataForCurrentFrame = (indexOfDataForCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void UpdateUniformBuffer(uint32_t indexOfDataForCurrentFrame) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};

        ubo.model = glm::mat4(1.0);
        ubo.model = glm::rotate(ubo.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f) * -1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::vec3 cameraPos = glm::vec3(5.0f);

        ubo.view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), vk_SwapChainExtent.width / (float)vk_SwapChainExtent.height, 0.1f, 1000.0f);
        ubo.proj[1][1] *= -1;

        vmaCopyMemoryToAllocation(vma_Allocator, &ubo, vk_UniformBuffersAllocations[indexOfDataForCurrentFrame], 0, sizeof(ubo));
    }

#pragma endregion



#pragma region Vulkan Init Utils

    VkShaderModule CreateShaderModule(const std::vector<char>& code) {

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(vk_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vk_Surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_Surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_Surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_Surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_Surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vk_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    bool IsPhysicalDeviceGPUSuitable(const VkPhysicalDevice& device) {

        QueueFamilyIndices indices = FindQueueFamilies(device);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.IsComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk_Surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.IsComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool HasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    VkFormat FindDepthFormat() {
        return FindSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(vk_PhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    bool CheckForVulkanValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> GetRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

#pragma endregion

#pragma region Vulkan Utils

    VkCommandBuffer BeginSingleTimeCommands() {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = vk_CommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(vk_LogicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer) {

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(vk_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(vk_GraphicsQueue);

        vkFreeCommandBuffers(vk_LogicalDevice, vk_CommandPool, 1, &commandBuffer);
    }

    void CreateBuffer(VkDeviceSize sizeInBytesOfBufferBeingPassedIn, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeInBytesOfBufferBeingPassedIn;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(vk_LogicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vk_LogicalDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(vk_LogicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(vk_LogicalDevice, buffer, bufferMemory, 0);
    }

    void CreateBuffer_VMA(VkDeviceSize sizeInBytesOfBufferBeingPassedIn, VmaAllocationCreateFlags allocInfoFlags, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VmaAllocation &allocation) {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeInBytesOfBufferBeingPassedIn;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = allocInfoFlags;
        allocInfo.requiredFlags = properties;

        VkResult result = vmaCreateBuffer(vma_Allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer using VMA!");
        }

    }

    void CreateImage_VMA(int texWidth, int texHeight, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags imageUsageFlags , VmaAllocationCreateFlags allocInfoFlags, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& imageAllocation) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(texWidth);
        imageInfo.extent.height = static_cast<uint32_t>(texHeight);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        //imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.format = imageFormat;
        //imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.tiling = imageTiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        //imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.usage = imageUsageFlags;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = allocInfoFlags;
        allocInfo.requiredFlags = properties;

        VkResult result = vmaCreateImage(vma_Allocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image using VMA! Error code := " + std::to_string(result));
        }
    }

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

        //VkCommandBufferAllocateInfo allocInfo{};
        //allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        //allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        //// VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV USE A SEPARATE COMMAND POOL SO THAT MEMORY ALLOCATION CAN BE OPTIMISED FOR INTERMEDIARY TRANSIENT BUFFERS VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
        //allocInfo.commandPool = vk_CommandPool;

        //allocInfo.commandBufferCount = 1;

        //VkCommandBuffer transferCmdCommandBuffer;
        //vkAllocateCommandBuffers(vk_LogicalDevice, &allocInfo, &transferCmdCommandBuffer);

        //VkCommandBufferBeginInfo beginInfo{};
        //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        //vkBeginCommandBuffer(transferCmdCommandBuffer, &beginInfo);

        VkCommandBuffer transferCmdCommandBuffer = BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        //copyRegion.srcOffset = 0; // Optional
        //copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(transferCmdCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        //vkEndCommandBuffer(transferCmdCommandBuffer);

        //VkSubmitInfo submitInfo{};
        //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        //submitInfo.commandBufferCount = 1;
        //submitInfo.pCommandBuffers = &transferCmdCommandBuffer;

        //vkQueueSubmit(vk_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        //vkQueueWaitIdle(vk_GraphicsQueue);

        //vkFreeCommandBuffers(vk_LogicalDevice, vk_CommandPool, 1, &transferCmdCommandBuffer);

        EndSingleTimeCommands(transferCmdCommandBuffer);
    }

    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
        
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        //barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (HasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        EndSingleTimeCommands(commandBuffer);
    }

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        EndSingleTimeCommands(commandBuffer);

    }

    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags subresourceRangeAspectMask) {

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = subresourceRangeAspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(vk_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

#pragma endregion

// VULKAN PRIVATE VARIABLES
private:

    VkInstance vk_Instance;

    VkDebugUtilsMessengerEXT vk_DebugMessenger;
    VkSurfaceKHR vk_Surface;

    VkPhysicalDevice vk_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice vk_LogicalDevice;
    
    VmaAllocator vma_Allocator;

    VkQueue vk_GraphicsQueue;
    VkQueue vk_PresentQueue;

    VkSwapchainKHR vk_SwapChain;
    std::vector<VkImage> vk_SwapChainImages;
    VkFormat vk_SwapChainImageFormat;
    VkExtent2D vk_SwapChainExtent;

    std::vector<VkImageView> vk_SwapChainImageViews;

    VkRenderPass vk_RenderPass;
    VkPipelineLayout vk_PipelineLayout;
    VkDescriptorSetLayout vk_DescriptorSetLayout;
    VkPipeline vk_GraphicsPipeline;

    std::vector<VkFramebuffer> vk_SwapChainFramebuffers;

    VkCommandPool vk_CommandPool;

    std::vector<VkCommandBuffer> vk_CommandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    uint32_t indexOfDataForCurrentFrame = 0;
    bool framebufferResized = false;

    VkBuffer vk_VertexBuffer;
    VmaAllocation vma_VertexBufferAllocation;

    VkBuffer vk_IndexBuffer;
    VmaAllocation vma_IndexBufferAllocation;
    //VkDeviceMemory vk_VertexBufferMemory;

    VkImage vk_TextureImage;
    VmaAllocation vma_TextureImageAllocation;
    VkImageView vk_TextureImageView;
    VkSampler vk_TextureSampler;

    VkImage vk_DepthImage;
    VmaAllocation vma_DepthImageAllocation;
    VkImageView vk_DepthImageView;

    VkDescriptorPool vk_DescriptorPool;
    std::vector<VkDescriptorSet> vk_DescriptorSets;

    std::vector<VkBuffer> vk_UniformBuffers;
    std::vector<VmaAllocation> vk_UniformBuffersAllocations;
    std::vector<void*> vk_UniformBuffersMapped;

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    const int MAX_FRAMES_IN_FLIGHT = 2;

// APPLICATION PUBLIC VARIABLES
public:
    inline static const uint32_t WIDTH = 800;
    inline static const uint32_t HEIGHT = 600;

    inline static const std::string WINDOW_TITLE = "Vulkan Window";

    inline static const std::string APPLICATION_NAME = "Hello Triangle Application";
    inline static const std::string ENGINE_NAME = "Wonderingne";

// APPLICATION PRIVATE VARIABLES
private:
    GLFWwindow* window;

};

int main() {
    HelloTriangleApplication app;

    try {
        app.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}