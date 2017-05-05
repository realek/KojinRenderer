#pragma once
#include "VulkanObject.h"
#include "VkManagedStructures.h"
#include <vector>
#include <memory>
namespace Vulkan
{
	class VkManagedQueue;
	class VkManagedInstance;
	class VkManagedDevice
	{

	public:
		~VkManagedDevice();
		VkManagedDevice(const VkManagedDevice&) = delete;
		VkManagedDevice & operator=(const VkManagedDevice&) = delete;
		operator VkDevice() const;
		uint32_t GetMemoryType(uint32_t desiredType, VkMemoryPropertyFlags memFlags);
		VkPhysicalDevice PhysicalDevice();
		VkPhysicalDeviceLimits GetPhysicalDeviceLimits();
		VkFormat Depthformat();
		void WaitForIdle();
		VkSurfaceData GetPhysicalDeviceSurfaceData();
		const VkPhysicalDeviceData * GetPhysicalDeviceData();
		Vulkan::VkManagedQueue * GetQueue(VkQueueFlags bits, VkBool32 present, VkBool32 fullMatch, VkBool32 markUsed);
		void UnmarkQueue(VkManagedQueue * queue);
		void UnmarkAllQueues();
		bool CheckFormatFeature(VkFormatFeatureFlags feature, VkFormat format, VkImageTiling tiling);


	private:
		VkFormat FindDepthFormat();
		void GetAllQueues();
		VkManagedDevice(VkDeviceCreateInfo createInfo, VkPhysicalDeviceData * physicalDevice);
		friend class VkManagedInstance;

	private:
		VulkanObjectContainer<VkDevice> m_device{vkDestroyDevice};
		std::vector<std::vector<VkManagedQueue*>> m_queues;
		VkFormat m_depthFormat;
		VkPhysicalDeviceData * m_physicalDevice = nullptr;

	};
}