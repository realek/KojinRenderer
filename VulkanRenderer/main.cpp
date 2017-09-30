/*=============================================================================
ENTRY POINT - Used to test current functionality of the renderer.
=============================================================================*/


#pragma once
#include "VKWorldSpace.h"
#include "KojinRenderer.h"
#include "Camera.h"
#include "Light.h"
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
	return windowinfo.info.win.window;
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

bool w = false; //fwd
bool s = false; //bk
bool a = false; //left
bool d = false; //right
bool q = false; // up
bool z = false; // down
bool aF = false;
bool aB = false;
bool aL = false;
bool aR = false;
bool f1 = false; // frameRate
void SDL_INPUT()
{
	SDL_Event evt;
	SDL_PollEvent(&evt);

	switch (evt.type)
	{
	case SDL_EventType::SDL_KEYDOWN:
	{
		switch (evt.key.keysym.sym)
		{
		case SDLK_w:
		{
			w = true;
			break;
		}
		case SDLK_a:
		{
			a = true;
			break;
		}
		case SDLK_s:
		{
			s = true;
			break;
		}
		case SDLK_d:
		{
			d = true;
			break;
		}
		case SDLK_z:
		{
			z = true;
			break;
		}
		case SDLK_q:
		{
			q = true;
			break;
		}

		case SDLK_F1:
		{
			f1 = !f1;
			break;
		}

		case SDLK_UP: 
		{
			aF = true;
			break;
		}

		case SDLK_DOWN: 
		{
			aB = true;
			break;
		}

		case SDLK_RIGHT: 
		{
			aR = true;
			break;
		}
		case SDLK_LEFT: 
		{
			aL = true;
			break;
		}
		}
		break;

	}
	}

}
void RESET_INPUT()
{
	aF = aB = aL = aR = w = a = s = d = z = q = false;
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
	{
		int appVer[3] = { 0,0,0 };
		std::shared_ptr<Vulkan::KojinRenderer> renderer;
		Vulkan::Camera * camera;
		Vulkan::Camera * camera1;
		Vulkan::Camera * camera2;
		Vulkan::Light* light;
		Vulkan::Light* light1;
		Vulkan::Light* light2;
		Vulkan::Light* light3;
		Vulkan::Light* light4;
		Vulkan::Light* lightDirectional;
		std::shared_ptr<Vulkan::Mesh> troper;
		std::shared_ptr<Vulkan::Mesh> troper1;
		std::shared_ptr<Vulkan::Mesh> troper2;
		std::shared_ptr<Vulkan::Mesh> troper3;
		std::shared_ptr<Vulkan::Mesh> at_at;
		std::shared_ptr<Vulkan::Mesh> planeMesh;
		std::shared_ptr<Vulkan::Mesh> cubeMesh;
		std::shared_ptr<Vulkan::Mesh> sphereMesh;

		Vulkan::Material material;
		Vulkan::Material planeMaterial;
		Vulkan::Material whiteMaterial;

		std::string err;
		bool e = false;
		try
		{
			renderer = std::make_shared<Vulkan::KojinRenderer>(window,"Vulkan Tester",appVer);

			//testing stuff
			
			cubeMesh = Vulkan::Mesh::GetCube();
			planeMaterial.diffuseColor = { 0.4f,0.3f,0.0f,1.0f };
			planeMaterial.albedo = renderer->GetTextureWhite();
			planeMaterial.specularity = 0;
			planeMesh = Vulkan::Mesh::GetPlane();
			sphereMesh = Vulkan::Mesh::GetSphere();
			
			at_at = Vulkan::Mesh::LoadMesh("models/Walker.fbx");
			troper = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");
			troper1 = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");
			troper2 = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");
			troper3 = Vulkan::Mesh::LoadMesh("models/Stormtrooper.obj");
			material.albedo = renderer->LoadTexture("textures/Stormtrooper_Diffuse.png",false);
			material.specularity = 0.1f;
			whiteMaterial.albedo = renderer->GetTextureWhite();
			whiteMaterial.specularity = 0.1f;
			planeMesh = Vulkan::Mesh::GetPlane();

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
			std::cerr << "Error occurred : " << ex.what() << std::endl;
		}
		catch (...)
		{
			e = true;
			err = "Unknown failure occurred. Possible memory corruption";
			std::cerr << "Unknown failure occurred. Possible memory corruption" << std::endl;
		}


#ifdef _WIN32 

		if (e)
		{
			int length = (int)strlen(err.c_str());
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
				IMG_Quit();
				SDL_Quit();
				return 0;
			}
		}

#endif


		bool running = true;
		auto startTime = std::chrono::high_resolution_clock::now();
		float fpsTimer = 0;
		int fpsCount = 0;
		float currentDelta = 0.0;
		float fixedTimeStep = 1 / 60.0f;
		float rotmod = 0;

		////Light and camera Test
		//{
		camera = renderer->CreateCamera({ 0, 5, -15 },true);
		camera->LookAt({ 0,0,0 });
	/*	camera1 = renderer->CreateCamera({ 0, 1, 4 },true);
		camera1->SetViewport({ 0.0f,0.0f }, { 0.35f,0.35f });
		camera1->LookAt({ 0,0.0,0.0 });

		camera2 = renderer->CreateCamera({ 3, 2, 0 }, true);
		camera2->SetViewport({ 0.65f,0.0f }, { 0.35f,0.35f });
		camera2->LookAt({ 0,0,0 });*/

		lightDirectional = renderer->CreateLight({ 0,0, 0 });
		lightDirectional->SetType(Vulkan::LightType::Directional);
		lightDirectional->range = 0.0f;
		lightDirectional->intensity = 0.3f;
		lightDirectional->angle = 0.0f;
		lightDirectional->m_rotation = { 50 , -30, 0 };
		lightDirectional->diffuseColor = glm::vec4(1.0, 1.0, 1.0, 1.0);

        light = renderer->CreateLight({ 0.0,3, -5.0 });
        light->SetType(Vulkan::LightType::Spot);
        light->range = 25.0f;
        light->intensity = 1.0f;
        light->angle = 60;
        light->m_rotation = { 45,0,0 };
        light->diffuseColor = glm::vec4(0.0, 0.65, 0.85, 1.0);

        light1 = renderer->CreateLight({ 0.0,3, 6.0 });
        light1->SetType(Vulkan::LightType::Spot);
        light1->range = 25.0f;
        light1->intensity = 1.0f;
        light1->angle = 60;
        light1->m_rotation = { 45,180,0 };
        light1->diffuseColor = glm::vec4(0.65, 0.15, 0.25, 1.0);

        light2 = renderer->CreateLight({ 4.0 ,6, 0.0 });
        light2->SetType(Vulkan::LightType::Spot);
        light2->range = 25.0f;
        light2->intensity = 1.0f;
        light2->angle = 60;
        light2->m_rotation = { 45,-90,0 };
        light2->diffuseColor = glm::vec4(0.25, 0.25, 0.65, 1.0);

		light3 = renderer->CreateLight({ -4.0 ,6, 1.0 });
		light3->SetType(Vulkan::LightType::Spot);
		light3->range = 20.0f;
		light3->intensity = 1.0f;
		light3->angle = 60;
		light3->m_rotation = { 45,0,0 };
		light3->diffuseColor = glm::vec4(0.35, 0.85, 0, 1.0);

		light4 = renderer->CreateLight({ 0.0 ,4, 7.0 });
		light4->SetType(Vulkan::LightType::Spot);
		light4->range = 25.0f;
		light4->intensity = 1.0f;
		light4->angle = 75;
		light4->m_rotation = { 45, 180 ,0 };
		light4->diffuseColor = glm::vec4(0.55, 0.25, 0.15, 1.0);
		

		//}
		////!Light Test

		while (running)
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			float frameDeltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

			//input read
			if (fpsTimer > 1)
			{
				if(f1)
				{
					system("CLS");
					std::cout << "FPS: "<<fpsCount<<std::endl;
					std::cout << "AVG ms: " << (fpsTimer / fpsCount);
				}

				fpsCount = 0;
				fpsTimer = 0;
			}


			//update objects here
			rotmod += 15 * frameDeltaTime;

			at_at->modelMatrix = VkWorldSpace::ComputeModelMatrix({ 0,0,0 }, { 0,rotmod,0 }, { 0.15,0.15,0.15 });
			troper->modelMatrix = VkWorldSpace::ComputeModelMatrix({ 5,0,-6 }, { 0,15,0 }, { 0.5,0.5,0.5 });
			troper1->modelMatrix = VkWorldSpace::ComputeModelMatrix({ 5,0,-4 }, { 0,5,0 }, { 0.5,0.5,0.5 });
			troper2->modelMatrix = VkWorldSpace::ComputeModelMatrix({ -6,0,3 }, { 0,45,0 }, { 0.5,0.5,0.5 });
			troper3->modelMatrix = VkWorldSpace::ComputeModelMatrix({ -5,0,1 }, { 0,-15,0 }, { 0.5,0.5,0.5 });
			planeMesh->modelMatrix = VkWorldSpace::ComputeModelMatrix({ 0,0,0 }, { 0, 0, 0 }, { 40,2,40 });
			cubeMesh->modelMatrix = VkWorldSpace::ComputeModelMatrix({ 2,1,2 }, { 45, rotmod, 0 }, { 1,1,1 });
			sphereMesh->modelMatrix = VkWorldSpace::ComputeModelMatrix({ -2,1,1 }, { 0,rotmod,0 }, { 1,1,1 });

			renderer->Draw({ at_at.get(), troper.get(), troper1.get(), troper2.get(), troper3.get(), planeMesh.get(), sphereMesh.get(), cubeMesh.get() },
			{ &whiteMaterial, &material, &material, &material, &material, &planeMaterial, &whiteMaterial, &whiteMaterial});

			renderer->Render();
			//
			SDL_INPUT();
			//input based
			if (w) light->m_position.z += frameDeltaTime * 5;
			else if (s) light->m_position.z -= frameDeltaTime * 5;
			else if (a) light->m_position.x -= frameDeltaTime * 5;
			else if (d) light->m_position.x += frameDeltaTime * 5;
			else if (q) light->m_position.y += frameDeltaTime * 5;
			else if (z) light->m_position.y -= frameDeltaTime * 5;

			if (aF) camera->m_position.z += frameDeltaTime * 5;
			else if (aB) camera->m_position.z -= frameDeltaTime * 5;
			else if (aL) camera->m_position.x -= frameDeltaTime * 5;
			else if (aR) camera->m_position.x += frameDeltaTime * 5;

			camera->Update();

			running = SDLExitInput();
			RESET_INPUT();

			fpsTimer += frameDeltaTime;
			fpsCount++;
			startTime = currentTime;
		}
		renderer->WaitForIdle();
		std::cout << "Vulkan resource cleanup." << std::endl;
	}


	
	std::cout <<std::endl<< "Finished Vulkan resource cleanup." << std::endl;
	IMG_Quit();
	SDL_Quit();

	return 1;
}