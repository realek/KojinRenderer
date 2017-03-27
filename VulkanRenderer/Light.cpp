#include "Light.h"
#include "VKWorldSpace.h"
#include <glm\gtx\euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\quaternion.hpp>

std::atomic<uint32_t> Vulkan::Light::globalID = 0;
void Vulkan::Light::SetType(Vulkan::LightType type)
{
	m_lightType = type;
}

glm::vec4 Vulkan::Light::GetLightForward()
{
	auto rotY = glm::eulerAngleY(glm::radians(-m_rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(m_rotation.x));

	return rotX*rotY*glm::vec4(VkWorldSpace::WORLD_FORWARD, 0.0f);
}

glm::mat4 Vulkan::Light::GetLightViewMatrix()
{
	auto rotY = glm::eulerAngleY(glm::radians(-m_rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(m_rotation.x));
	glm::mat4 look;
	auto fPos = -m_position;
	switch(m_lightType)
	{
		case Directional:
		{
			look = glm::translate(VkWorldSpace::WORLD_FORWARD)*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);
			return (rotX*rotY)*look;
		}
		case Spot:
		{

			fPos.y *= -1;
			look = glm::translate(fPos)*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);
			return (rotX*rotY)*look;
		}
		default:
		{
			return glm::mat4();
		}
	}
}

/*
*By default creates a point light.
*/
Vulkan::Light::Light(VulkanRenderUnit * rend, std::function<void(VulkanRenderUnit*, int)> deleter, glm::vec3 initialPosition) : id(++globalID)
{
	m_position = initialPosition;
	diffuseColor = { 1.0, 1.0, 1.0, 1.0 };
	intensity = 1.0f;
	angle = 45.0f;
	range = 1.0f;
	m_rotation = { 0,0,0 };

	m_onDestroy = [deleter, rend](Light* light)
	{
		if (rend != nullptr)
			deleter(rend,light->id);
	};
}


Vulkan::Light::~Light()
{
	if(m_bound)
		m_onDestroy(this);
}

Vulkan::LightType Vulkan::Light::GetType()
{
	return m_lightType;
}
