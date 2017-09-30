#include "VulkanSystemStructs.h"
#include "VulkanHash.h"

const float Vulkan::VkViewportDefaults::k_CameraZFar = 100.0f;
const float Vulkan::VkViewportDefaults::k_CameraZNear = 0.1f;
const float Vulkan::VkViewportDefaults::k_CameraFov = 60.0f;
const float Vulkan::VkViewportDefaults::k_CameraMaxFov = 175.0f;
const float Vulkan::VkViewportDefaults::k_CameraOrthoSize = 8.0f;

const float Vulkan::VkShadowmapDefaults::k_LightZFar = 100.0f;
const float Vulkan::VkShadowmapDefaults::k_lightZNear = 0.5f;
const float Vulkan::VkShadowmapDefaults::k_lightFOVOffset = 30.0f;
const glm::mat4 Vulkan::VkShadowmapDefaults::k_shadowBiasMatrix ={
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0};
const float Vulkan::VkShadowmapDefaults::k_depthBias = 0.05f;
const float Vulkan::VkShadowmapDefaults::k_depthBiasSlope = 0.25f;
const uint32_t Vulkan::VkShadowmapDefaults::k_resolution = 1024;
const VkFormat Vulkan::VkShadowmapDefaults::k_attachmentRGBFormat = VK_FORMAT_R32_SFLOAT;
const VkFormat Vulkan::VkShadowmapDefaults::k_attachmentDepthFormat = VK_FORMAT_D32_SFLOAT;
const std::vector<VkClearValue> Vulkan::VkShadowmapDefaults::k_clearValues = 
{
	{1, 0},
	{1, 0}
};