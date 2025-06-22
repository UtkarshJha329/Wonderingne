#pragma once

#include "DependencyIncludes.h"
#include "StandardIncludes.h"

#include "ShaderMemoryVariables.h"

struct Vertex {

    glm::vec3 position;
    glm::vec2 texCoord;

};

struct Material {

    int diffuseTextureIndex = -1;

    int descriptorSetIndex = -1;

    inline static std::vector<Material> allLoadedMaterials = {};

    inline static VkDescriptorSetLayout vk_DescriptorSetLayout;
};

struct Texture {

    std::string texturePath = "";

    VkImage vk_TextureImage;
    VmaAllocation vma_TextureImageAllocation;

    VkImageView vk_TextureImageView;

    bool loaded = false;

    inline static std::unordered_map<std::string, int> allLoadedTexturePathsWithMaterialIndex = {};
    inline static std::vector<Texture> allLoadedTextures = {};
};

struct Mesh {

public:
    std::vector<Vertex> vertices = {};
    std::vector<uint32_t> indices = {};

    int materialIndex = -1;

    VkBuffer vk_VertexBuffer;
    VmaAllocation vma_VertexBufferAllocation;

    VkBuffer vk_IndexBuffer;
    VmaAllocation vma_IndexBufferAllocation;

    std::vector<VkBuffer> vk_ModelUniformBuffers;
    std::vector<VmaAllocation> vk_ModelUniformBuffersAllocations;
};

struct Model {

public:

    std::string path = "";
    std::string directory = "";
    std::vector<Mesh> meshes = {};

    inline static std::vector<Model> allModelsThatNeedToBeLoadedAndRendered = {};

};