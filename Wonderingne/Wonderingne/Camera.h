#pragma once

#include "DependencyIncludes.h"
#include "StandardIncludes.h"

#include "EngineConstants.h"
#include "ShaderMemoryVariables.h"

struct Camera {

public:

	inline static VkDescriptorSetLayout vk_CameraUBODescriptorSetLayout;
	inline static int numCameras = 2;
	inline static std::vector<CameraUniformBufferObject> camera_ubos = { {} };
	inline static std::vector<int> allCameraUBODescriptorSetIndices = { {} };
	inline static std::vector<std::vector<VkBuffer>> all_vk_CameraUniformBuffers;
	inline static std::vector<std::vector<VmaAllocation>> all_vk_CameraUniformBuffersAllocations;

};