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
#include "VkManagedBuffer.h"
#include "VkSwapChainBuffer.h"
#include "VkManagedImage.h"
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
		glm::mat4* view;
		glm::mat4* proj;

	};

	struct VkStagingMesh
	{
		std::vector<int> ids;

		std::vector<uint32_t> indiceBases;
		std::vector<uint32_t> indiceCounts;
		std::vector<VkVertex> vertex;
		std::vector<uint32_t> indices;
	
		uint32_t totalIndices;

		std::vector<glm::mat4> modelMatrices;
		std::vector<glm::vec4> diffuseColors;
		std::vector<float> specularities;
		std::vector<VkImageView> diffuseTextures;

		VkStagingMesh();
		VkStagingMesh(const VkStagingMesh& other) = delete;

		void UpdateUniforms(VkStagingMesh& updated);
		void ClearTemporary();
		void ClearAll();
	};

	struct VkConsumedMaterial
	{
		//std::vector<glm::vec4> diffuseColor;
		std::vector<VkImageView> diffuseTexture;
		std::vector<VkImageView> normalMap;
		std::vector<VkImageView> specularMap;
		std::vector<float> specularity;
	};

	struct VkConsumedMesh
	{

		VkConsumedMesh() { this->totalIndiceCount = 0; }

		VkConsumedMesh(VkDevice device, VkDeviceSize vertexSize, VkDeviceSize indiceSize,uint32_t indiceCount)
		{
			vertexBuffer = VkManagedBuffer{ device, vertexSize };
			indiceBuffer = VkManagedBuffer{ device, indiceSize };
			material = {};
			this->totalIndiceCount = indiceCount;
		}


		bool loaded = false;
		VkManagedBuffer vertexBuffer;
		VkManagedBuffer indiceBuffer;
		std::vector<uint32_t> indiceBases;
		std::vector<uint32_t> indiceCounts;
		uint32_t totalIndiceCount;
		uint32_t composingObjectCount;
		VkConsumedMaterial material;
		std::vector<glm::mat4> modelMatrices;
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



