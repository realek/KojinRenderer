#include "VkManagedDescriptorSet.h"
#include "VkManagedBuffer.h"
#include "VkManagedImage.h"
#include <assert.h>


void Vulkan::VkManagedDescriptorSet::LoadCombinedSamplerImageArray(uint32_t dstSetIndex, std::vector<VkManagedImage*> images, uint32_t dstBind, std::vector<VkSampler> samplers)
{
	if (m_descriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] == m_totalDescriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER])
	{
		throw std::runtime_error("Maximum number for descriptors of type VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER has been reached. Unable to load more descriptors");
	}
	uint32_t imageCount = static_cast<uint32_t>(images.size());
	if(imageCount > 0 )
	{
		uint32_t samplerCount = static_cast<uint32_t>(samplers.size());
		assert(imageCount == samplerCount);

		std::vector<VkDescriptorImageInfo> combinedSamplerImages(images.size());

		for(uint32_t i = 0; i < imageCount; ++i)
		{
			VkDescriptorImageInfo textureInfo = {};
			textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureInfo.imageView = *images[i];
			textureInfo.sampler = samplers[i];
			combinedSamplerImages[i] = textureInfo;
		}
		

		VkWriteDescriptorSet descriptorVertexWrite = {};
		{
			descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorVertexWrite.dstSet = m_internalSets[dstSetIndex];
			descriptorVertexWrite.dstBinding = dstBind;
			descriptorVertexWrite.dstArrayElement = 0;
			descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorVertexWrite.descriptorCount = imageCount;
			descriptorVertexWrite.pImageInfo = combinedSamplerImages.data();
		}
		m_writes[dstSetIndex].push_back(descriptorVertexWrite);
		m_descriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER]++;
	}
	else
	{
		throw std::runtime_error("Atleast one image and one sampler must be provied in order to a call of LoadCompiledSamplerImageArray.");
	}

}

void Vulkan::VkManagedDescriptorSet::LoadCombinedSamplerImage(uint32_t dstSetIndex, VkManagedImage * image, uint32_t dstBind, VkSampler sampler)
{
	if (m_descriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] == m_totalDescriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER])
	{
		throw std::runtime_error("Maximum number for descriptors of type VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER has been reached. Unable to load more descriptors");
	}
	VkDescriptorImageInfo diffuseTextureInfo = {};
	diffuseTextureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	diffuseTextureInfo.imageView = *image;
	diffuseTextureInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = m_internalSets[dstSetIndex];
		descriptorVertexWrite.dstBinding = dstBind;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pImageInfo = &diffuseTextureInfo;
	}
	m_writes[dstSetIndex].push_back(descriptorVertexWrite);
	m_descriptorCounts[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER];
}

void Vulkan::VkManagedDescriptorSet::LoadUniformBuffer(uint32_t dstSetIndex, VkManagedBuffer * buffer, uint32_t dstBind)
{
	if (m_descriptorCounts[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] == m_totalDescriptorCounts[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER])
	{
		throw std::runtime_error("Maximum number for descriptors of type VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER has been reached. Unable to load more descriptors");
	}
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = *buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = buffer->Size();

	VkWriteDescriptorSet descriptorVertexWrite = {};
	{
		descriptorVertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorVertexWrite.dstSet = m_internalSets[dstSetIndex];
		descriptorVertexWrite.dstBinding = dstBind;
		descriptorVertexWrite.dstArrayElement = 0;
		descriptorVertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorVertexWrite.descriptorCount = 1;
		descriptorVertexWrite.pBufferInfo = &bufferInfo;
	}
	m_writes[dstSetIndex].push_back(descriptorVertexWrite);
	m_descriptorCounts[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER]++;
}

void Vulkan::VkManagedDescriptorSet::ClearSetsWrites()
{
	for (std::vector<VkWriteDescriptorSet>& writes : m_writes)
	{
		writes.clear();
	}
	for(uint32_t i = 0; i < VkDescriptorType::VK_DESCRIPTOR_TYPE_RANGE_SIZE;++i)
	{
		m_descriptorCounts[i] = 0;
	}
}

void Vulkan::VkManagedDescriptorSet::ClearSetWrites(uint32_t setIndex)
{
	uint32_t buffers = 0;
	uint32_t images = 0;
	for(VkWriteDescriptorSet write : m_writes[setIndex])
	{
		m_descriptorCounts[write.descriptorType] --;
	}
	m_writes[setIndex].clear();

}

void Vulkan::VkManagedDescriptorSet::WriteSet(uint32_t setIndex)
{
	uint32_t count = static_cast<uint32_t>(m_writes[setIndex].size());
	if(count > 0)
	{
		vkUpdateDescriptorSets(m_device, count, m_writes[setIndex].data(), 0, nullptr);
	}
}

void Vulkan::VkManagedDescriptorSet::WriteSets()
{
	for(std::vector<VkWriteDescriptorSet>& writes : m_writes)
	{
		uint32_t count = static_cast<uint32_t>(writes.size());
		if (count > 0)
		{
			vkUpdateDescriptorSets(m_device, count, writes.data(), 0, nullptr);
		}
		writes.clear();
	}
}

VkDescriptorSet Vulkan::VkManagedDescriptorSet::Set(uint32_t index)
{
	return m_internalSets[index];
}

size_t Vulkan::VkManagedDescriptorSet::Size()
{
	return m_internalSets.size();
}

Vulkan::VkManagedDescriptorSet::VkManagedDescriptorSet(VkDevice device, VkDescriptorPool pool, std::vector<VkDescriptorSet> sets, uint32_t maxDescriptorCounts[11])
{
	m_device = device;
	m_pool = pool;
	m_internalSets = sets;
	uint32_t setCount = static_cast<uint32_t>(sets.size());
	std::memcpy(m_totalDescriptorCounts, maxDescriptorCounts, sizeof(uint32_t) * 11);
	for(uint32_t i = 0; i < 11; ++i)
	{
		m_totalDescriptorCounts[i] *= setCount;
	}
}
