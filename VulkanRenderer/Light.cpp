#include "Light.h"
#include "KojinRenderer.h"

void Vulkan::Light::SetType(Vulkan::LightType type)
{
	m_lightType = (int)type;
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
