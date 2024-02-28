#include <vk_engine.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "vk_remote.h"

#define RENDER_HOST true


void handle_host(NetworkHost& networker)
{
	networker.start();
}

int main(int argc, char* argv[])
{

	if (RENDER_HOST)
	{
		//NetworkHost networker;
		//std::thread t1(handle_host, networker);

		VulkanEngine engine;

		engine.init();

		//engine.host = &networker;

		engine.run();

		engine.cleanup();

		//t1.join();
	}
	else
	{
		NetworkHost networker;
		std::thread t1(handle_host, std::ref(networker));

		RemoteEngine remoteEngine;
		remoteEngine.init();

		remoteEngine.run();
		

		t1.join();
	}

	return 0;
}
