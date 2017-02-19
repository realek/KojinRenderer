#define NOMINMAX
#include "VulkanSystem.h"
#include <SDL2\SDL_syswm.h>
#include <stdexcept>
#include <algorithm>
#include <set>

Vulkan::VulkanSystem::VulkanSystem(SDL_Window * window, const char * appName, int appVer[3], const char * engineName, int engineVer[3])
{

	if (window == nullptr)
		throw std::runtime_error("SDL_Window is null.");

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(appVer[0], appVer[1], appVer[2]);
	appInfo.pEngineName = engineName;
	appInfo.engineVersion = VK_MAKE_VERSION(engineVer[0], engineVer[1], engineVer[2]);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	this->CreateVulkanInstance(&appInfo);
	this->CreateVulkanSurface(window);
	this->GetPhysicalDevices();

}

void Vulkan::VulkanSystem::Initialize(int physicalDeviceId, VkPhysicalDeviceRequiredQueues * queues, int screenWidth, int screenHeight)
{
	this->CreateLogicalDeviceFromSelectedPhysicalDevice(physicalDeviceId,queues);
	this->m_width = screenWidth;
	this->m_height = screenHeight;

}
void Vulkan::VulkanSystem::GetScreenSizes(int & width, int & height)
{
	width = this->m_width;
	height = this->m_height;
}

VkDevice Vulkan::VulkanSystem::GetLogicalDevice()
{
	return m_currentLogicalDevice;
}

VkPhysicalDevice Vulkan::VulkanSystem::GetCurrentPhysical()
{
	return m_selectedPhysicalDevice;
}

VkPhysicalDeviceProperties Vulkan::VulkanSystem::GetCurrentPhysicalProperties()
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(m_selectedPhysicalDevice, &deviceProperties);
	return deviceProperties;
}


VkInstance Vulkan::VulkanSystem::GetInstance()
{
	return m_vulkanInstance;
}

VkSurfaceKHR Vulkan::VulkanSystem::GetSurface()
{
	return m_vulkanSurface;
}

Vulkan::VkQueueFamilyIDs Vulkan::VulkanSystem::GetQueueFamilies()
{
	return m_selectedPhysicalDeviceQueueIds;
}

Vulkan::VkQueueContainer Vulkan::VulkanSystem::GetQueues()
{
	return m_currentLogicalDeviceQueues;
}

Vulkan::VkSwapChainSupportData * Vulkan::VulkanSystem::GetSwapChainSupportData()
{
	return &m_selectedPhysicalDeviceSCData;
}

VkFormat Vulkan::VulkanSystem::GetDepthFormat()
{
	for (VkFormat format : k_depthFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_selectedPhysicalDevice, format, &props);

		if (k_depthStencilTiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			return format;
			break;
		}

	}

	throw std::runtime_error("Unable to find desired depth format");
}



bool Vulkan::VulkanSystem::ReportValidationLayers()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> properties(layerCount);
	if (layerCount == 0)
		return false;
	vkEnumerateInstanceLayerProperties(&layerCount, properties.data());

	for (const char* layerName : validationLayers) 
	{
		bool found = false;
		for (const auto& p : properties)
		{
			if (strcmp(p.layerName,layerName)==0)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;

}

const std::vector<const char*> Vulkan::VulkanSystem::GetExtensions()
{
	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_WIN32_KHR
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_WIN32_KHR


	if (enableValidationLayers)
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return extensions;
}

void Vulkan::VulkanSystem::CreateVulkanInstance(const VkApplicationInfo * appInfo)
{
	if (enableValidationLayers && !ReportValidationLayers())
		throw std::runtime_error("Reported validation layers did not match, requested layers");

	VkInstanceCreateInfo instanceCI = {};
	instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCI.pApplicationInfo = appInfo;
	auto extensions = GetExtensions();
	instanceCI.enabledExtensionCount = extensions.size();
	instanceCI.ppEnabledExtensionNames = extensions.data();
	if (enableValidationLayers) {
		instanceCI.enabledLayerCount = validationLayers.size();
		instanceCI.ppEnabledLayerNames = validationLayers.data();
	}
	else
		instanceCI.enabledLayerCount = 0;

	if(vkCreateInstance(&instanceCI, nullptr, ++m_vulkanInstance)!= VK_SUCCESS)
		throw std::runtime_error("Unable to create Vulkan instance.");
}

void Vulkan::VulkanSystem::CreateVulkanSurface(SDL_Window* window)
{
	if (m_vulkanInstance.Get() == VK_NULL_HANDLE)
		throw std::runtime_error("Vulkan instance was not created. Aborting...");

	SDL_SysWMinfo windowInfo;
	SDL_VERSION(&windowInfo.version);
	if (!SDL_GetWindowWMInfo(window, &windowInfo)) {
		throw std::runtime_error("SDL2 Window Manager info couldn't be retrieved.");
	}

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfaceCI = {};
	surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCI.hinstance = GetModuleHandle(NULL);
	surfaceCI.hwnd = windowInfo.info.win.window;

	if (vkCreateWin32SurfaceKHR(m_vulkanInstance, &surfaceCI, nullptr, ++m_vulkanSurface) != VK_SUCCESS)
		throw std::runtime_error("Unable to create Win32 KHR Surface!");
#endif // _WIN32


}

inline void Vulkan::VulkanSystem::GetPhysicalDevices()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_vulkanInstance, &count, nullptr);

	if (count == 0)
		throw std::runtime_error("No GPUs with Vulkan support were found on the current system");

	systemDevices.resize(count);
	vkEnumeratePhysicalDevices(m_vulkanInstance, &count, systemDevices.data());
}

