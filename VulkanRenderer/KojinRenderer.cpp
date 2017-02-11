#include "KojinRenderer.h"
#include "SPIRVShader.h"

Vk::KojinRenderer::KojinRenderer(SDL_Window * window, const char * appName, int appVer[3])
{
	int engineVer[3] = { RENDER_ENGINE_MAJOR_VERSION,RENDER_ENGINE_PATCH_VERSION,RENDER_ENGINE_MINOR_VERSION };
	auto system = new Vk::VulkanSystem{
		window,
		appName,
		appVer,
		RENDER_ENGINE_NAME,
		engineVer

	};
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	this->system = std::unique_ptr<Vk::VulkanSystem>(system);
	this->system->Initialize(-1, nullptr, w, h);

	auto renderUnit = new Vk::VulkanRenderUnit();
	this->renderUnit = std::unique_ptr<Vk::VulkanRenderUnit>(renderUnit);
	
	//dummy shader object
	Vk::SPIRVShader shader{
		Vk::SPIRVShader::ReadBinaryFile("shaders/vert.spv"),
		Vk::SPIRVShader::ReadBinaryFile("shaders/frag.spv")};

	this->renderUnit->Initialize(this->system.get(),&shader);
}

void Vk::KojinRenderer::DrawSingleObject(Vk::Texture2D * texture, Vk::Mesh * mesh)
{
	renderUnit->Render(texture, mesh);
}

void Vk::KojinRenderer::Update(float deltaTime)
{
	renderUnit->UpdateStaticUniformBuffer(deltaTime);
}

void Vk::KojinRenderer::Present()
{
	renderUnit->PresentFrame();
}

void Vk::KojinRenderer::WaitForIdle()
{
	vkDeviceWaitIdle(this->system->GetCurrentLogical()->Get());
}

