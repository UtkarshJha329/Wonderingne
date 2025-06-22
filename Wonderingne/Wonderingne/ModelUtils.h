#pragma once

#include "EngineConstants.h"

#include "Model.h"
#include "VulkanCreateUtils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "StbImage/stb_image.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

void CreateDescriptorSetLayoutForMaterials() {

    VkDescriptorSetLayoutBinding modelUBOLayoutBinding{};
    modelUBOLayoutBinding.binding = MODEL_UBO_BINDING_LOCATION;
    modelUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelUBOLayoutBinding.descriptorCount = 1;
    modelUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = SAMPLER_UBO_BINDING_LOCATION_IN_FRAG_SHADER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { modelUBOLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vk_LogicalDevice, &layoutInfo, nullptr, &Material::vk_DescriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}


void CreateMaterialWithTexturesForMesh(aiMaterial* mat, Mesh& curMesh, Model& model)
{
    aiTextureType type = aiTextureType_DIFFUSE;
    for (unsigned int i = 0; i < mat->GetTextureCount(type) && i < 1; i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string curTexturePathNameFull = model.directory + "/" + std::string(str.C_Str());
        if (!Texture::allLoadedTexturePathsWithMaterialIndex.contains(curTexturePathNameFull)) {

            std::cout << "Newly added texture := " << curTexturePathNameFull << " to texture list to be loaded." << std::endl;

            curMesh.materialIndex = static_cast<int>(Material::allLoadedMaterials.size());
            Material::allLoadedMaterials.push_back(Material());

            Material::allLoadedMaterials[curMesh.materialIndex].diffuseTextureIndex = static_cast<int>(Texture::allLoadedTextures.size());
            Texture::allLoadedTextures.push_back(Texture());
            Texture::allLoadedTextures[Material::allLoadedMaterials[curMesh.materialIndex].diffuseTextureIndex].texturePath = curTexturePathNameFull;

            Texture::allLoadedTexturePathsWithMaterialIndex[curTexturePathNameFull] = curMesh.materialIndex;

            //std::cout << "CALL 1 := " << "Added new material and texture.materialIndex : = " << curMesh.materialIndex << " textureIndex := " << Material::allLoadedMaterials[curMesh.materialIndex].diffuseTextureIndex << std::endl;
        }
        else {

            std::cout << "Already added texture := " << curTexturePathNameFull << " to texture list to be loaded." << std::endl;
            curMesh.materialIndex = Texture::allLoadedTexturePathsWithMaterialIndex[curTexturePathNameFull];
        }

    }
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, Mesh& curMesh, Model& model)
{

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

        curMesh.vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            curMesh.indices.push_back(face.mIndices[j]);
            //std::cout << indices[indices.size() - 1] << std::endl;
        }
    }

    CreateMaterialWithTexturesForMesh(scene->mMaterials[mesh->mMaterialIndex], curMesh, model);
    //std::cout << "CALL 2 := " << " materialIndex : = " << curMesh.materialIndex << " textureIndex := " << Material::allLoadedMaterials[curMesh.materialIndex].diffuseTextureIndex << std::endl;
}

void ProcessNode(aiNode* node, const aiScene* scene, Model& model)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        // Can be made faster by simply pre calculating the number of meshes and then passing around the index of the mesh in the meshes array to populate with data.
        Mesh curMesh = {};
        ProcessMesh(mesh, scene, curMesh, model);
        model.meshes.push_back(curMesh);

        //std::cout << "Node Meshes := " << node->mNumMeshes << std::endl;
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        //std::cout << "Processing children meshes." << std::endl;
        ProcessNode(node->mChildren[i], scene, model);

        //std::cout << "Children Meshes := " << node->mNumChildren << std::endl;
    }
}

