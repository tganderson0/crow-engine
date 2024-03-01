#pragma once
#include "networker.h"
#include <thread>
#include <vector>
#include <chrono>

class RemoteEngine {

public:
	void init();
	void run();
	void handle_remote();

private:
	NetworkClient _client;
	struct SDL_Window* _window{ nullptr };
	struct SDL_Surface* _surface{ nullptr };
	std::thread _image_recv_thread;
	std::thread _input_send_thread;
	struct SDL_Renderer* _renderer{ nullptr };
	std::vector<std::chrono::steady_clock::time_point> transfer_times;
	std::vector<std::chrono::steady_clock::time_point> present_times;
};