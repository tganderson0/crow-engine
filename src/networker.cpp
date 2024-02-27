#include "networker.h"
#include <array>

#define REMOTE_HOST "localhost"

NetworkHost::NetworkHost() : acceptor(io_context, tcp::endpoint(tcp::v4(), 1234))
{
	std::cout << "Waiting for connection" << std::endl;
	for (;;)
	{
		tcp::socket socket(io_context);
		acceptor.accept(socket);

		for (;;)
		{
			std::cout << "SERVER: Starting new message" << std::endl;
			std::string msg = "hello";

			std::array<int64_t, 1> msg_length = { sizeof(char) * 5 };
			boost::system::error_code ignored_error;
			boost::asio::write(socket, boost::asio::buffer(msg_length), ignored_error);
			boost::asio::write(socket, boost::asio::buffer(msg), ignored_error);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
}

NetworkClient::NetworkClient() : resolver(io_context), socket(io_context)
{
	std::cout << "Starting connection" << std::endl;
	tcp::resolver::results_type endpoints = resolver.resolve(REMOTE_HOST, "1234");
	boost::asio::connect(socket, endpoints);

	std::cout << "connection completed" << std::endl;

	while (true)
	{
		std::cout << "Begin read" << std::endl;
		int64_t total_size = 0;
		int64_t current_size = 0;
		bool header_read = false;
		for (;;)
		{
			std::array<char, 128> buf;
			boost::system::error_code error;
			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			
			if (len < sizeof(int64_t) && !header_read)
			{
				continue;
			}

			if (header_read == false)
			{
				memcpy(&total_size, buf.data(), sizeof(int64_t));
			}

			if (error == boost::asio::error::eof)
			{
				std::cout << std::endl << "\nEnd of file\n\n\n\n" << std::endl;
				break; // Connection closed cleanly by peer.
			}
			else if (error)
				throw boost::system::system_error(error); // Some other error.

			if (header_read)
			{
				std::cout.write(buf.data(), len);
				current_size += len;
			}
			else
			{
				std::cout.write(buf.data() + sizeof(int64_t), len - sizeof(int64_t));
				current_size += len - sizeof(int64_t);
				header_read = true;
			}

			if (current_size >= total_size)
			{
				std::cout << std::endl;
				break;
			}
		}
	}


}