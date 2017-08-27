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

		Vulkan::VkManagedDescriptorSet AllocateDescriptorSet(const VkDevice& device, uint32_t setCount, VkDescriptorSetLayout layout);
		void FreeDescriptorSet(const VkDevice & device, VkManagedDescriptorSet * descSet);
		void SetDescriptorCount(VkDescriptorType desc, uint32_t count);
		void ClearPool();
		uint32_t Size();
		void BuildPool(const VkDevice& device, uint32_t maxSets);
	private:

		VkManagedObject<VkDescriptorPool> m_descriptorPool;
		uint32_t m_descriptorCounts[11]{ 0 };
		struct
		{
			uint32_t currentlyAllocated = 0;
			uint32_t maxAllocations = 0;
		} m_setAllocation;

	};
}