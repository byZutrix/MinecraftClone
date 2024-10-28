#include "app.h"

void App::run() {
	initLogger();
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void App::initLogger() {
	Logger::getInstance("application.log", LogLevel::INFO);
	LOG_INFO("Logger successfully loaded!");
}

void App::initWindow() {
	window = new Window("Minecraft Clone", 1240, 720);
	window->initSDL();
}

void App::initVulkan() {
	vulkan = new Vulkan(window->getSDLWindow());
	vulkan->initVulkan();
}

void App::mainLoop() {

	float delta = 0.0f;
	uint64_t perfCounterFrequency = SDL_GetPerformanceFrequency();
	uint64_t lastCounter = SDL_GetPerformanceCounter();

	while (window->handleEvents()) {
		vulkan->updateVulkan(delta);
		vulkan->renderVulkan();

		uint64_t endCounter = SDL_GetPerformanceCounter();
		uint64_t counterElapsed = endCounter - lastCounter;
		delta = ((float)counterElapsed) / (float)perfCounterFrequency;
		lastCounter = endCounter;
	}
}

void App::cleanup() {
	vulkan->cleanup();
	window->cleanup();
	
}