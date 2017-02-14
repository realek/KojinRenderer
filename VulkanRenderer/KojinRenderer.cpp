#include "KojinRenderer.h"
#include "SPIRVShader.h"

Vulkan::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };
	auto system = new Vulkan::VulkanSystem{
		window,
		appName,
		appVer,
		RENDER_ENGINE_NAME,
		engineVer

	};
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	this->system = std::unique_ptr<Vulkan::VulkanSystem>(system);
	this->system->Initialize(-1, nullptr, w, h);

	auto renderUnit = new Vulkan::VulkanRenderUnit();
	this->renderUnit = std::unique_ptr<Vulkan::VulkanRenderUnit>(renderUnit);

	//dummy shader object
	Vulkan::SPIRVShader shader{
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/vert.spv"),
		Vulkan::SPIRVShader::ReadBinaryFile("shaders/frag.spv") };

	this->renderUnit->Initialize(this->system.get(), &shader);
	defaultCamera = new KojinCamera();
	defaultCamera->Bind();
	auto camera2 = new KojinCamera();
	camera2->SetCameraOrigin({ 0.65, 0.65 });
	camera2->SetViewPortScale({ 0.35,0.35 });
	camera2->Bind();
}

void Vulkan::KojinRenderer::DrawSingleObject(Vulkan::Texture2D * texture, Vulkan::Mesh * mesh)
{
	renderUnit->Render(texture, mesh);
}

void Vulkan::KojinRenderer::Update(float deltaTime)
{
	renderUnit->UpdateStaticUniformBuffer(deltaTime);
}

void Vulkan::KojinRenderer::Present()
{
	renderUnit->PresentFrame();
}

void Vulkan::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->system->GetCurrentLogical()->Get());
}

Vulkan::KojinCamera * Vulkan::KojinRenderer::GetDefaultCamera()
{
	return defaultCamera;
}

