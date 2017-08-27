#pragma once
#include "VulkanObject.h"
#include <memory>
#include <vector>
#include "VkManagedFrameBuffer.h"
#include "VkManagedStructures.h"

namespace Vulkan
{

	enum PushConstantMode
	{
		PerPass = 0,
		PerDraw = 1
	};


	class VulkanImageUnit;
	class VulkanCommandUnit;

	
	struct VkManagedImage;
	class VkManagedDevice;
	class VkManagedCommandBuffer;
	class VkManagedDescriptorSet;
	class VkManagedQueue;
	class VkManagedPipeline;
	class VkManagedBuffer;

	class VkManagedRenderPass
	{
	public:

		inline VkManagedRenderPass() {};
		inline ~VkManagedRenderPass() 
		{
			if (m_fbs.size() > 0)
			{
				for (VkManagedFrameBuffer * fb : m_fbs)
					delete fb;
			}
		};
		void Build(const VkDevice & device, VkExtent2D extent, VkFormat depthFormat);
		void Build(const VkDevice & device, VkExtent2D extent, VkFormat colorFormat, VkFormat depthFormat);
		void SetPipeline(VkManagedPipeline * pipeline, VkDynamicStatesBlock dynamicStates, VkPipelineBindPoint bindPoint);
		void UpdateDynamicStates(VkDynamicStatesBlock dynamicStates);
		void PreRecordData(VkCommandBuffer commandBuffer, uint32_t frameBufferIndex);
		void Record(const std::vector<VkClearValue>& values, std::vector<VkManagedDescriptorSet*> descriptors, std::vector<VkPushConstant>& pushConstants, VkManagedBuffer * indexBuffer, VkManagedBuffer * vertexBuffer, std::vector<VkIndexedDraw>& draws);
		void SetFrameBufferCount(const VkDevice & device, const VkPhysicalDevice pDevice, uint32_t count, uint32_t layerCount, VkManagedFrameBufferUsage mask);
		VkExtent2D GetExtent();
		VkExtent3D GetExtent3D();

		inline const size_t FramebufferCount()
		{
			return m_fbs.size();
		}

		inline const VkFramebuffer& GetFrameBuffer(uint32_t index = 0)
		{
			return m_fbs[index]->frameBuffer();
		}

		inline std::vector<VkFramebuffer> Vulkan::VkManagedRenderPass::GetFrameBuffers()
		{
			std::vector<VkFramebuffer> fbs;
			fbs.resize(m_fbs.size());

			for (size_t i = 0; i < fbs.size(); i++)
				fbs[i] = m_fbs[i]->frameBuffer();

			return fbs;
		}

		inline VkManagedImage GetAttachment(size_t index, VkImageUsageFlagBits attachmentType)
		{
			switch (attachmentType)
			{
			case VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
			{
				VkManagedImage img = m_fbs[index]->colorAttachment();
				img.layout = m_colorFinalLayout;
				img.format = m_colorformat;
				img.imageExtent.width = m_extent.width;
				img.imageExtent.height = m_extent.height;
				img.imageExtent.depth = 1U;
				return img;
			}

			case VkImageUsageFlagBits::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
			{

				VkManagedImage img = m_fbs[index]->depthAttachment();
				img.layout = m_depthFinalLayout;
				img.format = m_depthFormat;
				img.imageExtent.width = m_extent.width;
				img.imageExtent.height = m_extent.height;
				img.imageExtent.depth = 1U;
				return img;
			}

			default:
				assert(false && "Incorrect attachment type provided.");
				return {};
			}
		}

		inline const VkRenderPass& renderPass() const
		{
			return m_pass.object();
		}

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
		VkManagedObject<VkRenderPass> m_pass;
		std::vector<VkManagedFrameBuffer*> m_fbs;

	};
}