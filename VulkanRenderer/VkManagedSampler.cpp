#include "VkManagedSampler.h"
#include "VkManagedDevice.h"
#include <assert.h>

Vulkan::VkManagedSampler::VkManagedSampler(VkManagedDevice * device, VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color)
{
	assert(device != nullptr);
	m_device = *device;
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	if (mode == DEPTH_NORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS;
	}
	else if(mode == COLOR_NORMALIZED_COORDINATES || mode == COLOR_UNNORMALIZED_COORDINATES)
	{
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.compareEnable = VK_FALSE;
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

	VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, ++m_sampler);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create texture sampler. Reason: " + Vulkan::VkResultToString(result));

	m_cSamplerInfo = samplerInfo;
}

void Vulkan::VkManagedSampler::BasicEdit(VkManagedSamplerMode mode, float anisotrophy, VkBorderColor color, bool apply)
{
	bool update = false;
	VkSamplerCreateInfo samplerInfo = m_cSamplerInfo;
	if (mode != m_cMode)
	{
		if (mode == DEPTH_NORMALIZED_COORDINATES || mode == DEPTH_UNNORMALIZED_COORDINATES)
		{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.compareEnable = VK_TRUE;
			samplerInfo.compareOp = VK_COMPARE_OP_LESS;
		}
		else if (mode == COLOR_NORMALIZED_COORDINATES || mode == COLOR_UNNORMALIZED_COORDINATES)
		{
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.compareEnable = VK_FALSE;
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
		if (apply)
		{
			VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, ++m_sampler);
			if (result != VK_SUCCESS)
				throw std::runtime_error("Unable to create texture sampler. Reason: " + Vulkan::VkResultToString(result));
		}
		m_cSamplerInfo = samplerInfo;
	}
}
void Vulkan::VkManagedSampler::MipMapEdit(VkFilter magfilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, bool apply)
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
		if(apply)
		{
			VkResult result = vkCreateSampler(m_device, &samplerInfo, nullptr, ++m_sampler);
			if (result != VK_SUCCESS)
				throw std::runtime_error("Unable to create texture sampler. Reason: " + Vulkan::VkResultToString(result));
		}
		m_cSamplerInfo = samplerInfo;
	}
}
Vulkan::VkManagedSampler::operator VkSampler() const
{
	return m_sampler;
}
