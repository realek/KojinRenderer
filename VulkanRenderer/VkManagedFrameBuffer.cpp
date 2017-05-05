#include "VkManagedFrameBuffer.h"
#include "VkManagedImage.h"
#include "VulkanImageUnit.h"
#include "VkManagedDevice.h"


Vulkan::VkManagedFrameBuffer::VkManagedFrameBuffer(VkManagedDevice * device, VkRenderPass pass)
{
	assert(device != nullptr);
	assert(pass != VK_NULL_HANDLE);
	m_mdevice = device;
	m_pass = pass;
	m_device = *m_mdevice;
}

void Vulkan::VkManagedFrameBuffer::Build(VkExtent2D extent, bool sample, bool copy, VkFormat format, VkManagedFrameBufferAttachment singleAttachment)
{
	assert(format != VK_FORMAT_UNDEFINED);
	uint32_t usage = 0;
	std::vector<VkImageView> attachments;
	if (singleAttachment == VkManagedFrameBufferAttachment::ColorAttachment)
	{
		if (!m_mdevice->CheckFormatFeature(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, format, VK_IMAGE_TILING_OPTIMAL))
		{
			throw std::invalid_argument("Provided color format does not support optimal tiling");
		}

		usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (sample)
			usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (copy)
			usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (m_colorAttachment == nullptr)
			m_colorAttachment = new VkManagedImage(m_mdevice);

		m_colorAttachment->Build(extent, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, format, VK_IMAGE_ASPECT_COLOR_BIT, usage);

		attachments.push_back(*m_colorAttachment);
	}
	else if (singleAttachment == VkManagedFrameBufferAttachment::DepthAttachment)
	{
		if (!m_mdevice->CheckFormatFeature(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, format, VK_IMAGE_TILING_OPTIMAL))
		{
			throw std::invalid_argument("Provided depth format does not support optimal tiling");
		}

		usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (sample)
			usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (copy)
			usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		if (m_depthAttachment == nullptr)
			m_depthAttachment = new VkManagedImage(m_mdevice);

		uint32_t depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT)
			depthAspect = depthAspect | VK_IMAGE_ASPECT_STENCIL_BIT;

		m_depthAttachment->Build(extent, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, format, depthAspect, usage);
		attachments.push_back(*m_depthAttachment);
	}

	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_pass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;
	VkResult result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++m_framebuffer);

	if (result != VK_SUCCESS) {
		//clear attachments
		if (singleAttachment == VkManagedFrameBufferAttachment::ColorAttachment)
			m_colorAttachment->Clear();
		else if (singleAttachment == VkManagedFrameBufferAttachment::DepthAttachment)
			m_depthAttachment->Clear();

		throw std::runtime_error("Unable to create frame buffer, reason: " + Vulkan::VkResultToString(result));
	}
}

void Vulkan::VkManagedFrameBuffer::Build(VkExtent2D extent, bool sampleColor, bool copyColor,  bool sampleDepth, bool copyDepth, VkFormat colorFormat, VkFormat depthFormat)
{
	assert(depthFormat != VK_FORMAT_UNDEFINED);
	assert(colorFormat != VK_FORMAT_UNDEFINED);

	uint32_t usage = 0;
	std::vector<VkImageView> attachments;

	if (!m_mdevice->CheckFormatFeature(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL))
	{
		throw std::invalid_argument("Provided color format does not support optimal tiling");
	}

	usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (sampleColor)
		usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (copyColor)
		usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (m_colorAttachment == nullptr)
		m_colorAttachment = new VkManagedImage(m_mdevice);

	m_colorAttachment->Build(extent, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, usage);

	attachments.push_back(*m_colorAttachment);

	if (!m_mdevice->CheckFormatFeature(VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL))
	{
		throw std::invalid_argument("Provided depth format does not support optimal tiling");
	}

	usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (sampleDepth)
		usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (copyDepth)
		usage = usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (m_depthAttachment == nullptr)
		m_depthAttachment = new VkManagedImage(m_mdevice);

	uint32_t depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D16_UNORM_S8_UINT)
		depthAspect = depthAspect | VK_IMAGE_ASPECT_STENCIL_BIT;

	m_depthAttachment->Build(extent, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, VK_IMAGE_TILING_OPTIMAL, depthFormat, depthAspect, usage);
	attachments.push_back(*m_depthAttachment);
	
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_pass;
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;
	VkResult result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, ++m_framebuffer);

	if (result != VK_SUCCESS) {
		//clear attachments
		m_colorAttachment->Clear();
		m_depthAttachment->Clear();

		throw std::runtime_error("Unable to create frame buffer, reason: " + Vulkan::VkResultToString(result));
	}
}

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
