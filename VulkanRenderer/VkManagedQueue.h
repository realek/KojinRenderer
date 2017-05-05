#pragma once
#include <vulkan\vulkan.h>
namespace Vulkan
{

	class VkManagedQueue
	{
	public:

		const VkQueue queue;
		const uint32_t familyIndex;
		const uint32_t queueIndex;
		const VkBool32 presentSupport;
	private:
		VkManagedQueue(VkQueue queue, uint32_t familyIndex, uint32_t queueIndex, VkBool32 presentSupport);
		~VkManagedQueue();
		friend class VkManagedDevice;

	private:
		bool m_used = false;

	};
}