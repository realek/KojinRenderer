#pragma once
#include <glm\vec4.hpp>
#include <memory>
namespace Vulkan
{
	class Texture2D;
	class Material
	{
	public:
		glm::vec4 colorTint;
		std::weak_ptr<Vulkan::Texture2D> albedo;
		float specularity;
		Material();
		~Material();
	private:
	};
}