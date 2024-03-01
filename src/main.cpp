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

void handle_client(RemoteEngine& client)
{
	client.init();
	client.run();
}

int main(int argc, char* argv[])
{

	if (RENDER_HOST)
	{
		NetworkHost networker;
		std::thread t1(handle_host, std::ref(networker));

		//RemoteEngine remoteEngine;
		//std::thread t2(handle_client, std::ref(remoteEngine));

		VulkanEngine engine;

		engine.init();

		engine.networkHost = &networker;

		engine.run();

		engine.cleanup();

		t1.join();
		//t2.join();
	}
	else
	{
		RemoteEngine remoteEngine;
		remoteEngine.init();

		remoteEngine.run();
	}

	return 0;
}
