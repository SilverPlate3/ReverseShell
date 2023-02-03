#ifndef _SERVER_H_
#define _SERVER_H_

#include <fstream>
#include <boost/asio.hpp>
#include <memory>
#include <filesystem>
#include <iostream>
#include <stdlib.h>

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
        : m_socket(std::move(socket))
    {
        SetClientDedicatedDirectory();
    }

private:
    void SetClientDedicatedDirectory()
    {
        std::string userHomeDirectory(EnvVariable("USERPROFILE"));
        std::string clientIp(m_socket.remote_endpoint().address().to_string());
        m_clientDedicatedDirectory = (std::filesystem::path(userHomeDirectory) / "Downloads" / "Dst" / clientIp).string();
        std::filesystem::create_directory(m_clientDedicatedDirectory);
        std::filesystem::current_path(m_clientDedicatedDirectory);
    }

public:
    void start()
    {
        auto buf = boost::asio::buffer("Command\n\nads", 10);
        Send(buf);
        doRead();
    }

private:
    // Move location and try to integrate with client as both have same function
    template<typename Buffer>
    void Send(Buffer& t_buffer)
    {
        boost::asio::async_write(m_socket, t_buffer,
            [this](const boost::system::error_code& ec, std::size_t)
            {
                //ReadFileBytes(ec);
            });
    }

    // Read until we meet "\n\n"
    void doRead()
    {
        auto self = shared_from_this(); // return another shared_pointer to the *this

        // We recive data from the socket, and read until "\n\n". Now m_requestBuf is suppose to hold the file name and size. 
        async_read_until(m_socket, m_requestBuf, "\n\n",
            [this, self](boost::system::error_code ec, size_t bytes)
            {
                if (!ec)
                processRead(bytes);
                else
                    handleError(__FUNCTION__, ec); // Maybe the error can be EOF ?
            });
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
            m_outputFile.write(m_buf.data(), requestStream.gcount()); // break point insdie the gcount(). Check its return value.
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
        m_outputFile.open(m_fileName, std::ios_base::binary);
        if (!m_outputFile) {
            return;
        }
    }

    void doReadFileContent(size_t t_bytesTransferred)
    {
        // If we recived more then 0 bytes, write them to the destination file. 
        if (t_bytesTransferred > 0) {
            m_outputFile.write(m_buf.data(), static_cast<std::streamsize>(t_bytesTransferred));

            // If the bytes amount we allready got are equal to the m_fileSize we recived in Session::readData() , end the operation. 
            if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
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
        m_outputFile.clear();
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
    std::ofstream m_outputFile;
    size_t m_fileSize;
    std::string m_fileName;
    std::string m_clientDedicatedDirectory;
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