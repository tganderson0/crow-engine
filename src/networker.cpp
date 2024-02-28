#include "networker.h"

#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <array>

#define REMOTE_HOST "localhost"

NetworkHost::NetworkHost() : acceptor(io_context, tcp::endpoint(tcp::v4(), 1234))
{
	std::ifstream infile("../../textures/yokohama_skybox/negx.jpg", std::ios::binary);
	std::vector<char> buffer ((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());


	std::cout << "Size of char: " << sizeof(char) << " Size of file: " << buffer.size() << std::endl;

	std::cout << "Waiting for connection" << std::endl;
	for (;;)
	{
		tcp::socket socket(io_context);
		acceptor.accept(socket);

		for (;;)
		{
			std::array<int64_t, 1> msg_length = { buffer.size() };
			boost::system::error_code ignored_error;
			boost::asio::write(socket, boost::asio::buffer(msg_length), ignored_error);
			boost::asio::write(socket, boost::asio::buffer(buffer), ignored_error);
			std::this_thread::sleep_for(std::chrono::milliseconds(2000000));
		}
	}
}

NetworkClient::NetworkClient() : resolver(io_context), socket(io_context)
{
	std::cout << "Starting connection" << std::endl;
	tcp::resolver::results_type endpoints = resolver.resolve(REMOTE_HOST, "1234");
	boost::asio::connect(socket, endpoints);

	std::cout << "connection completed" << std::endl;

	std::vector<char> image;
	std::vector<char> lastImage;

	while (true)
	{
		int64_t total_size = 0;
		int64_t current_size = 0;
		bool header_read = false;
		for (;;)
		{
			std::array<char, 4096> buf;
			boost::system::error_code error;
			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			
			if (len < sizeof(int64_t) && !header_read)
			{
				continue;
			}

			if (header_read == false)
			{
				memcpy(&total_size, buf.data(), sizeof(int64_t));
				image.resize(total_size);
			}

			if (error == boost::asio::error::eof)
			{
				break; // Connection closed cleanly by peer.
			}
			else if (error)
				throw boost::system::system_error(error); // Some other error.

			

			if (header_read)
			{
				memcpy(image.data() + current_size, buf.data(), len);
				current_size += len;
			}
			else
			{
				memcpy(image.data(), buf.data() + sizeof(int64_t), len - sizeof(int64_t));
				current_size += len - sizeof(int64_t);
				header_read = true;
			}

			if (current_size >= total_size)
			{
				lastImage.resize(image.size());
				memcpy(lastImage.data(), image.data(), image.size());
				
				std::ofstream fs("example.jpg", std::ios::binary);
				fs.write(image.data(), image.size());
				fs.close();
				exit(0);
				break;
			}
		}
	}


}