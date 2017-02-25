#include "KojinLight.h"

void Vulkan::KojinLight::SetType(Vulkan::LightType type)
{
	m_lightType = (int)type;
}

Vulkan::KojinLight::~KojinLight()
{
}

Vulkan::LightType Vulkan::KojinLight::GetType()
{
	return (LightType)m_lightType;
}