void LoadModelDataWithAssimp(Model& model) {

    if (strcmp(model.path.c_str(), "") == 0) {
        throw std::runtime_error("Empty path for model!");
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(model.path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }
    model.directory = model.path.substr(0, model.path.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene, model);
}


void CreateVertexBuffer_VMA(Mesh& currentMesh) {

    VkDeviceSize bufferSize = sizeof(currentMesh.vertices[0]) * currentMesh.vertices.size();

    VkBuffer stagingBuffer;
    VmaAllocation vma_StagingBufferAllocation;
    CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

    if (vmaCopyMemoryToAllocation(vma_Allocator, currentMesh.vertices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
        throw std::runtime_error("failed to copy vertex data to staging buffer!");
    }

    CreateBuffer_VMA(bufferSize, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, currentMesh.vk_VertexBuffer, currentMesh.vma_VertexBufferAllocation);

    CopyBuffer(stagingBuffer, currentMesh.vk_VertexBuffer, bufferSize);
    vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);
}

void CreateIndexBuffer_VMA(Mesh& currentMesh) {

    VkDeviceSize bufferSize = sizeof(currentMesh.indices[0]) * currentMesh.indices.size();

    VkBuffer stagingBuffer;
    VmaAllocation vma_StagingBufferAllocation;
    CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

    //if (vmaCopyMemoryToAllocation(vma_Allocator, indices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
    if (vmaCopyMemoryToAllocation(vma_Allocator, currentMesh.indices.data(), vma_StagingBufferAllocation, 0, bufferSize) != VK_SUCCESS) {
        throw std::runtime_error("failed to copy vertex data to staging buffer!");
    }

    CreateBuffer_VMA(bufferSize, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, currentMesh.vk_IndexBuffer, currentMesh.vma_IndexBufferAllocation);

    CopyBuffer(stagingBuffer, currentMesh.vk_IndexBuffer, bufferSize);
    vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);
}

void CreateModelUniformBuffers_VMA(Mesh& currentMesh) {

    VkDeviceSize bufferSize = sizeof(ModelUniformBufferObject);

    currentMesh.vk_ModelUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    currentMesh.vk_ModelUniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer_VMA(bufferSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, currentMesh.vk_ModelUniformBuffers[i], currentMesh.vk_ModelUniformBuffersAllocations[i]);

        if (vmaCopyMemoryToAllocation(vma_Allocator, currentMesh.vk_ModelUniformBuffers.data(), currentMesh.vk_ModelUniformBuffersAllocations[i], 0, bufferSize) != VK_SUCCESS) {
            throw std::runtime_error("failed to copy model buffer data to buffer!");
        }
    }
}

