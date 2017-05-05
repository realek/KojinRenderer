#pragma once
#include "VulkanObject.h"
#include <vector>

namespace Vulkan
{
	class VkManagedBuffer;
	class VkManagedImage;
	class VkManagedDescriptorSet
	{
	public:
		void LoadCombinedSamplerImageArray(uint32_t dstSetIndex,std::vector<VkManagedImage*> images, uint32_t dstBind,std::vector<VkSampler> samplers);
		void LoadCombinedSamplerImage(uint32_t dstSetIndex, VkManagedImage * image, uint32_t dstBind, VkSampler sampler);
		void LoadUniformBuffer(uint32_t dstSetIndex, VkManagedBuffer * buffer, uint32_t dstBind);
		void ClearSetsWrites();
		void ClearSetWrites(uint32_t setIndex);
		void WriteSet(uint32_t setIndex);
		void WriteSets();
		VkDescriptorSet Set(uint32_t setIndex);
		size_t Size();
	private:
		VkManagedDescriptorSet(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSet> sets, uint32_t totalDescriptorCounts[11]);
		friend class VkManagedDescriptorPool;
	private:
		std::vector<VkDescriptorSet> m_internalSets;
		std::vector<std::vector<VkWriteDescriptorSet>> m_writes;
		VkDevice m_device = VK_NULL_HANDLE;
		VkDescriptorPool m_pool = VK_NULL_HANDLE;
		uint32_t m_descriptorCounts[11]{ 0 };
		uint32_t m_totalDescriptorCounts[11];


	};
}