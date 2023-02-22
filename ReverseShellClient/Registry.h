#pragma once
#include "WinApiUtils.hpp"

class Registry
{
public:
	bool m_IsKeyCreated;

public:
	Registry(const std::string& fullKeyPath);
	~Registry();
	std::string QueryValue(const std::string& value);
	void SetValue(const std::string& value, const std::string& data, unsigned long type = REG_SZ);

private:
	void SplitRegistryPath(const std::string& fullKeyPath);
	unsigned long OpenRegistry();
	HKEY getHkey();

private:
	std::string m_key;
	std::string m_subKey;
	HKEY m_keyHandle;
};