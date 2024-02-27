#pragma once
#include <iostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class NetworkHost
{
public:
	NetworkHost();

private:
	boost::asio::io_context io_context;
	tcp::acceptor acceptor;
};

class NetworkClient
{
public:
	NetworkClient();

private:
	boost::asio::io_context io_context;
	tcp::resolver resolver;
	tcp::socket socket;

};