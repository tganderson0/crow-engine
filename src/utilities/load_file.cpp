#include "load_file.hpp"

std::string utilities::load_ascii_file(const char* filename)
{
	std::ifstream ifs(filename);
	std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
	return content;
}