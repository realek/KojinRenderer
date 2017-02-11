#pragma once
#include "KojinRenderer.h"
#include "Texture2D.h"
#include "Mesh.h"
#include "Filesystem.h"
#include "SPIRVShader.h"
#define SDL_MAIN_HANDLED
#include <SDL2\SDL.h>
#include <SDL_image.h>
#include <SDL2\SDL_syswm.h>
#include <iostream>
#include <assert.h>
#include <chrono>
#ifdef _WIN32
#include <Windows.h>
#include <atlconv.h>
#endif
SDL_Window * window;

#ifdef _WIN32
//HWND GetHwnd() {
//	SDL_SysWMinfo windowinfo;
//	
//	if (!SDL_GetWMInfo(&windowinfo)) return(NULL);
//	return(windowinfo.window);
//}

#endif // _WIN32

bool SDLExitInput()
{
	SDL_Event evt;
	SDL_PollEvent(&evt);

	switch (evt.type)
	{
		case SDL_EventType::SDL_KEYDOWN:
		{
			if (evt.key.keysym.sym == SDLK_ESCAPE)
				return false;
			else
				return true;
		}

		case SDL_QUIT:
		{
			return false;
		}
	default:
		return true;
	}

}

int main() 
{
	int width = 1280;
	int height = 720;
	// Create an SDL window that supports Vulkan and OpenGL rendering.
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Could not initialize SDL." << std::endl;
	}

	if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF) == 0)
		std::cout << "Unable to initialize SDL image."<<std::endl;


	window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		std::cout << "Could not create SDL window." << std::endl;
		assert(window != nullptr);
	}




	int appVer[3] = { 1,0,0 };
	//int engVer[3] = { 1,0,0 };

	//Vk::VulkanSystem * baseSystem = nullptr;
	//Vk::VulkanRenderUnit * renderUnit = nullptr;
	//auto vertShaderCode = FileSystem::ReadBinaryFile("shaders/vert.spv");
	//auto fragShaderCode = FileSystem::ReadBinaryFile("shaders/frag.spv");
	//Vk::SPIRVShader baseShader(vertShaderCode,fragShaderCode);
	Vk::KojinRenderer * renderer = nullptr;
	Vk::Texture2D * tex = nullptr;
	Vk::Mesh * mesh = nullptr;
	try
	{
		//baseSystem = new Vk::VulkanSystem{ window,"My vulkan App",appVer,"My vulkan Engine",engVer };
		//baseSystem->Initialize(-1, nullptr, width, height);
		//renderUnit = new Vk::VulkanRenderUnit();
		//renderUnit->Initialize(baseSystem,&baseShader);
		renderer = new Vk::KojinRenderer{window,"Vulkan Tester",appVer};
		tex = Vk::Texture2D::CreateFromFile("textures/model.png");
		mesh = Vk::Mesh::LoadMesh("models/model.obj");
	}
	catch(std::runtime_error e)
	{
#ifdef _WIN32

		int length = strlen(e.what());
		wchar_t errorText[4096] = {0};
		MultiByteToWideChar(0, 0, e.what(), length, errorText, length);



		int msgboxID = MessageBox(
			NULL,
			(LPCWSTR)errorText,
			(LPCWSTR)L"Runtime Error",
			MB_ICONERROR | MB_OK
		);
		
		switch (msgboxID)
		{
		case IDOK:
			delete(renderer);
			IMG_Quit();
			SDL_Quit();
			return 0;
		}

#else
		throw std::runtime_error(e.what());
#endif // _WIN32
	}


	//load stuff into renderer

	renderer->DrawSingleObject(tex, mesh);

	//


	bool running = true;
	auto startTime = std::chrono::high_resolution_clock::now();
	float currentDelta = 0.0;
	float fixedTimeStep = 1/60.0f;

	while(running)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float frameDeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
		currentDelta += frameDeltaTime;

		//input read
		running = SDLExitInput();

		//do updates with delta time here
		while(currentDelta>=fixedTimeStep)
		{
			
			renderer->Update(currentDelta);
			//if (currentDelta == fixedTimeStep)
			//{
			//	//update physics
			//}
			currentDelta -= fixedTimeStep;
		}

		if(currentDelta>0 && currentDelta < fixedTimeStep)
			SDL_Delay((uint32_t)currentDelta);
		//

		//present

		renderer->Present();
		//

		startTime = currentTime;

	}
	renderer->WaitForIdle();


	std::cout << "Vulkan resource cleanup, press any key to start." << std::endl;
	//getchar();
	delete(renderer);
	std::cout <<std::endl<< "Finished Vulkan resource cleanup, press any key to end application." << std::endl;
	getchar();
	IMG_Quit();
	SDL_Quit();
	return 0;
}