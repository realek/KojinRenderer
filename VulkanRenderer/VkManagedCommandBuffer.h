#pragma once
#include "VulkanObject.h"
#include <vector>

namespace Vulkan
{
	class VkManagedQueue;
	class VkManagedCommandBuffer
	{
	public:

		///beging a specific command buffer with the provided flag
		void Begin(VkCommandBufferUsageFlags usage, size_t index);
		///begin all command buffers with the provided flags
		void Begin(VkCommandBufferUsageFlags usage);
		///End a specific command buffer
		void End(size_t index);
		///End all contained command buffers
		void End();
		///Called on the object in order to release its command buffers, command buffers always get released when their pool is released
		void Free();
		///Submit all contained command buffers
		VkResult Submit(VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait);
		///Submit specific command buffer
		VkResult Submit(VkQueue queue, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signal, std::vector<VkSemaphore> wait, size_t index);
		///Get current number of command buffers in the container
		size_t Size();
		///By default returns first command buffer, index can be specified
		VkCommandBuffer Buffer(size_t index = 0);
		///The level of the contained command buffers

	private:
		VkManagedCommandBuffer(VkDevice device,VkCommandBufferAllocateInfo allocInfo);
		friend class VkManagedCommandPool;

	private:

		std::vector<VkCommandBuffer> m_buffers;
		VkDevice m_device = VK_NULL_HANDLE;
		VkCommandPool m_pool = VK_NULL_HANDLE;
		VkCommandBufferLevel bufferLevel = VK_COMMAND_BUFFER_LEVEL_MAX_ENUM;
	};
}