#include "window.h"

Window::Window(const char* title, int width, int height) {
	this->sdlWindow = nullptr;
	this->title = title;
	this->width = width;
	this->height = height;
}

int Window::initSDL() {

	LOG_INFO("Initializing SDL2...");
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			LOG_ERROR("Error initializing SDL: ", SDL_GetError());
			return 1;
		}
	}

	LOG_INFO("Creating SDL2 Window...");
	{
		sdlWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (!sdlWindow) {
			LOG_ERROR("Error creating SDL window!");
			return 1;
		}
	}

	return 0;
}

bool Window::handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {

		case SDL_QUIT:
			return false;

		case SDL_KEYDOWN:
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (event.button.button == SDL_BUTTON_LEFT) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			break;
		}

	}
	return true;
}

void Window::cleanup() {
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();
}

SDL_Window* Window::getSDLWindow() const { return sdlWindow; }