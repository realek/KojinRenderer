#pragma once
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\matrix.hpp>
#include <functional>
#include <atomic>
namespace Vulkan
{
	enum LightType
	{
		Point = 0,
		Spot = 1,
		Directional = 2
	};

	class KojinRenderer;
	class VulkanRenderUnit;

	class Light
	{

	public:
		~Light();
		glm::vec4 diffuseColor;
		glm::vec3 m_position;
		glm::vec3 m_rotation;
		float intensity;
		float range;
		float angle;
		const uint32_t id;
		LightType GetType();
		void SetType(LightType type);
		glm::vec4 GetLightForward();
		glm::mat4 GetLightViewMatrix();
		glm::mat4 GetLightProjectionMatrix();
	private:

		static std::atomic<uint32_t> globalID;
		Light(VulkanRenderUnit* rend, std::function<void(VulkanRenderUnit*, int)> deleter, glm::vec3 initialPosition);
		LightType m_lightType = LightType::Point;
		std::function<void(Light*)> m_onDestroy;
		bool m_bound = false;
		friend class KojinRenderer;

	};
}