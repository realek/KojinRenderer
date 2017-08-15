#pragma once
#include "VulkanObject.h"
#include <vector>
namespace Vulkan
{

	class VkManagedDevice;
	class VkManagedDescriptorSet;
	class VkManagedDescriptorPool 
	{
	public:
		VkManagedDescriptorPool(VkManagedDevice * device);
		VkManagedDescriptorSet AllocateDescriptorSet(uint32_t setCount, VkDescriptorSetLayout layout);
		void AllocateDescriptorSet(uint32_t setCount, VkDescriptorSetLayout layout, VkManagedDescriptorSet *& descriptorSet);
		void FreeDescriptorSet(VkManagedDescriptorSet * descSet);
		void SetDescriptorCount(VkDescriptorType desc, uint32_t count);
		void ClearPool();
		uint32_t Size();
		void BuildPool(uint32_t maxSets);

	private:
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice,false };
		VkManagedObject<VkDescriptorPool> m_descriptorPool{ m_device, vkDestroyDescriptorPool };
		uint32_t m_descriptorCounts[11]{ 0 };
		struct
		{
			uint32_t currentlyAllocated = 0;
			uint32_t maxAllocations = 0;
		} m_setAllocation;

	};
}