#include "WinApiUtils.hpp"

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
