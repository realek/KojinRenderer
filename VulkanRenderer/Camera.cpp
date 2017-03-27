
#include "VKWorldSpace.h"
#include "Camera.h"
#include "VulkanRenderUnit.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm\gtx\euler_angles.hpp>
#include <functional>

std::atomic<uint32_t> Vulkan::Camera::globalID = 0;

Vulkan::Camera::Camera(VkExtent2D extent, bool perspective, VulkanRenderUnit * rend, std::function<void(VulkanRenderUnit*, uint32_t)> deleter) : id(++globalID)
{
	m_swapChainExtent = extent;
	m_viewPort = {};
	m_viewPort.width = static_cast<float>(extent.width);
	m_viewPort.height = static_cast<float>(extent.height);
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

	m_rotation = { 0,0,0 };

	if (perspective)
		SetPerspective();
	else
		SetOrthographic();

	onDestroy = [rend, deleter](Camera* camera)
	{
		deleter(rend, camera->id);
	};;
}

void Vulkan::Camera::ComputeViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::mat4 & viewMatrix)
{
	m_position.y *= -1;
	//auto target = position + VkWorldSpace::WORLD_FORWARD;
	auto rotZ = glm::eulerAngleZ(glm::radians(-m_rotation.z));
	auto rotY = glm::eulerAngleY(glm::radians(-m_rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(m_rotation.x));
	//glm::mat4 look = glm::lookAt(position, target, VkWorldSpace::WORLD_UP);
	auto look = glm::translate(-m_position)*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);

	viewMatrix = (rotX*rotY*rotZ)*look;
}

Vulkan::Camera::~Camera()
{
	if(m_bound)
		onDestroy(this);
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
	m_viewMatrix = glm::lookAt(m_position, target, VkWorldSpace::WORLD_UP);

	if (m_viewMatrix[1][1] == 1.0f || m_viewMatrix[1][1] == -1.0f)
	{
		m_rotation.y = glm::degrees(glm::atan(m_viewMatrix[1][3], m_viewMatrix[3][4])); //might be broken
		m_rotation.x = 0;
		m_rotation.z = 0;

	}
	else
	{

		m_rotation.y = glm::degrees(glm::atan(-m_viewMatrix[3][1], -m_viewMatrix[1][1]));
		m_rotation.x = glm::degrees(glm::asin(-m_viewMatrix[2][1]));
		m_rotation.z = 0;//glm::degrees(glm::atan(m_viewMatrix[2][3], m_viewMatrix[2][2]));
	}
}
