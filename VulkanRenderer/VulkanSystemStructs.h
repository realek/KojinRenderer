#pragma once
#include <vector>
#include <array>
#include "VulkanObject.h"
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\matrix.hpp>
namespace Vk
{

	static std::string VkResultToString(VkResult errorCode)
	{
		switch (errorCode)
		{
		case VK_NOT_READY:
			return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILEDs";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPABILE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FALIED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		default:
			return "VK_UNKNOWN_ERROR_CODE";
		}
	}

	struct VkPhysicalDeviceRequiredQueues
	{
		bool hasGraphicsQueue;
		bool hasPresentQueue;
		bool hasComputeQueue;
		bool hasTransferQueue;
		bool hasSparseBindingQueue;

		bool operator ==(VkPhysicalDeviceRequiredQueues rhs)
		{
			return hasComputeQueue == rhs.hasComputeQueue && hasGraphicsQueue == rhs.hasGraphicsQueue
				&& hasPresentQueue == rhs.hasPresentQueue && hasTransferQueue == rhs.hasTransferQueue
				&& hasTransferQueue == rhs.hasTransferQueue && hasSparseBindingQueue == rhs.hasSparseBindingQueue;
		}


		bool operator ==(VkPhysicalDeviceRequiredQueues * rhs)
		{
			return hasComputeQueue == rhs->hasComputeQueue && hasGraphicsQueue == rhs->hasGraphicsQueue
				&& hasPresentQueue == rhs->hasPresentQueue && hasTransferQueue == rhs->hasTransferQueue
				&& hasTransferQueue == rhs->hasTransferQueue && hasSparseBindingQueue == rhs->hasSparseBindingQueue;
		}

		bool operator ==(const VkPhysicalDeviceRequiredQueues * rhs)
		{
			return hasComputeQueue == rhs->hasComputeQueue && hasGraphicsQueue == rhs->hasGraphicsQueue
				&& hasPresentQueue == rhs->hasPresentQueue && hasTransferQueue == rhs->hasTransferQueue
				&& hasTransferQueue == rhs->hasTransferQueue && hasSparseBindingQueue == rhs->hasSparseBindingQueue;
		}

	};

	struct VkQueueFamilyIDs
	{
		uint32_t graphicsFamily = -1;
		uint32_t presentFamily = -1;
		uint32_t computeFamily = -1;
		uint32_t transferFamily = -1;
		uint32_t sparseBindingFamily = -1;

		bool Validate(const VkPhysicalDeviceRequiredQueues * reqs)
		{
			VkPhysicalDeviceRequiredQueues checks = { false,false,false,false,false };
			if (reqs->hasGraphicsQueue && graphicsFamily >= 0)
				checks.hasGraphicsQueue = true;

			if (reqs->hasComputeQueue && computeFamily >= 0)
				checks.hasComputeQueue = true;

			if (reqs->hasPresentQueue && presentFamily >= 0)
				checks.hasPresentQueue = true;

			if (reqs->hasSparseBindingQueue && sparseBindingFamily >= 0)
				checks.hasSparseBindingQueue = true;

			if (reqs->hasTransferQueue && transferFamily >= 0)
				checks.hasTransferQueue = true;

			return checks == reqs;
		}
	};

	struct VkSwapChainSupportData {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		bool Validate()
		{
			return formats.size() > 0 && presentModes.size() > 0;
		}
	};

	struct VkQueueContainer
	{
		VkQueue graphicsQueue{ VK_NULL_HANDLE };
		VkQueue presentQueue{ VK_NULL_HANDLE };
		VkQueue transferQueue{ VK_NULL_HANDLE };
		VkQueue computeQueue{ VK_NULL_HANDLE };
		VkQueue sparseBindingQueue{ VK_NULL_HANDLE };

	};

	struct VkVertex
	{
		
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 texCoord;


		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VkVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VkVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VkVertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(VkVertex, color);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(VkVertex, texCoord);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


}

