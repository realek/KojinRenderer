/*=============================================================================
VulkanSystemStructs.h - Header file containing multiple structure definitions
that wrap certain Vulkan objects such as VkBuffer, providing an easier way to
use them.
=============================================================================*/

#pragma once
#include <memory>
#include <vector>
#include <array>
#include <map>
#include "VulkanObject.h"
#include <glm/gtx/hash.hpp>
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\matrix.hpp>
namespace Vulkan
{


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
				&& hasSparseBindingQueue == rhs.hasSparseBindingQueue;
		}


		bool operator ==(VkPhysicalDeviceRequiredQueues * rhs)
		{
			return hasComputeQueue == rhs->hasComputeQueue && hasGraphicsQueue == rhs->hasGraphicsQueue
				&& hasPresentQueue == rhs->hasPresentQueue && hasTransferQueue == rhs->hasTransferQueue
				&& hasSparseBindingQueue == rhs->hasSparseBindingQueue;
		}

		bool operator ==(const VkPhysicalDeviceRequiredQueues * rhs)
		{
			return hasComputeQueue == rhs->hasComputeQueue && hasGraphicsQueue == rhs->hasGraphicsQueue
				&& hasPresentQueue == rhs->hasPresentQueue && hasTransferQueue == rhs->hasTransferQueue
				&& hasSparseBindingQueue == rhs->hasSparseBindingQueue;
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

	struct VkSwapchainBuffer
	{
		VkSwapchainBuffer()
		{
			image = VK_NULL_HANDLE;
			imageView = VK_NULL_HANDLE;
		}
		VkSwapchainBuffer(VkDevice device)
		{
			image = VK_NULL_HANDLE;
			imageView = VulkanObjectContainer<VkImageView>{ device, vkDestroyImageView };
		}

		VkImage image;
		VulkanObjectContainer<VkImageView> imageView;

	};

	struct VkManagedImage
	{
		VkManagedImage()
		{
			imageMemory = VK_NULL_HANDLE;
			imageView = VK_NULL_HANDLE;
			image = VK_NULL_HANDLE;
		}
		VkManagedImage(VkDevice device)
		{
			image = VulkanObjectContainer<VkImage>{ device,vkDestroyImage };
			imageView = VulkanObjectContainer<VkImageView>{ device,vkDestroyImageView };
			imageMemory = VulkanObjectContainer<VkDeviceMemory>{ device,vkFreeMemory };
		};

		VulkanObjectContainer<VkImage> image;
		VulkanObjectContainer<VkImageView> imageView;
		VulkanObjectContainer<VkDeviceMemory> imageMemory;
	};

	struct VkManagedBuffer
	{
		VulkanObjectContainer<VkBuffer> buffer;
		VulkanObjectContainer<VkDeviceMemory> memory;
		VkDeviceSize bufferSize;
		VkDevice device;

		void* mappedMemory = nullptr;
		VkManagedBuffer()
		{
			device = VK_NULL_HANDLE;
			buffer = VK_NULL_HANDLE;
			memory = VK_NULL_HANDLE;
			bufferSize = 0;
		}

		VkManagedBuffer(VkDevice device, VkDeviceSize bufferSize)
		{
			this->device = device;
			this->bufferSize = bufferSize;
			buffer = VulkanObjectContainer<VkBuffer>{ device,vkDestroyBuffer };
			memory = VulkanObjectContainer<VkDeviceMemory>{ device, vkFreeMemory };
		}

		void* Map(VkDeviceSize offset = 0, VkMemoryMapFlags flags = 0)
		{
			vkMapMemory(device, memory, offset, bufferSize, flags, &mappedMemory);

			return mappedMemory;
		};
		void UnMap()
		{
			vkUnmapMemory(device, memory);
			mappedMemory = nullptr;
		};
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
		bool operator==(const VkVertex& other) const {
			return pos == other.pos && normal == other.normal && color == other.color && texCoord == other.texCoord;
		}
	};

	struct VkCamera
	{
		VkViewport* viewport;
		VkRect2D* scissor;

	};

	struct VkConsumedMesh
	{

		VkConsumedMesh() {}

		VkConsumedMesh(VkDevice device, VkDeviceSize vertexSize, VkDeviceSize indiceSize,uint32_t indiceCount)
		{
			vertexBuffer = VkManagedBuffer{ device, vertexSize };
			indiceBuffer = VkManagedBuffer{ device, indiceSize };
			this->totalIndiceCount = indiceCount;
		}

		VkManagedBuffer vertexBuffer;
		VkManagedBuffer indiceBuffer;
		std::vector<uint32_t> indiceOffsets;
		std::vector<uint32_t> indiceCounts;
		uint32_t totalIndiceCount;
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 modelView;
		glm::mat4 modelViewProjection;
		glm::mat4 normal;

		inline void ComputeMatrices()
		{
			modelView = view*model;
			normal = glm::transpose(glm::inverse(modelView));
			modelViewProjection = proj*modelView;
		}
	};

	struct LightingUniformBuffer
	{
		glm::vec4 ambientLightColor;
		glm::vec4 perFragmentLightPos[4];
		glm::vec4 perFragmentLightColor[4];
		glm::vec4 perFragmentLightIntensity[4];
		float specularity;
	};

	//dynamic uniform buffers - unused
	struct DynamicUniformBuffer
	{
		glm::mat4 * model;
	};

	struct DynamicCameraUniformBufferObject
	{
		glm::mat4* view;
		glm::mat4* projection;
		glm::mat4* viewProjection;
	};

	struct DynamicLightingUniformBuffer
	{
		glm::vec4* ambientLightColor;
		glm::vec4* perFragmentLightPos;
		glm::vec4* perFragmentLightColor;
		glm::vec4* perFragmentLightIntensity;
	};
}

namespace std {

	template<> struct hash<Vulkan::VkVertex> {
		size_t operator()(Vulkan::VkVertex const& vertex) const {
			return (((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(((hash<glm::vec3>()(vertex.color) << 1) ^ 
					hash<glm::vec2>()(vertex.texCoord)) >> 1)) << 1;
		}
	};
}

