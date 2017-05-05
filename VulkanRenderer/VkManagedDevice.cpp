#include "VkManagedDevice.h"
#include "VkManagedInstance.h"
#include "VkManagedQueue.h"
#include <assert.h>

const std::vector<VkFormat> k_depthFormats{
	VK_FORMAT_D32_SFLOAT_S8_UINT,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_D16_UNORM_S8_UINT,
	VK_FORMAT_D16_UNORM
};

Vulkan::VkManagedDevice::VkManagedDevice(VkDeviceCreateInfo createInfo, VkPhysicalDeviceData * physicalDevice)
{

	if (vkCreateDevice(physicalDevice->device, &createInfo, nullptr, ++m_device) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create Vulkan logical device from provided physical device.");
	}
	m_physicalDevice = physicalDevice;

	try
	{
		m_depthFormat = FindDepthFormat();
	}
	catch (...)
	{
		throw;
	}

	GetAllQueues();
}

Vulkan::VkManagedDevice::~VkManagedDevice()
{
	if(!m_queues.empty())
	{
		for (auto& qV : m_queues)
		{
			for (VkManagedQueue* q : qV)
			{
				delete(q);
			}
		}
	}
}

Vulkan::VkManagedDevice::operator VkDevice() const
{
	return m_device;
}

uint32_t Vulkan::VkManagedDevice::GetMemoryType(uint32_t desiredType, VkMemoryPropertyFlags memFlags) {

	uint32_t memtype = -1;
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice->device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		if ((desiredType & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & memFlags) == memFlags) {
			memtype = i;
			break;
		}
	}

	//Fails if no memory is found
	assert(memtype!=-1);
	return memtype;
}

VkPhysicalDevice Vulkan::VkManagedDevice::PhysicalDevice()
{
	return m_physicalDevice->device;
}

VkPhysicalDeviceLimits Vulkan::VkManagedDevice::GetPhysicalDeviceLimits()
{
	return m_physicalDevice->deviceProperties.limits;
}

VkFormat Vulkan::VkManagedDevice::Depthformat()
{
	return m_depthFormat;
}

void Vulkan::VkManagedDevice::WaitForIdle()
{
	VkResult result = vkDeviceWaitIdle(m_device);
	if (result != VK_SUCCESS)
		throw std::runtime_error("VkManagedDevice failed while waiting on logical device. Reason: " + VkResultToString(result));
}

Vulkan::VkSurfaceData Vulkan::VkManagedDevice::GetPhysicalDeviceSurfaceData()
{
	return m_physicalDevice->deviceSurfaceData;
}

const Vulkan::VkPhysicalDeviceData * Vulkan::VkManagedDevice::GetPhysicalDeviceData()
{
	return m_physicalDevice;
}

Vulkan::VkManagedQueue * Vulkan::VkManagedDevice::GetQueue(VkQueueFlags bits, VkBool32 present, VkBool32 fullMatch, VkBool32 markUsed)
{
	size_t qFsize = m_physicalDevice->queueFamilies.size();
	for (size_t i = 0; i < qFsize; ++i)
	{
		bool flagsFound = false;
		if (fullMatch)
		{
			if (m_physicalDevice->queueFamilies[i].queueFlags == bits)
				flagsFound = true;
		}
		else
		{
			if (m_physicalDevice->queueFamilies[i].queueFlags & bits)
				flagsFound = true;
		}
		if (present && m_physicalDevice->presentFamilies[i] != i)
			flagsFound = false;

		if (flagsFound)
		{
			size_t qSize = m_queues[i].size();
			for(size_t j = 0; j < qSize; ++j)
			{
				if (!m_queues[i][j]->m_used)
				{
					if(markUsed)
						m_queues[i][j]->m_used = true;

					return m_queues[i][j];
				}
			}
		}

	}
	return nullptr;
}

void Vulkan::VkManagedDevice::UnmarkQueue(VkManagedQueue * queue)
{
	m_queues[queue->familyIndex][queue->queueIndex]->m_used = false;
}

void Vulkan::VkManagedDevice::UnmarkAllQueues()
{
	size_t qFsize = m_physicalDevice->queueFamilies.size();
	for (size_t i = 0; i < qFsize; ++i)
	{
		size_t qSize = m_queues[i].size();
		for (size_t j = 0; j < qSize; ++j)
		{
			m_queues[i][j]->m_used = false;
		}
	}
}

bool Vulkan::VkManagedDevice::CheckFormatFeature(VkFormatFeatureFlags feature, VkFormat format, VkImageTiling tiling)
{
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(m_physicalDevice->device, format, &props);
	switch (tiling)
	{
	case VK_IMAGE_TILING_OPTIMAL:
		if (props.optimalTilingFeatures & feature)
			return true;
		else
			return false;

	case VK_IMAGE_TILING_LINEAR:
		if (props.linearTilingFeatures & feature)
			return true;
		else
			return false;
	default:
		throw std::invalid_argument("Invalid tiling argument has been passed to the device.");
		break;
	}
}

VkFormat Vulkan::VkManagedDevice::FindDepthFormat()
{
	for (VkFormat format : k_depthFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice->device, format, &props);

		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			return format;

	}

	throw std::runtime_error("Unable to find desired depth format");
}

void Vulkan::VkManagedDevice::GetAllQueues()
{
	uint32_t size = static_cast<uint32_t>(m_physicalDevice->queueFamilies.size());
	m_queues.resize(size);
	for (uint32_t i = 0; i < size; ++i) 
	{
		VkBool32 canPresent = VK_FALSE;
		if (i < m_physicalDevice->presentFamilies.size() && i == m_physicalDevice->presentFamilies[i])
			canPresent = VK_TRUE;

		
		std::vector<VkManagedQueue*> queues;
		queues.resize(m_physicalDevice->queueFamilies[i].queueCount,nullptr);
		for(uint32_t j = 0; j < m_physicalDevice->queueFamilies[i].queueCount;++j)
		{
			VkQueue queue;
			vkGetDeviceQueue(m_device, i, j, &queue);
			queues[j] = new VkManagedQueue(queue, i, j, canPresent);
		}
		m_queues[i] = queues;
	}

}