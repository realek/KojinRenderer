#pragma once
#include "VulkanObject.h"

namespace Vulkan
{
	enum VkManagedSamplerMode
	{
		UNDEFINED_UNDEFINED_COORDINATES = 0,
		COLOR_NORMALIZED_COORDINATES = 1,
		COLOR_UNNORMALIZED_COORDINATES = 2,
		DEPTH_NORMALIZED_COORDINATES = 3,
		DEPTH_UNNORMALIZED_COORDINATES = 4
		
	};
	class VkManagedDevice;
	class VkManagedSampler
	{
	public:
		VkManagedSampler(const VkDevice& device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare = VK_FALSE);
		void BasicEdit(const VkDevice& device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare);
		void MipMapEdit(const VkDevice& device, VkFilter magfilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode);
		const VkSampler sampler() const 
		{
			return m_sampler.object();
		}
	private:

		VkManagedObject<VkSampler> m_sampler;
		VkSamplerCreateInfo m_cSamplerInfo = {};
		VkManagedSamplerMode m_cMode = UNDEFINED_UNDEFINED_COORDINATES;
	};
}