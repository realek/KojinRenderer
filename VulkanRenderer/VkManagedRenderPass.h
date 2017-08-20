#pragma once
#include "VulkanObject.h"
#include <memory>
#include <map>
#include "VkManagedStructures.h"

namespace Vulkan
{

	enum PushConstantMode
	{
		PerPass = 0,
		PerDraw = 1
	};

	class VkManagedImage;
	class VkManagedFrameBuffer;
	class VulkanImageUnit;
	class VulkanCommandUnit;

	
	class VkManagedImage;
	class VkManagedDevice;
	class VkManagedCommandBuffer;
	class VkManagedDescriptorSet;
	class VkManagedQueue;
	class VkManagedPipeline;
	class VkManagedBuffer;

	class VkManagedRenderPass
	{
	public:

		VkManagedRenderPass(VkManagedDevice * device);
		void Build(VkExtent2D extent, VkFormat depthFormat);
		void Build(VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat);
		void SetPipeline(VkManagedPipeline * pipeline, VkDynamicStatesBlock dynamicStates, VkPipelineBindPoint bindPoint);
		void UpdateDynamicStates(VkDynamicStatesBlock dynamicStates);
		void PreRecordData(VkCommandBuffer commandBuffer, uint32_t frameBufferIndex);
		void Record(const std::vector<VkClearValue>& values, std::vector<VkManagedDescriptorSet*> descriptors, std::vector<VkPushConstant>& pushConstants, VkManagedBuffer * indexBuffer, VkManagedBuffer * vertexBuffer, std::vector<VkIndexedDraw>& draws);
		VkManagedRenderPass();
		~VkManagedRenderPass();
		void SetFrameBufferCount(uint32_t count, bool setFinalLayout, bool sampleColor, bool copyColor, bool sampleDepth, bool copyDepth);
		operator VkRenderPass() const;
		VkExtent2D GetExtent();
		VkExtent3D GetExtent3D();
		VkDevice GetDevice();
		size_t FramebufferCount();
		VkFramebuffer GetFrameBuffer(uint32_t index = 0);
		std::vector<VkFramebuffer> GetFrameBuffers();
		Vulkan::VkManagedImage * GetAttachment(size_t index, VkImageUsageFlagBits attachmentType);

	private:

		enum RenderPassType
		{
			Uninitialized = 0,
			Secondary_OnScreen_Forward = 1,
			Secondary_Offscreen_Forward_Projected_Shadows = 2,
			Secondary_Offscreen_Forward_OmniDirectional_Shadows = 3,
			Secondary_Offscreen_Deffered_Lights = 4,
			Secondary_Offscreen_Deffered_Normal = 5,
			Secondary_Offscreen_Deffered_Shadows = 6,
			RenderPassCount = 7
		};
	
	private:

		VkImageLayout m_colorFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout m_depthFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkDynamicStatesBlock m_currentPipelineStateBlock;
		VkManagedPipeline * m_currentPipeline = nullptr;
		VkPipelineBindPoint m_currentPipelineBindpoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_MAX_ENUM;
		VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;
		uint32_t m_currentFBindex = 0;
		RenderPassType m_type;
		VkFormat m_colorformat;
		VkFormat m_depthFormat;
		VkExtent2D m_extent;
		VkManagedDevice * m_mdevice = nullptr;
		VkManagedObject<VkDevice> m_device{ vkDestroyDevice, false };
		VkManagedObject<VkRenderPass> m_pass{ m_device, vkDestroyRenderPass };
		std::vector<VkManagedFrameBuffer*> m_fbs;
		size_t m_fbSize = 0;

	};
}