#include "KojinRenderer.h"
#include "SPIRVShader.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanRenderUnit.h"
#include "KojinCamera.h"
#include "SDL2\SDL_syswm.h"

Vulkan::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };


	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	//auto sys = new VulkanSystem(
	//	window,
	//	appName,
	//	appVer,
	//	RENDER_ENGINE_NAME,
	//	engineVer);

	this->m_system = std::make_shared<Vulkan::VulkanSystem>(window,
		appName,
		appVer,
		RENDER_ENGINE_NAME,
		engineVer);

	this->m_commandUnit = std::make_shared<Vulkan::VulkanCommandUnit>();
	this->m_imageUnit = std::make_shared<Vulkan::VulkanImageUnit>();
	this->m_swapChainUnit = std::make_shared<Vulkan::VulkanSwapchainUnit>();
	this->m_renderUnit = std::make_shared<Vulkan::VulkanRenderUnit>();

	try
	{
		this->m_system->Initialize(-1, nullptr, w, h);
		this->m_commandUnit->Initialize(m_system);
		this->m_imageUnit->Initialize(m_system, m_commandUnit);
		this->m_swapChainUnit->Initialize(m_system, m_imageUnit);
		this->m_renderUnit->Initialize(m_system, m_commandUnit, m_imageUnit, m_swapChainUnit);
	}
	catch (...)
	{
		throw;
	}

	m_defaultCamera = std::make_shared<KojinCamera>(KojinCamera(this->m_swapChainUnit->swapChainExtent2D));

	BindCamera(m_defaultCamera, true);

}

Vulkan::KojinRenderer::~KojinRenderer()
{
}

void Vulkan::KojinRenderer::Load(std::shared_ptr<Vulkan::Mesh> mesh, std::shared_ptr<Vulkan::Material> material)
{
	
}

void Vulkan::KojinRenderer::BindCamera(const std::weak_ptr<KojinCamera>& camera,bool isMainCamera)
{
	if (camera.expired())
		return;

	auto cam = camera.lock();
	if (isMainCamera)
	{
		this->m_renderUnit->SetAsMainCamera(cam->m_cameraID, &cam->m_cameraViewport, &cam->m_cameraScissor);
		return;
	}
	this->m_renderUnit->AddCamera(cam->m_cameraID, &cam->m_cameraViewport, &cam->m_cameraScissor);
}

void Vulkan::KojinRenderer::UnbindCamera(std::weak_ptr<KojinCamera>& camera)
{
	if (camera.expired())
		return;

	auto cam = camera.lock();
	m_renderUnit->RemoveCamera(cam->m_cameraID);
}

void Vulkan::KojinRenderer::DrawSingleObject(Vulkan::Texture2D * texture, Vulkan::Mesh * mesh)
{
	m_renderUnit->Render(texture, mesh);
}

void Vulkan::KojinRenderer::Update(float deltaTime)
{
	m_renderUnit->UpdateStaticUniformBuffer(deltaTime);
}

void Vulkan::KojinRenderer::Render()
{
	//
	//pass created mesh
	//
	m_renderUnit->Render(nullptr, nullptr);
	m_renderUnit->PresentFrame();
}

void Vulkan::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->m_system->GetLogicalDevice());
}

std::shared_ptr<Vulkan::KojinCamera> Vulkan::KojinRenderer::GetDefaultCamera()
{
	return m_defaultCamera;
}

