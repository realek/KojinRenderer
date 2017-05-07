#include <vulkan\vulkan.h>
#include "Material.h"

Vulkan::Material::Material()
{
	diffuseColor = glm::vec4(1);
	//diffuseTexture = VK_NULL_HANDLE;
	specularity = 0;
}

Vulkan::Material::~Material()
{
}
