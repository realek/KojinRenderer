#include "VkManagedDescriptorPool.h"
#include "VkManagedDevice.h"
#include "VkManagedDescriptorSet.h"
#include <assert.h>

Vulkan::VkManagedDescriptorSet Vulkan::VkManagedDescriptorPool::AllocateDescriptorSet(const VkDevice& device, uint32_t setCount, VkDescriptorSetLayout layout)
{
	if(m_setAllocation.currentlyAllocated == m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: All sets within the pool have been allocated.");
	if (m_setAllocation.currentlyAllocated + setCount > m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: Not enough available sets within the pool.");

	std::vector<VkDescriptorSet> descriptors;
	descriptors.resize(setCount);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool.object();
	allocInfo.descriptorSetCount = setCount;
	std::vector<VkDescriptorSetLayout> layouts(setCount, layout);
	allocInfo.pSetLayouts = layouts.data();
	vkAllocateDescriptorSets(device, &allocInfo, descriptors.data());
	Vulkan::VkManagedDescriptorSet descriptorSet(device, m_descriptorPool.object(), descriptors, m_descriptorCounts);
	m_setAllocation.currentlyAllocated += setCount;
	return descriptorSet;
}

void Vulkan::VkManagedDescriptorPool::FreeDescriptorSet(const VkDevice& device, VkManagedDescriptorSet * descSet)
{
	assert(descSet != nullptr);
	assert(descSet->m_pool != VK_NULL_HANDLE);
	if (descSet->m_pool != m_descriptorPool.object())
		throw std::runtime_error("Descriptor set allocated from a different pool or the allocating pool was destroyed.");

	descSet->m_writes.clear();
	uint32_t setSize = static_cast<uint32_t>(descSet->m_internalSets.size());
	m_setAllocation.currentlyAllocated -= setSize;
	VkResult result = vkFreeDescriptorSets(device, descSet->m_pool, setSize, descSet->m_internalSets.data());
	assert(result == VK_SUCCESS);
	descSet->m_internalSets.clear();


}

void Vulkan::VkManagedDescriptorPool::SetDescriptorCount(VkDescriptorType desc, uint32_t count)
{
	assert(m_descriptorPool.object() == VK_NULL_HANDLE);
	m_descriptorCounts[desc] = count;
}

void Vulkan::VkManagedDescriptorPool::ClearPool()
{
	for (uint32_t& desc : m_descriptorCounts)
		desc = 0;
	m_descriptorPool;
	m_setAllocation.maxAllocations = 0;
	m_setAllocation.currentlyAllocated = 0;
}

uint32_t Vulkan::VkManagedDescriptorPool::Size() 
{
	return m_setAllocation.maxAllocations;
}

void Vulkan::VkManagedDescriptorPool::BuildPool(const VkDevice& device, uint32_t maxSets)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	poolSizes.reserve(11);
	for(uint32_t i = 0; i < VkDescriptorType::VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
	{
		if(m_descriptorCounts[i] > 0 )
		{
			VkDescriptorPoolSize ps = {};
			ps.type = static_cast<VkDescriptorType>(i);
			ps.descriptorCount = m_descriptorCounts[i]*maxSets;
			poolSizes.push_back(ps);
		}

	}
	VkDescriptorPoolCreateInfo poolCI = {};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = maxSets;
	poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	m_setAllocation.currentlyAllocated = 0;
	m_setAllocation.maxAllocations = maxSets;

	VkDescriptorPool pool = VK_NULL_HANDLE;
	VkResult result = vkCreateDescriptorPool(device, &poolCI, nullptr, &pool);
	assert(result == VK_SUCCESS);
	m_descriptorPool.set_object(pool, device, vkDestroyDescriptorPool);
}