/*=============================================================================
ENTRY POINT - Used to test current functionality of the renderer.
=============================================================================*/


#pragma once
#include "KojinRenderer.h"
#include "Texture2D.h"
#include "Material.h"
#include "Mesh.h"
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

HWND GetHwnd(SDL_Window * window) {
	SDL_SysWMinfo windowinfo;
	
	if (!SDL_GetWindowWMInfo(window, &windowinfo)) {
		throw std::runtime_error("SDL2 Window Manager info couldn't be retrieved.");
	}
	return (HWND)windowinfo.subsystem;
}

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

void InitSDL()
{
	int width = 1280;
	int height = 720;
	// Create an SDL window that supports Vulkan and OpenGL rendering.
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "Could not initialize SDL." << std::endl;
	}

	window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
	if (window == NULL) {
		std::cout << "Could not create SDL window." << std::endl;
		assert(window != nullptr);
	}

	int result = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	if (result == 0)
	{
		std::cout << "Unable to initialize SDL image." << std::endl;
		assert(result == 0);
	}
}

int main() 
{
	
	InitSDL();

	int appVer[3] = { 0,0,0 };
	Vulkan::KojinRenderer * renderer = nullptr;
	std::shared_ptr<Vulkan::Mesh> mesh;
	std::shared_ptr<Vulkan::Material> material = std::make_shared<Vulkan::Material>(Vulkan::Material());
	try
	{
		renderer = new Vulkan::KojinRenderer{window,"Vulkan Tester",appVer};
		material->albedo = Vulkan::Texture2D::CreateFromFile("textures/Stormtrooper_Diffuse.png");
		//material->albedo = Vk::Texture2D::GetWhiteTexture();
		mesh = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");

	}
	catch(std::runtime_error e)
	{
		//WINDOWS APITEST BLOCK -- EXCEPTION TEST
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
#endif // _WIN32 //END WINDOWS API TEST BOCK
	}

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
			//update objects here
			renderer->Update(currentDelta);
			//if (currentDelta == fixedTimeStep)
			//{
			//	//update physics
			//}
			currentDelta -= fixedTimeStep;
		}

		if(currentDelta>0 && currentDelta < fixedTimeStep)
			SDL_Delay((uint32_t)currentDelta);
		
		//load up objects to the renderer
		//renderer->Load(mesh, material);
		renderer->DrawSingleObject(material->albedo.lock()->ImageView(), mesh.get());
		//!load up objects -- currently single load

		//present

		renderer->Render();
		//

		startTime = currentTime;

	}
	renderer->WaitForIdle();


	std::cout << "Vulkan resource cleanup." << std::endl;
	delete(renderer);
	std::cout <<std::endl<< "Finished Vulkan resource cleanup." << std::endl;
	IMG_Quit();
	SDL_Quit();

	return 1;
}