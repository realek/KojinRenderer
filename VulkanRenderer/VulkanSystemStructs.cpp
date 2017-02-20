#include "VulkanSystemStructs.h"
#include "VulkanHash.h"
Vulkan::VkStagingMesh::VkStagingMesh()
{
	totalIndices = 0;
}

void Vulkan::VkStagingMesh::UpdateUniforms(VkStagingMesh& updated)
{
	this->modelMatrices = updated.modelMatrices;
	this->diffuseColors = updated.diffuseColors;
	this->specularities = updated.specularities;
	this->diffuseTextures = updated.diffuseTextures;
}


void Vulkan::VkStagingMesh::ClearTemporary()
{
	vertex.clear();
	indices.clear();
}

void Vulkan::VkStagingMesh::ClearAll()
{
	totalIndices = 0;
	ids.clear();
	vertex.clear();
	indices.clear();
	modelMatrices.clear();
	diffuseTextures.clear();
	diffuseColors.clear();
	specularities.clear();
	indiceBases.clear();

}
