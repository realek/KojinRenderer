#include "KojinCamera.h"
#include "VulkanRenderUnit.h"
//#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define ZFAR_DEFAULT 100;
#define ZNEAR_DEFAULT 0.0001f;
#define FOV_DEFAULT 60;

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

	m_fov = FOV_DEFAULT;
	m_zNear = ZNEAR_DEFAULT;
	m_zFar = ZFAR_DEFAULT;

	m_rotation = { 0,0,0 };
	SetPosition({ 0,0,0 });

}

Vulkan::KojinCamera::~KojinCamera()
{

}

void Vulkan::KojinCamera::SetOrthographic()
{
	//projection = glm::ortho(0.0f, (float)m_swapChainExtent.width, (float)m_swapChainExtent.height, 0.0f, zNear, zFar);
}



void Vulkan::KojinCamera::SetPerspective()
{
	float aspect = (float)this->m_swapChainExtent.width / (float)this->m_swapChainExtent.height;
	m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_zNear, m_zFar);
	m_projectionMatrix[1][1] *= -1;
}

void Vulkan::KojinCamera::SetPosition(glm::vec3 position)
{
	this->m_position = position;
	auto tMat = glm::translate(glm::mat4(1), position);
	auto rMat = glm::rotate(glm::mat4(1), glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rMat = glm::rotate(rMat, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rMat = glm::rotate(rMat, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	m_viewMatrix = tMat * rMat;

}

void Vulkan::KojinCamera::SetRotation(glm::vec3 rotation)
{
	this->m_rotation = rotation;
	auto tMat = glm::translate(glm::mat4(1), m_position);
	auto rMat = glm::rotate(glm::mat4(1), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rMat = glm::rotate(rMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rMat = glm::rotate(rMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	m_viewMatrix = tMat * rMat;
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

void Vulkan::KojinCamera::LookAt(glm::vec3 target)
{
	m_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), (target+glm::vec3(0,0,-1)), glm::vec3(0.0f, 1.0f, 0.0f)); 

}
