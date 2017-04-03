/*====================================================
Camera.h abstraction class used to add camera 
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
	class VulkanRenderUnit;
	class Camera
	{
	public:

		~Camera();
		const uint32_t id;
		void SetOrthographic(float orthoSize = VkViewportDefaults::k_CameraOrthoSize);
		void SetPerspective();
		void SetAsMain();
		void SetPositionRotation(glm::vec3 position, glm::vec3 rotation);
		void SetViewport(glm::vec2 screenCoords, glm::vec2 scale);
		void LookAt(glm::vec3 target);
		glm::vec3 m_position;
		glm::vec3 m_rotation;
	private:
		Camera(VkExtent2D extent, bool perspective, VulkanRenderUnit * rend, std::function<void(VulkanRenderUnit*, Camera*)> onSetAsMain, std::function<void(VulkanRenderUnit*, uint32_t)> deleter);
		void ComputeViewMatrix(glm::vec3 position, glm::vec3 rotation, glm::mat4& viewMatrix);

	private:
		bool m_bound = false;
		float m_fov;
		float m_zNear;
		float m_zFar;
		VkExtent2D m_swapChainExtent;
		VkViewport m_viewPort;
		VkRect2D m_scissor;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionMatrix;
		static std::atomic<uint32_t> globalID;
		std::function<void(Camera*)> onDestroy;
		std::function<void(Camera*)> onSetAsMain;
		friend class VulkanRenderUnit;
		friend class KojinRenderer;
	};
}