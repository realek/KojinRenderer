#pragma once
#include <glm\vec3.hpp>
#include <vulkan\vulkan.h>
#include "VulkanSystemStructs.h"
namespace Vulkan
{
	class KojinCamera
	{
	public:
		KojinCamera();
		void SetPosition(glm::vec3 position);
		void SetRotation(glm::vec3 rotation);
		void SetCameraOrigin(glm::vec2 screenCoords);
		void SetViewPortScale(glm::vec2 scale);
		void Bind();
		void UnBind();

	public:
		glm::vec3 position;
		glm::vec3 rotation;
		float fov;
		float zNear;
		float zFar;

	private:
		VkViewport m_cameraViewport;
		VkRect2D m_cameraScissor;
		CameraUniformBufferObject m_cameraUniformData;
		size_t m_cameraID;

	};
}