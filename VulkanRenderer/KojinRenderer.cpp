#include "KojinRenderer.h"
#include "SPIRVShader.h"
#include "VulkanSystem.h"
#include "VulkanCommandUnit.h"
#include "VulkanImageUnit.h"
#include "VulkanSwapChainUnit.h"
#include "VulkanRenderUnit.h"
#include "KojinCamera.h"
//#include "vulkan\vulkan.h"
#include "SDL2\SDL_syswm.h"
Vulkan::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };


	//make and init system
	{
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		auto system = new Vulkan::VulkanSystem{
			window,
			appName,
			appVer,
			RENDER_ENGINE_NAME,
			engineVer

		};
		this->m_system = std::unique_ptr<Vulkan::VulkanSystem>(system);
		this->m_system->Initialize(-1, nullptr, w, h);
	}
	


	////make and init command unit
	//{
	//	auto cmdUnit = new Vulkan::VulkanCommandUnit();
	//	this->m_commandUnit = std::unique_ptr<Vulkan::VulkanCommandUnit>(cmdUnit);
	//	this->m_commandUnit->Initialize(this->m_system.get());
	//}

	////make and init image unit
	//{
	//	auto imgUnit = new Vulkan::VulkanImageUnit();
	//	this->m_imageUnit = std::unique_ptr<Vulkan::VulkanImageUnit>(imgUnit);
	//	auto cmdUnit = this->m_commandUnit.get();
	//	this->m_imageUnit->Initialize(m_system->GetCurrentPhysical(), m_system->GetCurrentLogical(), cmdUnit);
	//}

	//{
	//	auto swpChain = new Vulkan::VulkanSwapChainUnit();
	//	this->m_swapChainUnit = std::unique_ptr<Vulkan::VulkanSwapChainUnit>(swpChain);
	//	this->m_swapChainUnit->Initialize(m_system.get());
	//}


//	imageUnit->Initialize

	auto renderUnit = new Vulkan::VulkanRenderUnit();
	this->m_renderUnit = std::unique_ptr<Vulkan::VulkanRenderUnit>(renderUnit);

	//dummy shader object
	Vulkan::SPIRVShader shader{
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/vert.spv"),
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/frag.spv") };

	this->m_renderUnit->Initialize(this->m_system.get(), &shader);
	defaultCamera = new KojinCamera(this->m_renderUnit->swapChainExt);
	BindCamera(defaultCamera, true);
	auto camera2 = new KojinCamera(this->m_renderUnit->swapChainExt);
	camera2->SetViewport({ 0.35, 0.0 }, { 0.4,0.4 });
	BindCamera(camera2);
}

Vulkan::KojinRenderer::~KojinRenderer()
{
	delete(defaultCamera);
}

void Vulkan::KojinRenderer::BindCamera(KojinCamera * camera,bool isMainCamera)
{
	if (isMainCamera)
	{
		this->m_renderUnit->SetAsMainCamera(camera->GetID(), camera->GetViewport(), camera->GetScissor());
		return;
	}
	this->m_renderUnit->AddCamera(camera->GetID(), camera->GetViewport(), camera->GetScissor());
}

void Vulkan::KojinRenderer::UnbindCamera(KojinCamera * camera)
{
	m_renderUnit->RemoveCamera(camera->GetID());
}

void Vulkan::KojinRenderer::DrawSingleObject(Vulkan::Texture2D * texture, Vulkan::Mesh * mesh)
{
	m_renderUnit->Render(texture, mesh);
}

void Vulkan::KojinRenderer::Update(float deltaTime)
{
	m_renderUnit->UpdateStaticUniformBuffer(deltaTime);
}

void Vulkan::KojinRenderer::Present()
{
	m_renderUnit->PresentFrame();
}

void Vulkan::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->m_system->LogicalDevice());
}

Vulkan::KojinCamera * Vulkan::KojinRenderer::GetDefaultCamera()
{
	return defaultCamera;
}

