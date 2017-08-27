#pragma once
#include "VulkanObject.h"
#include <unordered_map>
namespace Vulkan
{
	//class VkManagedDevice;
	//class VkManagedShaderModule
	//{
	//public:
	//	VkManagedShaderModule(VkManagedDevice * device, std::string path);
	//	VkManagedShaderModule(const VkManagedShaderModule& other) = delete;
	//	VkManagedShaderModule& operator=(const VkManagedShaderModule& other) = delete;
	//	operator VkShaderModule();
	//	void Reload();
	//	VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo();
	//private:
	//	VkManagedObject<VkDevice> m_device{ vkDestroyDevice,false };
	//	VkManagedObject<VkShaderModule> m_shader {m_device,vkDestroyShaderModule};
	//	VkManagedObject<VkDescriptorSetLayout> m_shaderLayout{ m_device,vkDestroyDescriptorSetLayout };
	//	std::unordered_map<VkDescriptorType, uint32_t> m_shaderBindingMap;
	//};
}