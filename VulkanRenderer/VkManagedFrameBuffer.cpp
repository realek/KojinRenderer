#include "VkManagedFrameBuffer.h"
#include "VkManagedImage.h"
#include "VulkanImageUnit.h"

Vulkan::VkManagedFrameBuffer::VkManagedFrameBuffer(std::weak_ptr<VulkanImageUnit> imageUnit)
{
	m_imageUnit = imageUnit;
}

Vulkan::VkManagedFrameBuffer::~VkManagedFrameBuffer()
{
	if (m_colorAttachment != nullptr)
		delete m_colorAttachment;
	if (m_depthAttachment != nullptr)
		delete m_depthAttachment;
}

void Vulkan::VkManagedFrameBuffer::SetupAttachment(VkManagedFrameBufferAttachment type, VkExtent2D extent, VkFormat format, bool canSample, bool stencil, bool canCopy)
{
	int usage = 0;
	if (canSample)
		usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (canCopy)
		usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if(type==VkManagedFrameBufferAttachment::ColorAttachment)
	{
		if (m_colorAttachment != nullptr)
			throw std::runtime_error("Color attachment already setup for this buffer, call Clear on the framebuffer before setting up the attachment again");

		std::shared_ptr<VulkanImageUnit> imageUnit = m_imageUnit.lock();
		if (imageUnit == nullptr)
			throw std::runtime_error("Unable to lock weak ptr to Vulkan Image Unit object.");
		usage = usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageUnit->CreateVulkanManagedImageNoData(
			extent.width, extent.height, 1, format, usage, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			m_colorAttachment);


	}
	else
	{
		if (m_depthAttachment != nullptr)
			throw std::runtime_error("Depth attachment already setup for this buffer, call Clear on the framebuffer before setting up the attachment again");
		std::shared_ptr<VulkanImageUnit> imageUnit = m_imageUnit.lock();
		if (imageUnit == nullptr)
			throw std::runtime_error("Unable to lock weak ptr to Vulkan Image Unit object.");

		usage = usage | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		int aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (stencil && format == VK_FORMAT_D32_SFLOAT_S8_UINT)
			aspect = aspect | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageUnit->CreateVulkanManagedImageNoData(
			extent.width, extent.height, 1, format, usage, VK_IMAGE_TILING_OPTIMAL,
			aspect,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			m_depthAttachment);

	}
}

//build frame buffer based on the attachments that were setup
void Vulkan::VkManagedFrameBuffer::Build(VkExtent2D extent, VkDevice device, VkRenderPass pass)
{
	if (m_framebuffer != VK_NULL_HANDLE)
		throw std::runtime_error("Framebuffer already built, Call clear and setup the attachments before calling Build.");

	std::vector<VkImageView> attachments;
	if (m_colorAttachment!=nullptr)
		attachments.push_back(m_colorAttachment->imageView);

	if(m_depthAttachment!=nullptr)
		attachments.push_back(m_depthAttachment->imageView);

	uint32_t size = static_cast<uint32_t>(attachments.size());
	if (size == 0)
		throw std::runtime_error("No attachments were setup prior to calling Build, setup attachments first.");

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = pass;
	framebufferInfo.attachmentCount = size;
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;
	m_framebuffer = { device, vkDestroyFramebuffer };
	VkResult result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, ++m_framebuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Unable to create frame buffer, reason: "+Vulkan::VkResultToString(result));
	}
}

//Clear frame buffer internal data
void Vulkan::VkManagedFrameBuffer::Clear()
{
	if (m_framebuffer != VK_NULL_HANDLE)
		++m_framebuffer;
	if (m_colorAttachment != nullptr)
		delete m_colorAttachment;
	if (m_depthAttachment != nullptr)
		delete m_depthAttachment;
}

VkFramebuffer Vulkan::VkManagedFrameBuffer::FrameBuffer()
{
	return m_framebuffer;
}

Vulkan::VkManagedImage * Vulkan::VkManagedFrameBuffer::ColorAttachment() const
{
	return this->m_colorAttachment;
}

Vulkan::VkManagedImage * Vulkan::VkManagedFrameBuffer::DepthAttachment() const
{
	return this->m_depthAttachment;
}
