#include "Material.h"
std::atomic<int> Vulkan::Material::globalID = 0;
Vulkan::Material::Material() : m_materialID(++globalID)
{
}

Vulkan::Material::~Material()
{
}

int Vulkan::Material::GetID()
{
	return m_materialID;
}
