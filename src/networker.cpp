#include "networker.h"

#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <chrono>
#include <turbojpeg.h>


#define REMOTE_HOST "localhost"

NetworkHost::NetworkHost() : acceptor(io_context, tcp::endpoint(tcp::v4(), 1234))
{
}

void NetworkHost::start()
{
	std::ifstream infile("../../textures/yokohama_skybox/negx.jpg", std::ios::binary);
	img = std::vector<char>((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());

	std::cout << "HOST: Waiting for connection" << std::endl;
	for (;;)
	{
		tcp::socket socket(io_context);
		acceptor.accept(socket);

		for (;;)
		{
			std::array<int64_t, 2> msg_length = { img.size(), rowPitch };
			boost::system::error_code ignored_error;
			boost::asio::write(socket, boost::asio::buffer(msg_length), ignored_error);
			boost::asio::write(socket, boost::asio::buffer(img), ignored_error);
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
}

void NetworkClient::start()
{
	std::cout << "Starting connection" << std::endl;
	tcp::resolver::results_type endpoints = resolver.resolve(REMOTE_HOST, "1234");
	boost::asio::connect(socket, endpoints);

	std::cout << "connection completed" << std::endl;
	tjhandle jpegDecompresser = tjInitDecompress();


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

			if (len < sizeof(int64_t) * 2 && !header_read)
			{
				continue;
			}

			if (header_read == false)
			{
				memcpy(&total_size, buf.data(), sizeof(int64_t));
				image.resize(total_size);
				memcpy(&rowPitch, buf.data() + sizeof(int64_t), sizeof(int64_t));
				//std::cout << "Got a new image, size: " << total_size << std::endl;
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
				memcpy(image.data(), buf.data() + 2 * sizeof(int64_t), len - 2 * sizeof(int64_t));
				current_size += len - 2 * sizeof(int64_t);
				header_read = true;
			}

			if (current_size >= total_size)
			{
				//lastImage.resize(image.size());
				//memcpy(lastImage.data(), image.data(), image.size());
				if (lastImage.size() != 1700 * 900 * 4)
				{
					lastImage.resize(1700 * 900 * 4);
				}
				tjDecompress2(jpegDecompresser, (unsigned char*)image.data(), image.size(), lastImage.data(), 1700, 0, 900, TJPF_BGRA, TJFLAG_FASTDCT);
				break;
			}
		}
	}
}

NetworkClient::NetworkClient() : resolver(io_context), socket(io_context)
{
}