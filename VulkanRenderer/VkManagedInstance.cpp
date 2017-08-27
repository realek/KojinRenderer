#include "VkManagedInstance.h"
#include "VkManagedValidationLayers.h"
#include "VkManagedDevice.h"
#include <vector>
#include <array>
#include <assert.h>
#include <set>
#include <SDL2\SDL_syswm.h>

Vulkan::VkManagedInstance::VkManagedInstance(VkApplicationInfo * appInfo, std::vector<const char*> layers)
{
	VkResult result;
	VkInstanceCreateInfo instanceCI = {};
	instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCI.pApplicationInfo = appInfo;
	auto extensions = GetInstanceExtensions();
	instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCI.ppEnabledExtensionNames = extensions.data();
	instanceCI.pNext = nullptr;
	instanceCI.flags = 0;
#ifndef NDEBUG
	VkManagedValidationLayers vLayers;
	assert(vLayers.SelectLayers(layers) != VK_SUCCESS);

	instanceCI.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instanceCI.ppEnabledLayerNames = layers.data();
#else
	instanceCI.enabledLayerCount = 0;
#endif // !NDEBUG

	VkInstance instance = VK_NULL_HANDLE;
	result = vkCreateInstance(&instanceCI, nullptr, &instance);
	assert(result == VK_SUCCESS);
	m_instance.set_object(instance, vkDestroyInstance);

	// acquire devices and data
	try
	{
		AcquirePhysicalDevices();
	}
	catch (...)
	{
		throw;
	}
}

Vulkan::VkManagedInstance::~VkManagedInstance()
{
	for (size_t i = 0; i < devices.size(); ++i)
		delete(devices[i]);

}

VkInstance Vulkan::VkManagedInstance::Instance()
{
	return m_instance.object();
}

VkSurfaceKHR Vulkan::VkManagedInstance::Surface()
{
	//assert(m_surface != VK_NULL_HANDLE);
	return m_surface.object();
}

