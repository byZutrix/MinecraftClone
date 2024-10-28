#ifndef APP_H
#define APP_H

#include "logger.h"
#include "game_engine/window.h"
#include "vulkan_base.h"

class App {

public:
	void run();
private:
	void initLogger();
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

	Window* window;
	Vulkan* vulkan;

};

#endif // APP_H