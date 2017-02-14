#include "KojinCamera.h"
#include "VulkanRenderUnit.h"

std::atomic<int> Vulkan::KojinCamera::globalID = 0;
Vulkan::KojinCamera::KojinCamera() : m_cameraID(++globalID)
{
	auto swapChainExtent = VulkanSwapChainUnit::swapChainExtent2D;
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

	m_cameraUniformData = {};
	m_cameraUniformData.view = glm::mat4(1);


}

Vulkan::KojinCamera::~KojinCamera()
{
	UnBind();
}

void Vulkan::KojinCamera::SetPosition(glm::vec3 position)
{
}

void Vulkan::KojinCamera::SetRotation(glm::vec3 rotation)
{
}



void Vulkan::KojinCamera::SetCameraOrigin(glm::vec2 screenCoords)
{
	screenCoords = glm::clamp(screenCoords, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));
	m_cameraViewport.x = screenCoords.x;
	m_cameraViewport.y = screenCoords.y;
}

void Vulkan::KojinCamera::SetViewPortScale(glm::vec2 scale)
{
	auto swapChainExtent = VulkanSwapChainUnit::swapChainExtent2D;
	m_cameraViewport.width = swapChainExtent.width * scale.x;
	m_cameraViewport.height = swapChainExtent.height * scale.y;
	m_cameraScissor.extent.width = m_cameraViewport.width;
	m_cameraScissor.extent.height = m_cameraViewport.height;
}

void Vulkan::KojinCamera::Bind()
{
	VulkanRenderUnit::AddCamera(m_cameraID,&m_cameraViewport, &m_cameraScissor);
}

void Vulkan::KojinCamera::UnBind()
{
	VulkanRenderUnit::RemoveCamera(m_cameraID);
}
