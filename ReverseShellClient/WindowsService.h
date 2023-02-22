#pragma once
#include "WinApiUtils.hpp"

class WindowsServices
{
public:
	void CreateWindowsService();
private:
	SC_HANDLE OpenSercivesManager();
};






























