#include "Material.h"
#include <vulkan\vulkan.h>
std::atomic<int> Vulkan::Material::globalID = 0;
Vulkan::Material::Material() : m_materialID(++globalID)
{
	diffuseColor = glm::vec4(0);
	diffuseTexture = VK_NULL_HANDLE;
	specularity = 0;
}

Vulkan::Material::~Material()
{
}

int Vulkan::Material::GetID()
{
	return m_materialID;
}
