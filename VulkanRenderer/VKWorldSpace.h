#pragma once
#include <glm\vec3.hpp>
#include <glm\vec4.hpp>
namespace VkWorldSpace
{
	static const glm::vec4 WORLD_ORIGIN(0.0, 0.0, 0.0, 1.0);
	static const glm::vec3 WORLD_UP(0.0f, -1.0f, 0.0f);
	static const glm::vec3 WORLD_FORWARD(0.0f, 0.0f, 1.0f);
	static const glm::vec3 WORLD_RIGHT(1.0f, 0.0f, 0.0f);
	static const glm::vec3 REVERSE_AXES(-1.0f, 1.0f, -1.0f);
	static const glm::vec3 AXES(1.0f, -1.0f, 1.0f);
}