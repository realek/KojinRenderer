/*=========================================================
VulkanObject.h - Container class used to manage Vulkan object
instances. In vulkan all created objects are the responsability
of the programmer. Does not support move semantics, supports only assignment
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
	class VkManagedObject
	{
	public:

		VkManagedObject() {};

		inline void set_object(const T object, std::function<void(T, VkAllocationCallbacks*)> deleteFunction, bool destroy = true)
		{
			if (object != VK_NULL_HANDLE && m_destroy)
			{
				clean();
			}
			this->m_deleter = [=](T object) { deleteFunction(object, nullptr); };
			this->m_destroy = destroy;
			m_object = object;
		}

		inline void set_object(const T object, const VkInstance& owner, std::function<void(VkInstance, T, VkAllocationCallbacks*)> deleteFunction, bool destroy = true)
		{
			if (object != VK_NULL_HANDLE && m_destroy)
			{
				clean();
			}
			this->m_deleter = [&owner, deleteFunction](T object) { deleteFunction(owner, object, nullptr); };
			this->m_destroy = destroy;
			m_object = object;
		}

		inline void set_object(const T object, const VkDevice& owner, std::function<void(VkDevice, T, VkAllocationCallbacks*)> deleteFunction, bool destroy = true)
		{
			if (object != VK_NULL_HANDLE && m_destroy)
			{
				clean();
			}
			this->m_deleter = [&owner, deleteFunction](T object) { deleteFunction(owner, object, nullptr); };
			this->m_destroy = destroy;
			m_object = object;
		}

		//Releases the contained object without deleting it
		inline T release() 
		{
			T obj = m_object;
			m_object = VK_NULL_HANDLE;

			return obj;
		}

		//Clears and destroys(if clear flag is true) the contained object
		inline void clean()
		{
			if (this->m_object != VK_NULL_HANDLE)
			{
				this->m_deleter(m_object);
				this->m_object = VK_NULL_HANDLE;
			}
		}

		~VkManagedObject()
		{
			if(m_destroy)
				clean();
		}

		//Assign the contained object from another VkManagedObject, right hand side object will release
		void operator=(VkManagedObject<T> rhs)
		{
			if (rhs.m_object != this->m_object) {
				
				if (this->clear)
					this->clean();

				this->m_deleter = rhs.m_deleter;
				this->object = rhs.release();
			}
		}
		
		inline const T& object() const
		{
			return m_object;
		}

		template <typename C>
		bool operator ==(C& other)
		{
			T* objPtr = &m_object;
			C* otherPtr = &other;
			return objPtr == reinterpret_cast<T*>(otherPtr);
		}

		template <typename C>
		bool operator !=(C& other)
		{
			return !(this==other);
		}

	private:

		bool m_destroy = true;
		T m_object{ VK_NULL_HANDLE };
		std::function<void(T)> m_deleter { nullptr };
	};
}

