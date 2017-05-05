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

	class VkManagedRefCounter;
	template <typename T>
	class VulkanObjectContainer
	{
	public:

		VulkanObjectContainer() : VulkanObjectContainer([](T, VkAllocationCallbacks*) {}) {}
		VulkanObjectContainer(std::function<void(T, VkAllocationCallbacks*)> deleteFunction, bool clear = true)
		{
			this->deleter = [=](T object) { deleteFunction(object, nullptr); };
			this->clear = clear;
		}

		VulkanObjectContainer(const VulkanObjectContainer<VkInstance>& vkInstance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deleteFunction, bool clear = true)
		{
			this->deleter = [&vkInstance, deleteFunction](T object) {  deleteFunction(vkInstance, object, nullptr);	};
			this->clear = clear;
		
		}

		VulkanObjectContainer(const VulkanObjectContainer<VkDevice>& vkDevice, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deleteFunction, bool clear = true)
		{
			this->deleter = [&vkDevice, deleteFunction](T object) { deleteFunction(vkDevice, object, nullptr); };
			this->clear = clear;
		
		}

		VulkanObjectContainer(const VkInstance& vkInstance, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deleteFunction, bool clear = true)
		{
			this->deleter = [vkInstance, deleteFunction](T object) {  deleteFunction(vkInstance, object, nullptr);	};
			this->clear = clear;
		}

		VulkanObjectContainer(const VkDevice& vkDevice, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deleteFunction, bool clear = true)
		{
			this->deleter = [vkDevice, deleteFunction](T object) { deleteFunction(vkDevice, object, nullptr); };
			this->clear = clear;
		}


		~VulkanObjectContainer()
		{
			if(clear)
				Clean();
		}

		//prefix increment operator overloaded for use with vulkan api, in order to write on the object
		T* operator ++()
		{
			if (clear)
				Clean();
			else
				object = VK_NULL_HANDLE;
			return &object;
		}
		//prefix decrement operator overloaded to get internal object reference
		T* operator --()
		{
			return &object;
		}

		void operator=(T rhs) {
			if (rhs != this->object) {
				if(this->clear)
					this->Clean();
				this->object = rhs;
			}
		}

		operator T() const
		{
			return object;
		}

		template <typename C>
		bool operator ==(C& other)
		{
			T* objPtr = &object;
			C* otherPtr = &other;
			return objPtr == reinterpret_cast<T*>(otherPtr);
		}
		bool clear = false;

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

