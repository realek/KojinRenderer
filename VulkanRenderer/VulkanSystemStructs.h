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
#include "VkManagedImage.h"
#include <glm\vec2.hpp>
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\matrix.hpp>
#define MAX_LIGHTS_PER_FRAGMENT 6
namespace Vulkan
{
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
	
	struct VkViewportDefaults
	{
		static const float k_CameraZFar;
		static const float k_CameraZNear;
		static const float k_CameraFov;
		static const float k_CameraMaxFov;
		static const float k_CameraOrthoSize;
		VkViewportDefaults() = delete;
		VkViewportDefaults(const VkViewportDefaults& other) = delete;
	};

	struct VkShadowmapDefaults
	{
		static const float k_LightZFar;
		static const float k_lightZNear;
		static const float k_lightFOVOffset;
		static const glm::mat4 k_shadowBiasMatrix;
		static const float k_depthBias;
		static const float k_depthBiasSlope;
		static const uint32_t k_resolution;
		static const VkFormat k_attachmentRGBFormat;
		static const VkFormat k_attachmentDepthFormat;
		static const std::vector<VkClearValue> k_clearValues;

		VkShadowmapDefaults() = delete;
		VkShadowmapDefaults(const VkShadowmapDefaults& other) = delete;
	};

	struct materialData
	{
		materialData() : materialDiffuse(0),specularity(0) {}
		glm::vec4 materialDiffuse;
		float specularity;

	};

	struct vec4x4_container
	{
		glm::vec4 va;
		glm::vec4 vb;
		glm::vec4 vc;
		glm::vec4 vd;
	};

	struct mat4_container
	{
		glm::mat4 matrix;
	};

	struct mat4_vec4_float_container
	{
		glm::mat4 matrix;
		glm::vec4 vector;
		float floatValue;
	};

	struct mat4x6_container
	{
		glm::mat4 matrices[6];
	};

	struct vec4x4x6_vec4_container
	{
		vec4x4_container vec4x4x6[6];
		glm::vec4 va;
	};
}



