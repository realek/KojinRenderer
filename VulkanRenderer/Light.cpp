#include "Light.h"
#include "KojinRenderer.h"
#include "VKWorldSpace.h"
#include <glm\gtx\euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\quaternion.hpp>
void Vulkan::Light::SetType(Vulkan::LightType type)
{
	m_lightType = (int)type;
}

glm::vec4 Vulkan::Light::GetLightForward()
{
	auto rotY = glm::eulerAngleY(glm::radians(-rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(rotation.x));

	return rotX*rotY*glm::vec4(VkWorldSpace::WORLD_FORWARD, 0.0f);
}

glm::mat4 Vulkan::Light::GetLightViewMatrix()
{
	//auto rotZ = glm::eulerAngleZ(glm::radians(-rotation.z));
	auto rotY = glm::eulerAngleY(glm::radians(-rotation.y));
	auto rotX = glm::eulerAngleX(glm::radians(rotation.x));
	//glm::mat4 look = glm::lookAt(position, target, VkWorldSpace::WORLD_UP);
	glm::mat4 look;
	switch(m_lightType)
	{
	case Directional:
		look = glm::translate(glm::vec3(0))*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);
		break;
	case Spot:
		look = glm::translate(-position)*glm::scale(VkWorldSpace::AXES_WITH_LH_CORRECTION);
	}


	return look*(rotY*rotX);
}

/*
*By default creates a point light.
*/
Vulkan::Light::Light(KojinRenderer * rend, std::function<void(Light*,KojinRenderer*)> deleter, glm::vec3 initialPosition)
{
	position = initialPosition;
	diffuseColor = { 1.0, 1.0, 1.0, 1.0 };
	intensity = 1.0f;
	angle = 45.0f;
	range = 1.0f;
	rotation = { 0,0,0 };
	m_lightType = LightType::Point;
	m_onDestroy = [deleter,rend](Light* light)
	{
		if(rend!=nullptr)
			deleter(light,rend);
	};
}


Vulkan::Light::~Light()
{
	if(m_bound)
		m_onDestroy(this);
}

Vulkan::LightType Vulkan::Light::GetType()
{
	return (LightType)m_lightType;
}
