#pragma once
#include <vulkan\vulkan.h>
#include <vector>
namespace Vulkan
{
	struct VkDepthBias
	{
		float constDepth;
		float depthClamp;
		float depthSlope;
	};

	struct VkDepthBounds
	{
		float minDepth;
		float maxDepth;
	};

	struct VkStencilCompareMask
	{
		uint32_t compareMask = 0;
		VkStencilFaceFlags faceMask = 0;
	};

	struct VkStencilWriteMask
	{
		uint32_t writeMask = 0;
		VkStencilFaceFlags faceMask = 0;
	};

	struct VkStencilReference
	{
		VkStencilFaceFlags faceMask = 0;
		uint32_t stencilReference = 0;
	};

	struct VkDynamicStatesBlock
	{
		VkDepthBias depthBias;
		VkBool32 hasDepthBias = VK_FALSE;
		std::vector<VkViewport> viewports;
		uint32_t viewportOffset = 0;
		VkBool32 hasViewport = VK_FALSE;
		std::vector<VkRect2D> scissors;
		uint32_t scissorOffset = 0;
		VkBool32 hasScissor = VK_FALSE;
		float blendConstants[4];
		VkBool32 hasBlendConstants = VK_FALSE;
		float lineWidth = 0;
		VkBool32 hasLineWidth = VK_FALSE;
		VkDepthBounds depthBounds;
		VkBool32 hasDepthBounds = VK_FALSE;
		VkStencilCompareMask stencilCompareMask;
		VkBool32 hasStencilCompareMask = VK_FALSE;
		VkStencilWriteMask stencilWriteMask;
		VkBool32 hasStencilWriteMask = VK_FALSE;
		VkStencilReference stencilReference;
		VkBool32 hasStencilReference = VK_FALSE;
		std::vector<VkViewportWScalingNV> viewportScalingsNV;
		uint32_t viewportScalingsOffset = 0;
		VkBool32 hasViewportScalingsNV = VK_FALSE;
		std::vector<VkRect2D> discardRectangleEXTs;
		uint32_t discardRectangleEXTOffset = 0;
		VkBool32 hasDiscardRectangleEXT = VK_FALSE; 
	};
	struct VkSurfaceData
	{
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkExtent2D windowExtent = {};
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VkPhysicalDeviceData
	{
		VkPhysicalDevice device = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties deviceProperties = {};
		std::vector<VkQueueFamilyProperties> queueFamilies;
		std::vector<uint32_t> presentFamilies;
		VkSurfaceData deviceSurfaceData = {};
	};

	struct VkIndexedDraw
	{
		uint32_t vertexOffset = 0;
		uint32_t indexCount = 0;
		uint32_t indexStart = 0;
	};

	struct VkPushConstant
	{
		void * data = nullptr;
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		uint32_t size = 0;
		uint32_t offset = 0;
	};
}