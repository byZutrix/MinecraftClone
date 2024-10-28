#ifndef WINDOW_H
#define WINDOW_H

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

#include "../logger.h"

class Window {
public:
	Window(const char* title, int width, int height);

	bool handleEvents();

	int initSDL();
	SDL_Window* getSDLWindow() const;

	void cleanup();

private:
	SDL_Window* sdlWindow;
	const char* title;
	int width;
	int height;
};

#endif // ! WINDOW_H
