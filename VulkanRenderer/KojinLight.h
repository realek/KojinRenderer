#pragma once
#include <glm\vec3.hpp>

namespace Vulkan
{
	enum LightType
	{
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	class KojinLight
	{
	public:
		~KojinLight();
		glm::vec3 position;
		glm::vec3 rotation;
		float range;
		float angle;
		LightType GetType();
		void SetType(LightType type);
	private:
		KojinLight();
		int m_lightType = 0;

	};
}