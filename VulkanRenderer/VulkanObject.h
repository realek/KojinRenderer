/*=========================================================
VulkanObject.h - Container class used to manage Vulkan object
instances. In vulkan all created objects are the responsability
of the programmer.
==========================================================*/

#pragma once
#include <functional>
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "VulkanUtils.h"

namespace Vulkan
{
	template <typename T>
	class VulkanObjectContainer
	{
	public:

		VulkanObjectContainer() : VulkanObjectContainer([](T, VkAllocationCallbacks*) {}) {}
		VulkanObjectContainer(std::function<void(T, VkAllocationCallbacks*)> deleteFunction)
		{
			this->deleter = [=](T object) { deleteFunction(object, nullptr); };

		}

		VulkanObjectContainer(const VulkanObjectContainer<VkInstance> &vkInstance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deleteFunction)
		{
			this->deleter = [&vkInstance, deleteFunction](T object) {  deleteFunction(vkInstance, object, nullptr);	};
		}

		VulkanObjectContainer(const VulkanObjectContainer<VkDevice> &vkDevice, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deleteFunction)
		{
			this->deleter = [&vkDevice, deleteFunction](T object) { deleteFunction(vkDevice, object, nullptr); };
		}

		VulkanObjectContainer(VkInstance vkInstance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deleteFunction)
		{
			this->deleter = [vkInstance, deleteFunction](T object) {  deleteFunction(vkInstance, object, nullptr);	};
		}

		VulkanObjectContainer(VkDevice vkDevice, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deleteFunction)
		{
			this->deleter = [vkDevice, deleteFunction](T object) { deleteFunction(vkDevice, object, nullptr); };
		}


		~VulkanObjectContainer()
		{
			Clean();
		}


		//prefix increment operator overloaded for use with vulkan api
		T* operator ++()
		{
			Clean();
			return &object;
		}

		void operator=(T rhs) {
			if (rhs != this->object) {
				this->Clean();
				this->object = rhs;
			}
		}

		operator T() const
		{
			return object;
		}

		template <typename C>
		bool operator ==(C other)
		{
			return this->object == (T)other;
		}

		inline T Get() const
		{
			return object;
		}

	private:

		T object{ VK_NULL_HANDLE };
		std::function<void(T)> deleter;
		void Clean();
	};

	template <typename T>
	inline void VulkanObjectContainer<T>::Clean()
	{
		if (this->object != VK_NULL_HANDLE)
		{
			this->deleter(object);
			this->object = VK_NULL_HANDLE;

		}
	}
}

