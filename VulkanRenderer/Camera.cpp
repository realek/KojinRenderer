
#include "VKWorldSpace.h"
#include "Camera.h"
#include "VulkanRenderUnit.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm\gtx\euler_angles.hpp>
#include <functional>

std::atomic<int> Vulkan::KojinCamera::globalID = 0;


Vulkan::KojinCamera::KojinCamera(KojinRenderer* rend, std::function<void(KojinRenderer*, KojinCamera*)> bindFunction, std::function<void(KojinRenderer*, KojinCamera*)> deleter, VkExtent2D swapChainExtent, bool perspective) : m_cameraID(++globalID)
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

	m_fov = VkViewportDefaultSettings::k_Fov;
	m_zNear = VkViewportDefaultSettings::k_zNear;
	m_zFar = VkViewportDefaultSettings::k_zFar;



	m_rotation = { 0,0,0 };

	if (perspective)
		SetPerspective();
	else
		SetOrthographic();

	onBind = [rend,bindFunction](KojinCamera* camera)
	{
		bindFunction(rend, camera);
	};

	onDestroy = [rend,deleter](KojinCamera* camera)
	{
		deleter(rend,camera);
	};

}

void Vulkan::KojinCamera::BindSelf()
{
	onBind(this);
	m_bound = true;
}

void Vulkan::KojinCamera::ComputeViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::mat4 & viewMatrix)
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

Vulkan::KojinCamera::~KojinCamera()
{
	if(m_bound)
		onDestroy(this);
}

void Vulkan::KojinCamera::SetOrthographic(float orthoSize)
{
	float width = (float)this->m_swapChainExtent.width / (float)this->m_swapChainExtent.height;
	width *= orthoSize;
	m_projectionMatrix = glm::ortho(-width / 2, width/2, -orthoSize/2, orthoSize / 2, m_zNear, m_zFar);
}

void Vulkan::KojinCamera::SetPerspective()
{
	float aspect = (float)this->m_swapChainExtent.width / (float)this->m_swapChainExtent.height;
	m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_zNear, m_zFar);
}

void Vulkan::KojinCamera::SetPosition(glm::vec3 position)
{
	m_position = position;
	ComputeViewMatrix(m_position, m_rotation, m_viewMatrix);

}

void Vulkan::KojinCamera::SetRotation(glm::vec3 rotation)
{
	this->m_rotation = rotation;
	ComputeViewMatrix(m_position,m_rotation,m_viewMatrix);
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
	m_viewMatrix = glm::lookAt(m_position, target, VkWorldSpace::WORLD_UP);

}
