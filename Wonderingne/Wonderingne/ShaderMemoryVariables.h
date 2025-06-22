#pragma once

#include "DependencyIncludes.h"

struct ModelUniformBufferObject {
	alignas(16) glm::mat4 model;
};

struct CameraUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct SimplePushConstantData {
    alignas(16) uint32_t shaderFunctionUseID;
};