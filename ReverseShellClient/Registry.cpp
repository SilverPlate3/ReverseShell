#include "Registry.h"

Registry::Registry(const std::string& fullKeyPath)
{
	SplitRegistryPath(fullKeyPath);
	m_IsKeyCreated = (OpenRegistry() == REG_CREATED_NEW_KEY) ? true : false;
}

Registry::~Registry()
{
	RegCloseKey(m_keyHandle);
}

void Registry::SplitRegistryPath(const std::string& fullKeyPath)
{
	std::size_t pos = fullKeyPath.find(R"(\)");
	if (pos == std::string::npos)
		throw WinApiException("(Ignore the error code) Invalid registry path");

	m_key = fullKeyPath.substr(0, pos);
	m_subKey = fullKeyPath.substr(pos + 1);
}

unsigned long Registry::OpenRegistry()
{
	unsigned long disposition;
	auto result(RegCreateKeyExA(
		getHkey(),
		m_subKey.c_str(),
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&m_keyHandle,
		&disposition
	));

	if (result != ERROR_SUCCESS)
		throw WinApiException("RegOpenKeyExA");

	return disposition;
}

HKEY Registry::getHkey()
{
	if (m_key == "HKEY_CLASSES_ROOT")
		return HKEY_CLASSES_ROOT;
	else if (m_key == "HKEY_CURRENT_CONFIG")
		return HKEY_CURRENT_CONFIG;
	else if (m_key == "HKEY_CURRENT_USER")
		return HKEY_CURRENT_USER;
	else if (m_key == "HKEY_LOCAL_MACHINE")
		return HKEY_LOCAL_MACHINE;
	else if (m_key == "HKEY_USERS")
		return HKEY_USERS;
	else
	{
		std::string message("(Ignore the error code) Not a default Registry key - ");
		message += m_key;
		throw WinApiException(message);
	}
}

std::string Registry::QueryValue(const std::string& value)
{
	unsigned long neededDataSize = 0;
	for (int i = 0; i < 5; i++)
	{
		auto data = std::make_unique<unsigned char[]>(neededDataSize + 1);
		auto result(RegQueryValueExA(
			m_keyHandle,
			value.c_str(),
			NULL,
			NULL,
			reinterpret_cast<LPBYTE>(data.get()),
			&neededDataSize));

		if (result == ERROR_SUCCESS)
		{
			data.get()[neededDataSize] = '\0';
			std::string registryData(reinterpret_cast<char*>(data.get()), neededDataSize);
			return registryData;
		}
		else if (result == ERROR_MORE_DATA)
			continue;
		else if (result == ERROR_FILE_NOT_FOUND)
			throw WinApiException("(Ignore the error code)  The specified value wasn't found");
		else
			throw WinApiException("RegQueryValueExA");
	}
}

void Registry::SetValue(const std::string& value, const std::string& data, unsigned long type)
{
	auto result(RegSetValueExA(
		m_keyHandle,
		value.c_str(),
		0,
		type,
		reinterpret_cast<const BYTE*>(data.c_str()),
		(data.size() + 1)));

	if (result != ERROR_SUCCESS)
		throw WinApiException("RegSetValueExA");
}