VkSurfaceKHR Vulkan::VkManagedInstance::MakeSurface(void * window, uint32_t width, uint32_t height)
{
	VkResult result;
	SDL_SysWMinfo windowInfo;
	SDL_VERSION(&windowInfo.version);
	SDL_Window * win = reinterpret_cast<SDL_Window*>(window);
	assert(win != nullptr);

	if (!SDL_GetWindowWMInfo(win, &windowInfo)) {
		throw std::runtime_error("SDL2 Window Manager info couldn't be retrieved.");
	}
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfaceCI = {};
	surfaceCI.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCI.hinstance = GetModuleHandle(NULL);
	surfaceCI.hwnd = windowInfo.info.win.window;
	
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	result = vkCreateWin32SurfaceKHR(m_instance.object(), &surfaceCI, nullptr, &surface);
	assert(result == VK_SUCCESS);
	m_surface.set_object(surface, m_instance.object(), vkDestroySurfaceKHR);

#endif // _WIN32
	//TODO: add linux & mac here later


	//get surface capabilities for all physical devices
	for(VkPhysicalDeviceData* deviceData : devices)
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(deviceData->device, m_surface.object(), &deviceData->deviceSurfaceData.capabilities);
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(deviceData->device, m_surface.object(), &count, nullptr);
		if (count != 0)
		{
			deviceData->deviceSurfaceData.formats.resize(count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(deviceData->device, m_surface.object(), &count, deviceData->deviceSurfaceData.formats.data());
		}
		count = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(deviceData->device, m_surface.object(), &count, nullptr);
		if (count != 0)
		{
			deviceData->deviceSurfaceData.presentModes.resize(count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(deviceData->device, m_surface.object(), &count, deviceData->deviceSurfaceData.presentModes.data());
		}
		deviceData->deviceSurfaceData.surface = m_surface.object();
		deviceData->deviceSurfaceData.windowExtent.height = height;
		deviceData->deviceSurfaceData.windowExtent.width = width;
	}

	return m_surface.object();
}

Vulkan::VkManagedDevice * Vulkan::VkManagedInstance::CreateVkManagedDevice(size_t physDeviceIndex, std::vector<const char*> requiredExtensions, bool fPresent,bool fGraphics, bool fTransfer, bool fCompute, bool fSparse)
{
	//a surface must exist before creating a logical device
	//assert(m_surface != VK_NULL_HANDLE);

	//get queues
	uint32_t queueFamCount = 0;
	{
		vkGetPhysicalDeviceQueueFamilyProperties(devices[physDeviceIndex]->device, &queueFamCount, nullptr);
		devices[physDeviceIndex]->queueFamilies.resize(queueFamCount);
		vkGetPhysicalDeviceQueueFamilyProperties(devices[physDeviceIndex]->device, &queueFamCount, devices[physDeviceIndex]->queueFamilies.data());

		uint32_t index = 0;
		VkBool32 graphicsSupport = false;
		VkBool32 computeSupport = false;
		VkBool32 transferSupport = false;
		VkBool32 sparseBindSupport = false;

		for(VkQueueFamilyProperties& queueFam : devices[physDeviceIndex]->queueFamilies)
		{
			if (queueFam.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsSupport = true;
				//devices[physDeviceIndex].deviceQueueIndices.graphicsQueues.push_back(index);
			}

			if (queueFam.queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				computeSupport = true;
				//devices[physDeviceIndex].deviceQueueIndices.computeQueues.push_back(index);
			}

			if (queueFam.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				transferSupport = true;
				//devices[physDeviceIndex].deviceQueueIndices.transferQueues.push_back(index);
			}

			if (queueFam.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			{
				sparseBindSupport = true;
				//devices[physDeviceIndex].deviceQueueIndices.sparseBindQueues.push_back(index);
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(devices[physDeviceIndex]->device, index, m_surface.object(), &presentSupport);
			if (presentSupport)
			{
				devices[physDeviceIndex]->presentFamilies.push_back(index);
			}

			index++;

		}

		if (fGraphics && !graphicsSupport)
			throw std::invalid_argument("Force graphics queue was enabled but selected device has no graphics queue");

		if(fPresent)
		{
			if (devices[physDeviceIndex]->presentFamilies.empty())
				throw std::invalid_argument("Force present queue was enabled but selected device has no present queue");
		}

		if(fCompute && !computeSupport)
			throw std::invalid_argument("Force compute queue was enabled but selected device has no compute queue");

		if(fTransfer && !transferSupport)
			throw std::invalid_argument("Force transfer queue was enabled but selected device has no transfer queue");

		if(fSparse && !sparseBindSupport)
			throw std::invalid_argument("Force sparse binding queue was enabled but selected device has no sparse binding queue");
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<std::vector<float>> queuePriosByFamily;

	for(uint32_t qIndex = 0; qIndex < queueFamCount;++qIndex)
	{
		uint32_t queueCount = devices[physDeviceIndex]->queueFamilies[qIndex].queueCount;
		queuePriosByFamily.push_back(std::vector<float>(queueCount));
		float priorityFactor = 1.0f / queueCount;
		//populate queue priorities
		for(uint32_t i = 0; i < queueCount; ++i)
		{
			float prio = i*priorityFactor;
			queuePriosByFamily[qIndex][i] = prio;
		}

		VkDeviceQueueCreateInfo queueCI = {};
		queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCI.queueFamilyIndex = qIndex;
		queueCI.queueCount = queueCount;
		queueCI.pQueuePriorities = queuePriosByFamily[qIndex].data();
		queueCreateInfos.push_back(queueCI);

	}

	VkPhysicalDeviceFeatures features = {};
	vkGetPhysicalDeviceFeatures(devices[physDeviceIndex]->device, &features);

	VkDeviceCreateInfo deviceCI = {};
	deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCI.pQueueCreateInfos = queueCreateInfos.data();
	deviceCI.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();

	deviceCI.pEnabledFeatures = &features;
	VkResult result = CheckAndGetDeviceExtensions(devices[physDeviceIndex]->device, requiredExtensions);
	if (result != VK_SUCCESS)
		throw std::invalid_argument("Device extension check failed, reason:" + VkResultToString(result));
	deviceCI.enabledExtensionCount = (uint32_t)requiredExtensions.size();
	deviceCI.ppEnabledExtensionNames = requiredExtensions.data();
	deviceCI.enabledLayerCount = 0;

	VkManagedDevice * device = nullptr;
	try
	{
		device = new VkManagedDevice(deviceCI,devices[physDeviceIndex]);
	}
	catch(...)
	{
		for (const char* ext : requiredExtensions)
			delete ext;
		throw;
	}
	for (const char* ext : requiredExtensions)
		delete ext;

	return device;

}

void Vulkan::VkManagedInstance::AcquirePhysicalDevices()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance.object(), &count, nullptr);

	if (count == 0)
		throw std::runtime_error("No GPUs with Vulkan support were found on the current system");
	std::vector<VkPhysicalDevice> systemDevices;
	systemDevices.resize(count);
	devices.resize(count);
	vkEnumeratePhysicalDevices(m_instance.object(), &count, systemDevices.data());

	for (uint32_t i = 0; i < count;++i) 
	{
		devices[i] = new VkPhysicalDeviceData();
		devices[i]->device = systemDevices[i];
		vkGetPhysicalDeviceProperties(systemDevices[i],&devices[i]->deviceProperties);

	}
}

VkResult Vulkan::VkManagedInstance::CheckAndGetDeviceExtensions(VkPhysicalDevice physDevice, std::vector<const char*>& requiredExtensions)
{
	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extCount, extensions.data());

	std::vector<std::string> reqExts(requiredExtensions.begin(), requiredExtensions.end());
	std::vector<const char*> deviceExtensions;
	deviceExtensions.reserve(extCount);
	for(uint32_t i = 0; i < extCount;++i)
	{
		char * extName = new char[256]; //memcpy
		memcpy(extName, extensions[i].extensionName, 256 * sizeof(char));

		deviceExtensions.push_back(extName);
		reqExts.erase(std::remove(reqExts.begin(), reqExts.end(), deviceExtensions[i]), reqExts.end());

	}

	if (!reqExts.empty())
		return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;


	requiredExtensions = deviceExtensions;
	return VK_SUCCESS;
}

const std::vector<const char*> Vulkan::VkManagedInstance::GetInstanceExtensions()
{
	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_WIN32_KHR
	extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(__linux__)
	enabledExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif // VK_USE_PLATFORM_WIN32_KHR


#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif // !NDEBUG

	return extensions;
}
