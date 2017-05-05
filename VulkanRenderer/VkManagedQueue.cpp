#include "VkManagedQueue.h"

Vulkan::VkManagedQueue::VkManagedQueue(VkQueue queueHandle, uint32_t familyIdx, uint32_t queueIdx, VkBool32 canPresent) : queue(queueHandle), familyIndex(familyIdx), queueIndex(queueIdx), presentSupport(canPresent)
{

}

Vulkan::VkManagedQueue::~VkManagedQueue()
{
}

