#include <fstream>
#include <iostream>

#include "Crypt.h"

void Crypt::cryptFile(const std::string& filePath)
{
    try
    {
        Start(filePath);
    }
    catch (const std::fstream::failure& ex)
    {
        std::cout << ex.what() << std::endl;
    }
}

void Crypt::Start(const std::string& filePath)
{
    const auto key = GetEncyptionKey();
    CryptoPP::ChaCha::Encryption encryptor(key.data(), key.size(), key.data());
    auto fileData(ReadFileBytes(filePath));
    encryptor.ProcessData(fileData->data(), fileData->data(), fileData->size());
	WriteFileBytes(filePath, fileData);
}

typedef std::array<CryptoPP::byte, CryptoPP::ChaCha::MAX_KEYLENGTH> encryptionKey;
encryptionKey Crypt::GetEncyptionKey()
{
    const std::string encryptedEncryptionKey{ "1111111111111111" };
    const std::string xorKey{ "key" };
    std::array<CryptoPP::byte, CryptoPP::ChaCha::MAX_KEYLENGTH> key{ 0 };

    for (int j = 0, i = 0; i < encryptedEncryptionKey.size(); i++)
    {
        if (j == xorKey.size())
            j = 0;

        key.at(i) = (encryptedEncryptionKey[i] ^ xorKey[j]);
        j++;
    }

    return key;
}

std::unique_ptr<std::vector<unsigned char>> Crypt::ReadFileBytes(const std::string& filePath) const
{
    std::fstream inFile(filePath, std::ios::binary | std::ios::in);
    if (!inFile)
        throw std::fstream::failure("Can't open file: " + filePath);

    auto fileData = std::make_unique<std::vector<unsigned char>>(std::vector<unsigned char>((std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>()));
    inFile.close();
    return std::move(fileData);
}

void Crypt::WriteFileBytes(const std::string& filePath, std::unique_ptr<std::vector<unsigned char>>& fileData)
{
    std::fstream outFile(filePath, std::ios::binary | std::ios::out);
    if (!outFile)
        throw std::fstream::failure("Can't open file: " + filePath);

    outFile.write(reinterpret_cast<const char*>(fileData->data()), fileData->size());
    outFile.close();
}