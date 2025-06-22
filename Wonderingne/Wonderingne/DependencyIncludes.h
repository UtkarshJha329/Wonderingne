#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION
//#define VMA_DEBUG_LOG_FORMAT(format, ...) do { \
//       printf((format), __VA_ARGS__); \
//       printf("\n"); \
//   } while(false)
#include "vma/vk_mem_alloc.h"