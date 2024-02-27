#include <vk_engine.h>
#include <iostream>
#include <networker.h>
#include <thread>
#include <chrono>

#define RENDER_HOST false

void handle_client()
{
	NetworkClient client;
}

void handle_host()
{
	NetworkHost networker;
}

int main(int argc, char* argv[])
{

	if (RENDER_HOST)
	{
		VulkanEngine engine;

		engine.init();

		engine.run();

		engine.cleanup();
	}
	else
	{
		std::thread t1(handle_host);

		std::thread t2(handle_client);

		t1.join();
		t2.join();

	}

	return 0;
}
