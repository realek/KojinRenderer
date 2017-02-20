/*====================================================
KojinCamera.h abstraction class used to add camera 
functionality to the KojinRenderer class
====================================================*/

#pragma once
#include <glm\vec3.hpp>
#include <vulkan\vulkan.h>
#include "VulkanSystemStructs.h"
#include <atomic>
namespace Vulkan
{

	class KojinRenderer;
	class KojinCamera
	{
	public:

		~KojinCamera();
		void SetOrthographic();
		void SetPerspective();
		void SetPosition(glm::vec3 position);
		void SetRotation(glm::vec3 rotation);
		void SetViewport(glm::vec2 screenCoords, glm::vec2 scale);
		void LookAt(glm::vec3 target);

	private:
		KojinCamera(VkExtent2D swapChainExt);

	private:
		glm::vec3 m_position;
		glm::vec3 m_rotation;
		float m_fov;
		float m_zNear;
		float m_zFar;
		VkExtent2D m_swapChainExtent;
		VkViewport m_cameraViewport;
		VkRect2D m_cameraScissor;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionMatrix;
		int m_cameraID;
		static std::atomic<int> globalID;
		friend class KojinRenderer;
	};
}