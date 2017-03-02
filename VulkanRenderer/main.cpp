/*=============================================================================
ENTRY POINT - Used to test current functionality of the renderer.
=============================================================================*/


#pragma once
#include "VKWorldSpace.h"
#include "KojinRenderer.h"
#include "Camera.h"
#include "Light.h"
#include "Texture2D.h"
#include "Material.h"
#include "Mesh.h"
#include "SPIRVShader.h"
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1), position), glm::radians(angle), VkWorldSpace::WORLD_UP);
	model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
	return model;

}

int main() 
{
	
	InitSDL();

	int appVer[3] = { 0,0,0 };
	Vulkan::KojinRenderer * renderer = nullptr;
	std::shared_ptr<Vulkan::KojinCamera> camera;
	std::shared_ptr<Vulkan::KojinCamera> camera1;
	std::shared_ptr<Vulkan::Light> light;
	std::shared_ptr<Vulkan::Mesh> mesh;
	Vulkan::Material material;
	std::string err;
	bool e = false;
	try
	{
		renderer = new Vulkan::KojinRenderer{window,"Vulkan Tester",appVer};
		material.diffuseTexture = Vulkan::Texture2D::CreateFromFile("textures/Stormtrooper_Diffuse.png").lock()->ImageView();
		material.specularity = 64;
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

	//Light and camera Test
	{
		camera = renderer->CreateCamera({ 0, 1, -3 });
		camera->SetRotation({0.0,0.0,90.0 });
		camera->LookAt({ 0,0,0 });
		renderer->SetMainCamera(camera);
		//camera1 = renderer->CreateCamera({ 0, -1, 3 });
		//camera1->SetRotation({ 0.0,0.0,0.0 });
		//camera->SetOrthographic();
		//camera->LookAt({ 0,0.0,0.0 });
		light = renderer->CreateLight({ 3.0, -2.0, 0.0 });
		light->angle = 10;
		light->rotation = { 0,1,0 };
		light->diffuseColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
		light->specularColor = glm::vec4(1);

	}
	//!Light Test

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
		//for(int i = 0 ; i < 10; i++)
		//{
		//	mesh->modelMatrix = TransformMatrix({ (i*0.10f),-0.5,-2 }, { 0,1,0 }, rotmod);
		//	renderer->Load(mesh, &material);
		//	mesh->modelMatrix = TransformMatrix({ 1,(i*0.10f), -4 }, { 1,0,0 }, rotmod);
		//	renderer->Load(mesh, &material);
		//	mesh->modelMatrix = TransformMatrix({ -1,0, (i*0.10f)-2 }, { 0,0,1 }, rotmod);
		//	renderer->Load(mesh, &material);
		//	mesh->modelMatrix = TransformMatrix({ (i*0.10f) ,0.5, -2 }, { 1,1,0 }, -rotmod);
		//	renderer->Load(mesh, &material);
		//}
		mesh->modelMatrix = TransformMatrix({ 0,0,0 }, { 0,-1,0 }, rotmod);
		renderer->Load(mesh, &material);
		//!load up objects

		//present

		renderer->Render();
		//

		startTime = currentTime;

	}
	renderer->WaitForIdle();


	std::cout << "Vulkan resource cleanup." << std::endl;
	//delete(renderer);
	std::cout <<std::endl<< "Finished Vulkan resource cleanup." << std::endl;
	IMG_Quit();
	SDL_Quit();

	return 1;
}