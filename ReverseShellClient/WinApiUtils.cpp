#include "WinApiUtils.hpp"
#include <stdlib.h>

WinApiUtils::WinApiUtils()
{
	SetCurrentProcessPath();
}

void WinApiUtils::SetCurrentProcessPath()
{
	GetModuleFileNameA(NULL, m_currentPrcessPathChar.get(), MAX_PATH);
	std::string path(m_currentPrcessPathChar.get());
	m_currentPrcessPathString = path;
}

const char* WinApiUtils::GetCurrentPrcessPathChar() { return m_currentPrcessPathChar.get(); }

const std::string& WinApiUtils::GetCurrentPrcessPathString() { return m_currentPrcessPathString; }

std::string WinApiUtils::GetEnvVariable(const std::string& envVariable)
{
	char* buf = nullptr;
	size_t sz = 0;
	return (_dupenv_s(&buf, &sz, envVariable.c_str()) == 0 && buf != nullptr) ? std::string(buf) : "Error";
}

