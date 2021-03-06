#pragma once
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtx\euler_angles.hpp>
namespace VkWorldSpace
{
	static const glm::vec4 WORLD_ORIGIN(0.0, 0.0, 0.0, 1.0);
	static const glm::vec3 WORLD_UP(0.0f, -1.0f, 0.0f);
	static const glm::vec3 WORLD_FORWARD(0.0f, 0.0f, 1.0f);
	static const glm::vec3 WORLD_RIGHT(1.0f, 0.0f, 0.0f);
	static const glm::vec3 REVERSE_AXES(-1.0f, 1.0f, -1.0f);
	static const glm::vec3 AXES(1.0f, -1.0f, 1.0f);
	static const glm::vec3 AXES_WITH_LH_CORRECTION(-1.0f, -1.0f, 1.0f);
	static const float UNIT = 1.0f;

	static glm::mat4 ComputeModelMatrix(glm::vec3 m_position, glm::vec3 eulerRotation, glm::vec3 scale)
	{
		m_position.x *= -1; // flipping X due to flipping Y axis for the world
		auto rotX = glm::eulerAngleX(glm::radians(eulerRotation.x));
		auto rotY = glm::eulerAngleY(glm::radians(-eulerRotation.y));
		auto rotZ = glm::eulerAngleZ(glm::radians(-eulerRotation.z));
		auto model = glm::translate(glm::mat4(1), m_position);
		model = glm::scale(model*rotZ*rotY*rotX, scale);
		return model;

	}
}