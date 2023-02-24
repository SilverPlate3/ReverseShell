#pragma once

#include "chacha.h"
#include "filters.h"
#include <array>

class Crypt
{
public:
    void cryptFile(const std::string& filePath);

private:
    void Start(const std::string& filePath);
    typedef std::array<CryptoPP::byte, CryptoPP::ChaCha::MAX_KEYLENGTH> encryptionKey;
    encryptionKey GetEncyptionKey();
    std::unique_ptr<std::vector<unsigned char>> ReadFileBytes(const std::string& filePath) const;
    void WriteFileBytes(const std::string& filePath, std::unique_ptr<std::vector<unsigned char>>& fileData);
};
