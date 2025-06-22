#pragma once

#include "DependencyIncludes.h"
#include "StandardIncludes.h"

#include "EngineConstants.h"

struct UI {

public:

	inline static VkDescriptorSetLayout vk_uiSSBODescriptorSetLayout;
	inline static int vk_UI_Instance_Model_SSBO_DescriptorSetIndex = -1;
	inline static std::vector<VkBuffer> vk_UI_Instance_Model_SSBOBuffers;
	inline static std::vector<VmaAllocation> vk_UI_Model_Instance_SSBOBuffersAllocations;

	inline static std::vector<glm::mat4> uiModelMatricesPerInstance;
	inline static std::vector<Model> allUIModelsThatNeedToBeLoadedAndRendered = {};

};