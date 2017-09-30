
#include "VKWorldSpace.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm\gtx\euler_angles.hpp>
#include <functional>

std::atomic<uint32_t> Vulkan::Camera::globalID = 0;

Vulkan::Camera::Camera(VkExtent2D extent, bool perspective, CameraCallback callback) : id(++globalID)
{
	m_swapChainExtent = extent;
	m_viewPort = {};
	m_viewPort.width = (float)(extent.width);
	m_viewPort.height = (float)(extent.height);
	m_viewPort.x = 0;
	m_viewPort.y = 0;
	m_viewPort.minDepth = 0;
	m_viewPort.maxDepth = 1;

	m_scissor = {};
	m_scissor.extent = extent;
	m_scissor.offset = { 0,0 };

	m_fov = VkViewportDefaults::k_CameraFov;
	m_zNear = VkViewportDefaults::k_CameraZNear;
	m_zFar = VkViewportDefaults::k_CameraZFar;

	if (perspective)
		SetPerspective();
	else
		SetOrthographic();
	m_onDestroy = callback;
}

void Vulkan::Camera::ComputeViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::mat4 & viewMatrix)
{
	position.y *= -1;
	//auto target = position + VkWorldSpace::WORLD_FORWARD;
	auto rotZ = glm::eulerAngleZ(glm::radians(-rotation.z));
	auto rotY = glm::eulerAngleY(glm::radians(-rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(rotation.x));
	//glm::mat4 look = glm::lookAt(position, target, VkWorldSpace::WORLD_UP);
	auto look = glm::translate(-position)*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);

	viewMatrix = (rotX*rotY*rotZ)*look;
}

Vulkan::Camera::~Camera()
{
	if(m_bound)
		m_onDestroy(this);
}

void Vulkan::Camera::SetOrthographic(float orthoSize)
{
	float width = (float)this->m_swapChainExtent.width / (float)this->m_swapChainExtent.height;
	width *= orthoSize;
	m_projectionMatrix = glm::ortho(-width / 2, width/2, -orthoSize/2, orthoSize / 2, m_zNear, m_zFar);
}

void Vulkan::Camera::SetPerspective()
{
	float aspect = (float)this->m_swapChainExtent.width / (float)this->m_swapChainExtent.height;
	m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_zNear, m_zFar);
}

void Vulkan::Camera::Update() 
{
	ComputeViewMatrix(m_position, m_rotation, m_viewMatrix);
}

void Vulkan::Camera::SetPositionRotation(glm::vec3 position, glm::vec3 rotation)
{
	m_position = position;
	m_rotation = rotation;
	ComputeViewMatrix(m_position, m_rotation, m_viewMatrix);
}

void Vulkan::Camera::SetViewport(glm::vec2 screenCoords, glm::vec2 scale)
{

	m_viewPort.width = m_swapChainExtent.width * scale.x;
	m_viewPort.height = m_swapChainExtent.height * scale.y;
	m_scissor.extent.width = (uint32_t)m_viewPort.width;
	m_scissor.extent.height = (uint32_t)m_viewPort.height;
	screenCoords = glm::clamp(screenCoords, glm::vec2(0.0, 0.0), glm::vec2(1.0, 1.0));
	m_viewPort.x = (float)m_swapChainExtent.width*screenCoords.x;
	m_viewPort.y = (float)m_swapChainExtent.height*screenCoords.y;
	m_scissor.offset.x = (int32_t)m_viewPort.x;
	m_scissor.offset.y = (int32_t)m_viewPort.y;

}

void Vulkan::Camera::LookAt(glm::vec3 target)
{
	target.x *= -1; //flipped Y
	m_viewMatrix = glm::lookAt(m_position, target, VkWorldSpace::WORLD_UP);

	auto cosY = glm::sqrt(1 + (m_viewMatrix[0][2] / 2.0f)); //half z
	m_rotation.y = glm::degrees(-atan2(m_viewMatrix[0][2], cosY));
	m_rotation.x = glm::degrees(glm::atan(-m_viewMatrix[1][2], m_viewMatrix[2][2]));
	m_rotation.z = 0;
}
