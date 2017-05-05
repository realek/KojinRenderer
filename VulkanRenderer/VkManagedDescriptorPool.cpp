#include "VkManagedDescriptorPool.h"
#include "VkManagedDevice.h"
#include "VkManagedDescriptorSet.h"
#include <assert.h>

Vulkan::VkManagedDescriptorPool::VkManagedDescriptorPool(VkManagedDevice * device)
{
	assert(device != nullptr);
	m_device = *device;
}

Vulkan::VkManagedDescriptorSet Vulkan::VkManagedDescriptorPool::AllocateDescriptorSet(uint32_t setCount, VkDescriptorSetLayout layout)
{
	if(m_setAllocation.currentlyAllocated == m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: All sets within the pool have been allocated.");
	if (m_setAllocation.currentlyAllocated + setCount > m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: Not enough available sets within the pool.");

	std::vector<VkDescriptorSet> descriptors;
	descriptors.resize(setCount);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = &layout;
	vkAllocateDescriptorSets(m_device, &allocInfo, descriptors.data());
	Vulkan::VkManagedDescriptorSet descriptorSet(m_device, m_descriptorPool, descriptors, m_descriptorCounts);
	m_setAllocation.currentlyAllocated += setCount;
	return descriptorSet;
}

void Vulkan::VkManagedDescriptorPool::AllocateDescriptorSet(uint32_t setCount, VkDescriptorSetLayout layout, VkManagedDescriptorSet *& descriptorSet)
{
	if (m_setAllocation.currentlyAllocated == m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: All sets within the pool have been allocated.");
	if (m_setAllocation.currentlyAllocated + setCount > m_setAllocation.maxAllocations)
		throw std::runtime_error("Unable to allocate sets, reason: Not enough available sets within the pool.");

	if (descriptorSet != nullptr)
	{
		if(descriptorSet->m_pool == m_descriptorPool && m_device == descriptorSet->m_device)
		{
			FreeDescriptorSet(descriptorSet);

		}
		delete descriptorSet;
	}

	std::vector<VkDescriptorSet> descriptors;
	descriptors.resize(setCount);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = setCount;
	allocInfo.pSetLayouts = &layout;
	vkAllocateDescriptorSets(m_device, &allocInfo, descriptors.data());
	descriptorSet = new VkManagedDescriptorSet(m_device,m_descriptorPool,descriptors,m_descriptorCounts);
	m_setAllocation.currentlyAllocated += setCount;
}

void Vulkan::VkManagedDescriptorPool::FreeDescriptorSet(VkManagedDescriptorSet * descSet)
{
	if (descSet == nullptr)
		throw std::runtime_error("Descriptor set is null.");
	else if (descSet->m_pool != m_descriptorPool)
		throw std::runtime_error("Descriptor set allocated from a different pool or the allocating pool was destroyed.");

	descSet->m_writes.clear();
	uint32_t setSize = static_cast<uint32_t>(descSet->m_internalSets.size());
	m_setAllocation.currentlyAllocated -= setSize;
	VkResult result = vkFreeDescriptorSets(m_device, m_descriptorPool, setSize, descSet->m_internalSets.data());
	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to free VkManagedDescriptor set. Reason:" + VkResultToString(result));
	descSet->m_internalSets.clear();


}

void Vulkan::VkManagedDescriptorPool::SetDescriptorCount(VkDescriptorType desc, uint32_t count)
{
	assert(m_descriptorPool == VK_NULL_HANDLE);
	m_descriptorCounts[desc] = count;
}

void Vulkan::VkManagedDescriptorPool::ClearPool()
{
	for (uint32_t& desc : m_descriptorCounts)
		desc = 0;
	m_descriptorPool = VK_NULL_HANDLE;
	m_setAllocation.maxAllocations = 0;
	m_setAllocation.currentlyAllocated = 0;
}

uint32_t Vulkan::VkManagedDescriptorPool::Size() 
{
	return m_setAllocation.maxAllocations;
}

void Vulkan::VkManagedDescriptorPool::BuildPool(uint32_t maxSets)
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

	VkResult result = vkCreateDescriptorPool(m_device, &poolCI, nullptr, ++m_descriptorPool);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Unable to create VkManagedDescriptorPool. Reason: " + Vulkan::VkResultToString(result));

}