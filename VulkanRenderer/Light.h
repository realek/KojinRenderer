#pragma once
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <functional>

namespace Vulkan
{
	enum LightType
	{
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	class KojinRenderer;
	class Light
	{

	public:
		~Light();
		glm::vec4 diffuseColor;
		glm::vec4 specularColor;
		glm::vec3 position;
		glm::vec3 rotation;
		float range;
		float angle;
		LightType GetType();
		void SetType(LightType type);
	private:
		Light(KojinRenderer * rend, std::function<void(Light*, KojinRenderer*)> deleter, glm::vec3 initialPosition);
		int m_lightType = 0;
		std::function<void(Light*)> m_onDestroy;
		bool m_bound = false;
		friend class KojinRenderer;

	};
}