#include "VkManagedShaderModule.h"

Vulkan::VkManagedShaderModule::VkManagedShaderModule(VkManagedDevice * device, std::string path)
{
}

Vulkan::VkManagedShaderModule::operator VkShaderModule()
{
	return m_shader;
}

void Vulkan::VkManagedShaderModule::Reload()
{
}

VkPipelineShaderStageCreateInfo Vulkan::VkManagedShaderModule::GetShaderStageCreateInfo()
{
	return VkPipelineShaderStageCreateInfo();
}
