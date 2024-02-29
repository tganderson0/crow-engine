#pragma once
#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class NetworkHost
{
public:
	NetworkHost();
	std::vector<char> img;
	void start();
	int64_t rowPitch = 0;

private:
	boost::asio::io_context io_context;
	tcp::acceptor acceptor;
};

class NetworkClient
{
public:
	void start();
	NetworkClient();
public:
	std::vector<unsigned char> lastImage;
	int64_t rowPitch = 0;
private:
	boost::asio::io_context io_context;
	tcp::resolver resolver;
	tcp::socket socket;
	std::vector<char> image;
};