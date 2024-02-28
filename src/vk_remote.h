#pragma once
#include "networker.h"
#include <thread>

class RemoteEngine {

public:
	void init();
	void run();

private:
	NetworkClient _client;
	struct SDL_Window* _window{ nullptr };
	struct SDL_Surface* _surface{ nullptr };
	std::thread _image_recv_thread;
	std::thread _input_send_thread;
	struct SDL_Renderer* _renderer{ nullptr };
};