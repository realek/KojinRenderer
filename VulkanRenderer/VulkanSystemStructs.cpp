#include "VulkanSystemStructs.h"
#include "VulkanHash.h"

bool Vulkan::VkQueueFamilyIDs::Validate(const Vulkan::VkPhysicalDeviceRequiredQueues * reqs) {
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

bool Vulkan::VkSwapChainSupportData::Validate()
{
	return formats.size() > 0 && presentModes.size() > 0;
}

const float Vulkan::VkViewportDefaults::k_CameraZFar = 100.0f;
const float Vulkan::VkViewportDefaults::k_CameraZNear = 0.1f;
const float Vulkan::VkViewportDefaults::k_CameraFov = 60.0f;
const float Vulkan::VkViewportDefaults::k_CameraMaxFov = 175.0f;
const float Vulkan::VkViewportDefaults::k_CameraOrthoSize = 8.0f;

const float Vulkan::VkShadowmapDefaults::k_LightZFar = 100.0f;
const float Vulkan::VkShadowmapDefaults::k_lightZNear = 1.0f;
const float Vulkan::VkShadowmapDefaults::k_lightFOVOffset = 30.0f;
const glm::mat4 Vulkan::VkShadowmapDefaults::k_shadowBiasMatrix ={
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0};
const float Vulkan::VkShadowmapDefaults::k_depthBias = 1.25f;
const float Vulkan::VkShadowmapDefaults::k_depthBiasSlope = 1.75f;
const uint32_t Vulkan::VkShadowmapDefaults::k_defaultShadowmapResolution = 512;
const VkFormat Vulkan::VkShadowmapDefaults::k_shadowmapAttachmentFormat = VK_FORMAT_D32_SFLOAT;