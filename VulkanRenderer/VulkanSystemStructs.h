/*=============================================================================
VulkanSystemStructs.h - Header file containing multiple structure definitions
that wrap certain Vulkan objects such as VkBuffer, providing an easier way to
use them.
=============================================================================*/

#pragma once
#include <stdio.h>
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
#define MAX_LIGHTS_PER_FRAGMENT 4
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

		bool Validate(const VkPhysicalDeviceRequiredQueues * reqs);
	};

	struct VkSwapChainSupportData {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		bool Validate();
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
		glm::vec3* position;
	};

	struct VkLightProps
	{
		int32_t lightType;
		float intensity;
		float falloff;
		float angle;

	};

	struct VkLight
	{

		glm::vec4 color;
		glm::vec4 position;
		glm::vec4 direction;
		VkLightProps lightProps; //type,angle,range, 1.0f pad
		//float range;

	};

	struct VkDepthUniformBuffer 
	{
		glm::mat4 depthMVP;
	};

	struct VkViewportDefaultSettings
	{
		static const float k_zFar;
		static const float k_zNear;
		static const float k_Fov;
		static const float k_OrthoSize;
		VkViewportDefaultSettings() = delete;
		VkViewportDefaultSettings(const VkViewportDefaultSettings& other) = delete;
	};

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 modelView;
		glm::mat4 modelViewProjection;
		glm::mat4 normal;
		glm::mat4 depthBiasMVP;

		inline void ComputeMatrices(glm::mat4& view, glm::mat4& proj)
		{
			modelView = view*model;
			normal = glm::transpose(glm::inverse(modelView));
			modelViewProjection = proj*modelView;
		}
	};

	struct LightingUniformBuffer
	{
		//glm::vec4 cameraPos;
		VkLight lights[MAX_LIGHTS_PER_FRAGMENT];
		glm::vec4 ambientLightColor;
		glm::vec4 materialDiffuse;
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



