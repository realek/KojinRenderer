#include "Material.h"
#include <vulkan\vulkan.h>

Vulkan::Material::Material()
{
	diffuseColor = glm::vec4(0);
	diffuseTexture = VK_NULL_HANDLE;
	specularity = 0;
}

Vulkan::Material::~Material()
{
}
