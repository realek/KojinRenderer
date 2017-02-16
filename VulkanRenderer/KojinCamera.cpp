#include "KojinCamera.h"
#include "VulkanRenderUnit.h"

std::atomic<int> Vulkan::KojinCamera::globalID = 0;

Vulkan::KojinCamera::KojinCamera(VkExtent2D swapChainExtent) : m_cameraID(++globalID)
{
	m_swapChainExtent = swapChainExtent;
	m_cameraViewport = {};
	m_cameraViewport.width = (float)swapChainExtent.width;
	m_cameraViewport.height = (float)swapChainExtent.height;
	m_cameraViewport.x = 0;
	m_cameraViewport.y = 0;
	m_cameraViewport.minDepth = 0;
	m_cameraViewport.maxDepth = 1;

	m_cameraScissor = {};
	m_cameraScissor.extent = swapChainExtent;
	m_cameraScissor.offset = { 0,0 };

	position = { 0,0,0 };
	rotation = { 0,0,0 };

}

Vulkan::KojinCamera::~KojinCamera()
{

}

void Vulkan::KojinCamera::SetPosition(glm::vec3 position)
{
}

void Vulkan::KojinCamera::SetRotation(glm::vec3 rotation)
{
}



void Vulkan::KojinCamera::SetViewport(glm::vec2 screenCoords, glm::vec2 scale)
{

	m_cameraViewport.width = m_swapChainExtent.width * scale.x;
	m_cameraViewport.height = m_swapChainExtent.height * scale.y;
	m_cameraScissor.extent.width = (uint32_t)m_cameraViewport.width;
	m_cameraScissor.extent.height = (uint32_t)m_cameraViewport.height;
	screenCoords = glm::clamp(screenCoords, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));
	m_cameraViewport.x = (float)m_swapChainExtent.width*screenCoords.x;
	m_cameraViewport.y = (float)m_swapChainExtent.height*screenCoords.y;
	m_cameraScissor.offset.x = (int32_t)m_cameraViewport.x;
	m_cameraScissor.offset.y = (int32_t)m_cameraViewport.y;

}