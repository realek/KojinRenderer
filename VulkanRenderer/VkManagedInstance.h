#pragma once
#include "VulkanObject.h"
#include "VkManagedStructures.h"
#include <vector>
#include <memory>
namespace Vulkan
{
	const std::vector<const char*> minDeviceRequiredExtentions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	class VkManagedDevice;
	class VkManagedInstance
	{

	public:
		VkManagedInstance(VkApplicationInfo * appInfo, std::vector<const char*> layers);
		VkManagedInstance(const VkManagedInstance&) = delete;
		VkManagedInstance & operator=(const VkManagedInstance&) = delete;
		~VkManagedInstance();
		VkInstance Instance();
		VkSurfaceKHR Surface();
		VkSurfaceKHR MakeSurface(void * window, uint32_t width, uint32_t height);
		VkManagedDevice * CreateVkManagedDevice(size_t physDeviceIndex, std::vector<const char*> requiredExtensions, bool fPresent, bool fGraphics, bool fTransfer, bool fCompute, bool fSparse);
	private:
		void AcquirePhysicalDevices();
		VkResult CheckAndGetDeviceExtensions(VkPhysicalDevice physDevice, std::vector<const char*>& requiredExtentions);
		const std::vector<const char*> GetInstanceExtensions();

	private:

		VkManagedObject<VkInstance> m_instance;
		VkManagedObject<VkSurfaceKHR> m_surface;
		std::vector<VkPhysicalDeviceData*> devices;
	};
}