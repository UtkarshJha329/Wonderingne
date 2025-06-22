#pragma once

#include "DependencyIncludes.h"
#include "StandardIncludes.h"
#include "EngineConstants.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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
VkPipeline vk_GraphicsPipeline;

std::vector<VkFramebuffer> vk_SwapChainFramebuffers;

VkCommandPool vk_CommandPool;

std::vector<VkCommandBuffer> vk_CommandBuffers;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
std::vector<VkSemaphore> imageAvailableSemaphores;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

uint32_t indexOfDataForCurrentFrame = 0;
bool framebufferResized = false;

VkSampler vk_TextureSampler;

VkImage vk_DepthImage;
VmaAllocation vma_DepthImageAllocation;
VkImageView vk_DepthImageView;

VkDescriptorPool vk_DescriptorPool;
std::vector<std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>> vk_DescriptorSetsForEachFlightFrame;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