Vulkan::VkQueueFamilyIDs Vulkan::VulkanSystem::GetPhysicalDeviceQueueFamilies(VkPhysicalDevice device, VkPhysicalDeviceRequiredQueues * reqs)
{
	VkQueueFamilyIDs indices;

	uint32_t queueFamCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {

		if (reqs->hasGraphicsQueue && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (reqs->hasComputeQueue && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
			indices.computeFamily = i;

		if (reqs->hasTransferQueue && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			indices.transferFamily = i;

		if (reqs->hasSparseBindingQueue && queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			indices.transferFamily = i;

		if(reqs->hasPresentQueue)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_vulkanSurface, &presentSupport);

			if (queueFamily.queueCount > 0 && presentSupport)
				indices.presentFamily = i;
		}

		if (indices.Validate(reqs))
			break;

		i++;
	}

	return indices;

}

bool Vulkan::VulkanSystem::CheckPhysicalDeviceExtensions(const VkPhysicalDevice device)
{
	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, extensions.data());

	std::vector<std::string> reqExts(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& ext : extensions) {
		reqExts.erase(std::remove(reqExts.begin(), reqExts.end(), ext.extensionName), reqExts.end());
	}

	return reqExts.empty();
}

Vulkan::VkSwapChainSupportData Vulkan::VulkanSystem::GetPhysicalDeviceSwapChainSupport(const VkPhysicalDevice device)
{
	VkSwapChainSupportData data;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_vulkanSurface, &data.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vulkanSurface, &formatCount, nullptr);

	if (formatCount > 0) {
		data.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_vulkanSurface, &formatCount, data.formats.data());
	}


	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vulkanSurface, &presentModeCount, nullptr);

	if (presentModeCount > 0) {
		data.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_vulkanSurface, &presentModeCount, data.presentModes.data());
	}

	return data;
}

inline void Vulkan::VulkanSystem::CreateLogicalDeviceFromSelectedPhysicalDevice(int physicalDeviceId, VkPhysicalDeviceRequiredQueues * reqQueues)
{
	VkPhysicalDeviceRequiredQueues * required = &m_defaultRequiredQueues;
	if (reqQueues != nullptr)
		required = reqQueues;
	//select best gpu available matching default requirements
	if (physicalDeviceId == -1)
	{
		
		for (VkPhysicalDevice dev : systemDevices)
		{
			
			auto ids = GetPhysicalDeviceQueueFamilies(dev, required);
			if (ids.Validate(required) && CheckPhysicalDeviceExtensions(dev))
			{
				auto data = GetPhysicalDeviceSwapChainSupport(dev);
				if (data.Validate())
				{
					m_selectedPhysicalDevice = dev;
					m_selectedPhysicalDeviceQueueIds = ids;
					m_selectedPhysicalDeviceSCData = data;
					break;
				}

			}
		}

		if (m_selectedPhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("No suitable GPU was detected on the system.");

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { m_selectedPhysicalDeviceQueueIds.graphicsFamily, m_selectedPhysicalDeviceQueueIds.presentFamily };
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCI = {};
			queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCI.queueFamilyIndex = queueFamily;
			queueCI.queueCount = 1;
			queueCI.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCI);
		}

		VkPhysicalDeviceFeatures features = {};

		VkDeviceCreateInfo deviceCI = {};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		deviceCI.pQueueCreateInfos = queueCreateInfos.data();
		deviceCI.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

		deviceCI.pEnabledFeatures = &features;

		deviceCI.enabledExtensionCount = deviceExtensions.size();
		deviceCI.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCI.enabledLayerCount = 0;

		if (vkCreateDevice(m_selectedPhysicalDevice, &deviceCI, nullptr, ++m_currentLogicalDevice) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create Vulkan logical device from provided GPU.");
		}

		if (required->hasGraphicsQueue)
			vkGetDeviceQueue(m_currentLogicalDevice, m_selectedPhysicalDeviceQueueIds.graphicsFamily, 0, &m_currentLogicalDeviceQueues.graphicsQueue);
		if (required->hasPresentQueue)
			vkGetDeviceQueue(m_currentLogicalDevice, m_selectedPhysicalDeviceQueueIds.presentFamily, 0, &m_currentLogicalDeviceQueues.presentQueue);
		if (required->hasComputeQueue)
			vkGetDeviceQueue(m_currentLogicalDevice, m_selectedPhysicalDeviceQueueIds.computeFamily, 0, &m_currentLogicalDeviceQueues.computeQueue);
		if (required->hasTransferQueue)
			vkGetDeviceQueue(m_currentLogicalDevice, m_selectedPhysicalDeviceQueueIds.transferFamily, 0, &m_currentLogicalDeviceQueues.transferQueue);
		if (required->hasSparseBindingQueue)
			vkGetDeviceQueue(m_currentLogicalDevice, m_selectedPhysicalDeviceQueueIds.sparseBindingFamily, 0, &m_currentLogicalDeviceQueues.sparseBindingQueue);

	}
	else
	{
		if (physicalDeviceId < -1 && physicalDeviceId >(int)systemDevices.size())
			throw std::runtime_error("Incorrect GPU id provided.");

		//Add GPU selection code here
	}
}


