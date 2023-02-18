#pragma once
#include <iostream>

class NetworkExcetion : std::exception
{
public:
	std::string m_message;
	NetworkExcetion(std::string message)
	: m_message(message){}

	const char* what() const noexcept override
	{
		return m_message.c_str();
	}
};
