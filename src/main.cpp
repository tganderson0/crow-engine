#include <vk_engine.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "vk_remote.h"

#define RENDER_HOST false

//void handle_client()
//{
//	NetworkClient client;
//}
//
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

		

		RemoteEngine remoteEngine;
		remoteEngine.init();

		remoteEngine.run();
		

		t1.join();
	}

	return 0;
}
