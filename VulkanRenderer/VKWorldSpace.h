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

	static glm::mat4 ComputeModelMatrix(glm::vec3 position, glm::vec3 eulerRotation, glm::vec3 scale)
	{
		auto rotX = glm::eulerAngleX(glm::radians(eulerRotation.x));
		auto rotY = glm::eulerAngleY(glm::radians(-eulerRotation.y));
		auto rotZ = glm::eulerAngleZ(glm::radians(-eulerRotation.z));
		auto model = glm::translate(glm::mat4(1), position);
		model = glm::scale(rotZ*rotY*rotX*model, scale);
		return model;

	}
}