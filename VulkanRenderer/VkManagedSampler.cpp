#include "VkManagedSampler.h"
#include "VkManagedDevice.h"
#include <assert.h>

Vulkan::VkManagedSampler::VkManagedSampler(const VkDevice& device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare)
{
	assert(device != VK_NULL_HANDLE);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.compareEnable = compare;
	if (mode == DEPTH_NORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS;
	}
	else if(mode == COLOR_NORMALIZED_COORDINATES || mode == COLOR_UNNORMALIZED_COORDINATES)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	}

	if (anisotrophy == 0)
		samplerInfo.anisotropyEnable = VK_FALSE;
	else
		samplerInfo.anisotropyEnable = VK_TRUE;

	samplerInfo.maxAnisotropy = anisotrophy;
	samplerInfo.borderColor = color;
	if (mode == COLOR_UNNORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
		samplerInfo.unnormalizedCoordinates = VK_TRUE;
	else if (mode == COLOR_NORMALIZED_COORDINATES || mode == DEPTH_NORMALIZED_COORDINATES)
		samplerInfo.unnormalizedCoordinates = VK_FALSE;

	VkSampler sampler = VK_NULL_HANDLE;
	VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
	assert(result == VK_SUCCESS);
	m_sampler.set_object(sampler, device, vkDestroySampler);

	m_cSamplerInfo = samplerInfo;
}

void Vulkan::VkManagedSampler::BasicEdit(const VkDevice& device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, VkBool32 compare)
{
	bool update = false;
	VkSamplerCreateInfo samplerInfo = m_cSamplerInfo;
	if (mode != m_cMode)
	{
		samplerInfo.compareEnable = compare;
		if (mode == DEPTH_NORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
		{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

			samplerInfo.compareOp = VK_COMPARE_OP_LESS;
		}
		else if (mode == COLOR_NORMALIZED_COORDINATES || mode == COLOR_UNNORMALIZED_COORDINATES)
		{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		}

		if (mode == COLOR_UNNORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
			samplerInfo.unnormalizedCoordinates = VK_TRUE;
		else if (mode == COLOR_NORMALIZED_COORDINATES || mode == DEPTH_NORMALIZED_COORDINATES)
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
		update = true;
	}

	if (samplerInfo.maxAnisotropy != anisotrophy)
	{
		if (anisotrophy == 0)
			samplerInfo.anisotropyEnable = VK_FALSE;
		else
			samplerInfo.anisotropyEnable = VK_TRUE;

		samplerInfo.maxAnisotropy = anisotrophy;

		update = true;
	}

	if (samplerInfo.borderColor != color)
	{
		samplerInfo.borderColor = color;
		update = true;
	}

	if (update)
	{
		VkSampler sampler = VK_NULL_HANDLE;
		VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
		assert(result == VK_SUCCESS);
		m_sampler.set_object(sampler, device, vkDestroySampler);
		m_cSamplerInfo = samplerInfo;
	}
}

void Vulkan::VkManagedSampler::MipMapEdit(const VkDevice& device, VkFilter magfilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode)
{
	bool update = false;
	VkSamplerCreateInfo samplerInfo = m_cSamplerInfo;
	if(samplerInfo.mipmapMode != mipmapMode)
	{
		samplerInfo.mipmapMode = mipmapMode;
		update = true;
	}

	if (samplerInfo.magFilter != magfilter)
	{
		samplerInfo.magFilter = magfilter;
		update = true;
	}

	if (samplerInfo.minFilter != minFilter)
	{
		samplerInfo.minFilter = minFilter;
		update = true;
	}

	if(update)
	{
		VkSampler sampler = VK_NULL_HANDLE;
		VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &sampler);
		assert(result == VK_SUCCESS);
		m_sampler.set_object(sampler, device, vkDestroySampler);
		m_cSamplerInfo = samplerInfo;
	}
}
