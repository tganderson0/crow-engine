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

private:
	boost::asio::io_context io_context;
	tcp::acceptor acceptor;
};

class NetworkClient
{
public:
	NetworkClient();
public:
	std::vector<char> lastImage;
	void start();
private:
	boost::asio::io_context io_context;
	tcp::resolver resolver;
	tcp::socket socket;
	std::vector<char> image;
};