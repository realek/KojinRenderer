#pragma once
#include <glm\vec4.hpp>
namespace Vulkan
{
	class Texture2D;
	class Material
	{
	public:
		glm::vec4 colorTint;
		Texture2D * albedo;
		float specularity;
		Material();
		~Material();
	private:
	};
}