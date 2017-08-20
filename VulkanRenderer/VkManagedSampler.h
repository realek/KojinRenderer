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
		VkManagedSampler(VkManagedDevice * device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare = VK_FALSE);
		void BasicEdit(VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare);
		void MipMapEdit(VkFilter magfilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode);
		operator VkSampler() const;
	private:
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice, false };
		VkManagedObject<VkSampler> m_sampler{ m_device, vkDestroySampler };
		VkSamplerCreateInfo m_cSamplerInfo = {};
		VkManagedSamplerMode m_cMode = UNDEFINED_UNDEFINED_COORDINATES;
	};
}