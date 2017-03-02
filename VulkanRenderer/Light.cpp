#include "Light.h"
#include "KojinRenderer.h"

void Vulkan::Light::SetType(Vulkan::LightType type)
{
	m_lightType = (int)type;
}

Vulkan::Light::Light(KojinRenderer * rend, std::function<void(Light*,KojinRenderer*)> deleter, glm::vec3 initialPosition)
{
	position = initialPosition;
	diffuseColor = { 1.0, 1.0, 1.0, 1.0 };
	specularColor = { 1.0, 1.0, 1.0, 1.0 };
	rotation = { 0,0,0 };
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