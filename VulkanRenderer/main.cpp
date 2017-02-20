/*=============================================================================
ENTRY POINT - Used to test current functionality of the renderer.
=============================================================================*/


#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

glm::mat4 TransformMatrix(glm::vec3 position,glm::vec3 rotationAxes, float angle)
{
	 glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1), position), glm::radians(angle), rotationAxes);
	 model = glm::scale(model, glm::vec3(0.25, 0.25, 0.25));
	 return model;
}

int main() 
{
	
	InitSDL();

	int appVer[3] = { 0,0,0 };
	Vulkan::KojinRenderer * renderer = nullptr;
	std::shared_ptr<Vulkan::Mesh> mesh;
	std::shared_ptr<Vulkan::Material> material = std::make_shared<Vulkan::Material>(Vulkan::Material());
	
	std::string err;
	bool e = false;
	try
	{
		renderer = new Vulkan::KojinRenderer{window,"Vulkan Tester",appVer};
		material->diffuseTexture = Vulkan::Texture2D::CreateFromFile("textures/Stormtrooper_Diffuse.png").lock()->ImageView();
		//material->albedo = Vk::Texture2D::GetWhiteTexture();
		mesh = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");

	}
	catch (const std::runtime_error& re)
	{
		e = true;
		err = "Runtime error: ";
		err.append(re.what());
		std::cerr << "Runtime error: " << re.what() << std::endl;
	}
	catch (const std::exception& ex)
	{
		e = true;
		err = "Error occurred: ";
		err.append(ex.what());
		std::cerr <<"Error occurred : " << ex.what() << std::endl;
	}
	catch (...)
	{
		e = true;
		err =  "Unknown failure occurred. Possible memory corruption";
		std::cerr << "Unknown failure occurred. Possible memory corruption" << std::endl;
	}


#ifdef _WIN32 

	if(e)
	{
		int length = strlen(err.c_str());
		wchar_t errorText[4096] = { 0 };
		MultiByteToWideChar(0, 0, err.c_str(), length, errorText, length);



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
	}
	
#endif



	bool running = true;
	auto startTime = std::chrono::high_resolution_clock::now();
	float currentDelta = 0.0;
	float fixedTimeStep = 1/60.0f;
	float rotmod = 0;
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
			rotmod += 5* currentDelta;
			//renderer->Update(currentDelta);
			//if (currentDelta == fixedTimeStep)
			//{
			//	//update physics
			//}
			currentDelta -= fixedTimeStep;
		}

		if(currentDelta>0 && currentDelta < fixedTimeStep)
			SDL_Delay((uint32_t)currentDelta);
		
		//load up objects to the renderer
		mesh->modelMatrix = TransformMatrix({ 0,-0.5,-1 }, { 0,1,0 },rotmod);
		renderer->Load(mesh, material);
		mesh->modelMatrix = TransformMatrix({ 1,0, -3 }, { 1,0,0 },rotmod);
		renderer->Load(mesh, material);
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