#pragma once

#include <Windows.h>
#include <iostream>
#include <sstream> 

class WinApiException : std::exception
{
	std::string m_message;
	DWORD m_lastErrorCode;

public:
	WinApiException(std::string message)
		:m_message(message), m_lastErrorCode(GetLastError()) {}

	const char* what() noexcept
	{
		std::stringstream ss;
		ss << "[Error code: " << m_lastErrorCode << " ]  " << m_message;
		m_message = ss.str();
		return m_message.c_str();
	}
};


class WinApiUtils
{
public:
	WinApiUtils();
	const char* GetCurrentPrcessPathChar();
	const std::string& GetCurrentPrcessPathString();

private:
	std::unique_ptr<char[]> m_currentPrcessPathChar = std::make_unique<char[]>(MAX_PATH);
	std::string m_currentPrcessPathString;
	void SetCurrentProcessPath();
};