void CreateMaterialDescriptorSetsForMesh(Mesh& curMesh) {

    Material& curMaterial = Material::allLoadedMaterials[curMesh.materialIndex];
    if (curMaterial.descriptorSetIndex < 0) {

        curMaterial.descriptorSetIndex = static_cast<int>(vk_DescriptorSetsForEachFlightFrame.size());
        vk_DescriptorSetsForEachFlightFrame.push_back(std::array<VkDescriptorSet, 2>());

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, Material::vk_DescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = vk_DescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(vk_LogicalDevice, &allocInfo, vk_DescriptorSetsForEachFlightFrame[curMaterial.descriptorSetIndex].data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = curMesh.vk_ModelUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(ModelUniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = Texture::allLoadedTextures[curMaterial.diffuseTextureIndex].vk_TextureImageView;
            imageInfo.sampler = vk_TextureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = vk_DescriptorSetsForEachFlightFrame[curMaterial.descriptorSetIndex][i];
            descriptorWrites[0].dstBinding = MODEL_UBO_BINDING_LOCATION;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = vk_DescriptorSetsForEachFlightFrame[curMaterial.descriptorSetIndex][i];
            descriptorWrites[1].dstBinding = SAMPLER_UBO_BINDING_LOCATION_IN_FRAG_SHADER;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(vk_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

    }

}

void CreateTextureImageViewForTexture(Texture& curTexture) {

    curTexture.vk_TextureImageView = CreateImageView(curTexture.vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CreateTextureImageAndViewOnGPU() {

    for (int i = 0; i < Texture::allLoadedTextures.size(); i++)
    {
        Texture& curTexture = Texture::allLoadedTextures[i];

        if (curTexture.loaded == false) {
            std::string curTexturePath = curTexture.texturePath;

            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(curTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (!pixels) {
                throw std::runtime_error("failed to load texture image! path := " + curTexturePath);
            }

            uint64_t imageDataSize = texWidth * texHeight * 4;

            VkBuffer stagingBuffer;
            VmaAllocation vma_StagingBufferAllocation;

            CreateBuffer_VMA(imageDataSize, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, vma_StagingBufferAllocation);

            if (vmaCopyMemoryToAllocation(vma_Allocator, pixels, vma_StagingBufferAllocation, 0, imageDataSize) != VK_SUCCESS) {
                throw std::runtime_error("failed to copy texture data to staging buffer!");
            }

            stbi_image_free(pixels);

            CreateImage_VMA(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, curTexture.vk_TextureImage, curTexture.vma_TextureImageAllocation);

            std::cout << "Allocated Image Texture := " << curTexture.texturePath << std::endl;

            std::string allocName = curTexture.texturePath + std::string(" = (Texture For Mesh Allocation)");
            vmaSetAllocationName(vma_Allocator, curTexture.vma_TextureImageAllocation, allocName.c_str());


            TransitionImageLayout(curTexture.vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            CopyBufferToImage(stagingBuffer, curTexture.vk_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            TransitionImageLayout(curTexture.vk_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            vmaDestroyBuffer(vma_Allocator, stagingBuffer, vma_StagingBufferAllocation);

            CreateTextureImageViewForTexture(curTexture);
            curTexture.loaded = true;
        }
    }
}

void UpdateModelUniformBuffer(Mesh& currentMesh, uint32_t indexOfDataForCurrentFrame) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ModelUniformBufferObject model_ubo{};

    model_ubo.model = glm::mat4(1.0);
    model_ubo.model = glm::rotate(model_ubo.model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model_ubo.model = glm::rotate(model_ubo.model, time * glm::radians(90.0f) * -1.0f, glm::vec3(0.0f, 1.0f, 0.0f));

    vmaCopyMemoryToAllocation(vma_Allocator, &model_ubo, currentMesh.vk_ModelUniformBuffersAllocations[indexOfDataForCurrentFrame], 0, sizeof(model_ubo));
}


void LoadSingleModelDataToCPU(Model& _currentModel) {

    LoadModelDataWithAssimp(_currentModel);
}

void LoadAllModelsDataToCPU(const std::vector<std::string>& allModelPaths, std::vector<Model>& allModels) {

    for (int i = 0; i < allModelPaths.size(); i++)
    {
        allModels.push_back(Model(allModelPaths[i]));
        LoadSingleModelDataToCPU(allModels.back());
    }
}

void UploadSingleModelToGPU(Model& _currentModel) {

    for (int i = 0; i < _currentModel.meshes.size(); i++)
    {
        CreateVertexBuffer_VMA(_currentModel.meshes[i]);
        CreateIndexBuffer_VMA(_currentModel.meshes[i]);

        //TODO : Need to create separate buffers for each object or somehow increase the sizee of one and index into it in the shader or something.
        CreateModelUniformBuffers_VMA(_currentModel.meshes[i]);

        CreateMaterialDescriptorSetsForMesh(_currentModel.meshes[i]);
    }
}

void UploadAllModelsAndMaterialDataToGPU(std::vector<Model>& allModels) {

    CreateTextureImageAndViewOnGPU();

    for (int i = 0; i < allModels.size(); i++)
    {
        UploadSingleModelToGPU(allModels[i]);
    }
}

void CleanUpModelData(Model& currentModel) {

    for (int i = 0; i < currentModel.meshes.size(); i++)
    {
        Mesh& curMesh = currentModel.meshes[i];

        vmaDestroyBuffer(vma_Allocator, curMesh.vk_VertexBuffer, curMesh.vma_VertexBufferAllocation);
        vmaDestroyBuffer(vma_Allocator, curMesh.vk_IndexBuffer, curMesh.vma_IndexBufferAllocation);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vmaDestroyBuffer(vma_Allocator, curMesh.vk_ModelUniformBuffers[i], curMesh.vk_ModelUniformBuffersAllocations[i]);
        }

    }
}